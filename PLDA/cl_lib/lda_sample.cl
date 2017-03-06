
float mwcReadPseudoRandomValue(mwc64x_state_t* rng) {
	return (1.0f * MWC64X_NextUint(rng)) / (float)(0xffffffff);	// normalized to 1.0
}

#define K %d
#define V %d
#define M %d
#define alpha %.6ff
#define beta %.6ff
#define Y %d  //second dimension size of this workgroup, also Partition number
#define total_word_count %d

__kernel void sample(

	//__global int * _iter_partition, //iteration number
	__global const int *partition_offset, // the starting position of the partition, size P * P, access as [col + row*P]
	__global const int *partition_word_count, //the number of words of the partition, size P * P, access as [col + row*P]
	__global const int *w, // size of 2W , in form of [m_w_0, w_0, ...], SHARED by all threads, access as w[offset+ i*2, offset+i*2+1]

	__global int *z, //topic assignment of word i: z_i, SHARED by all threads, access as z[offset+i]

	__global int *nd, //size of cpu partialM * K, access as nd[m*K+k]
	__global int *nw, //size of cpu partialV * K, access as nw[v*K+k]

	__global const int *ndsum, //size of partialM, access as ndsum[m]

	//__global int *nwsum_unsync, //size of K * P, access as nwsum[k + p * K], nwsum from each workgroup, before sync
	__local int *nwsum_local, //size of K, access as nwsum[k], each thread will have a unique copy of this in local, will be synchronized
	__global int *nwsum_global, //size of K, nwsum before this iteration of partition

	__global int *debug
	)
{
	// Get the index of the current element to be processed
	// the partition position of this thread

	/* The flollowing 6 const are replaced by using Boost::format at Runtime */
	//const int group_total_p = get_local_size(0);

	__private float p[K];
	float Vbeta = (float)V * beta;
	float Kalpha = (float)K * alpha;

	int pid = get_local_id(0);
	int row = pid; //partition row position 

	//debug[get_global_id(0)] = 1;

	//copy nwsum to local
	if (pid < K) { //assume that local work item will always be more than K
		nwsum_local[pid] = nwsum_global[pid];
		
		//debug[pid + K * group_id] = nwsum_local[pid];

	}



	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, get_global_id(0) * K, total_word_count * K);

	int iter;
	for (iter = 0; iter < Y; iter++) {
		int col = (iter + row) %% Y;
		int offset_index = col + row * Y;
		int offset = partition_offset[offset_index];
		int word_count = partition_word_count[offset_index];

		int wi;
		for (wi = 0; wi < word_count; wi++) {
			int index = (offset + wi) * 2;
			int m = w[index];
			int v = w[index + 1];
			int topic = z[offset + wi];
			nd[m * K + topic]--;
			nw[v * K + topic]--;
			atomic_dec(&nwsum_local[topic]);
			int k;
			for (k = 0; k < K; k++) {
				float A = nw[k + v * K];
				float B = nwsum_local[k];
				float C = nd[k + m * K];
				float D = ndsum[m] - 1;
				p[k] = (A + beta) / (B + Vbeta) *
					(C + alpha) / (D + Kalpha);
			}
			for (k = 1; k < K; k++) {
				p[k] += p[k - 1];
			}
			float u = mwcReadPseudoRandomValue(&rng) * p[K - 1];
			for (topic = 0; topic < K; topic++) {
				if (p[topic] >= u) {
					break;
				}
			}
			z[offset + wi] = topic;
			nd[m*K + topic]++;
			nw[v*K + topic]++;
			atomic_inc(&nwsum_local[k]);

		}

		barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

	}



	//barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
	//copy nwsum to global
	if (pid < K) {
		nwsum_global[pid] = nwsum_local[pid];
		debug[pid] = nwsum_global[pid];
		//debug[pid + group_id * K] = nwsum_local[pid] - nwsum_global[pid];

	}
	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
}