
float mwcReadPseudoRandomValue(mwc64x_state_t* rng) {
	return (1.0f * MWC64X_NextUint(rng)) / (float)(0xffffffff);	// normalized to 1.0
}


__kernel void sample(

	__global int * _iter_partition, //iteration number
	__global const int *partition_offset, // the starting position of the partition, size P * P, access as [col + row*P]
	__global const int *partition_word_count, //the number of words of the partition, size P * P, access as [col + row*P]
	__global const int *w, // size of 2W , in form of [m_w_0, w_0, ...], SHARED by all threads, access as w[offset+ i*2, offset+i*2+1]

	__global int *z, //topic assignment of word i: z_i, SHARED by all threads, access as z[offset+i]

	__global int *nd, //size of cpu partialM * K, access as nd[m*K+k]
	__global int *nw, //size of cpu partialV * K, access as nw[v*K+k]

	__global const int *ndsum, //size of partialM, access as ndsum[m]

	__global int *nwsum_unsync, //size of K * P, access as nwsum[k + p * K], nwsum from each workgroup, before sync
	__local int *nwsum_local, //size of K, access as nwsum[k], each thread will have a unique copy of this in local, will be synchronized
	__global int *nwsum_global, //size of K, nwsum before this iteration of partition

	__global int *debug
	)
{
	// Get the index of the current element to be processed
	// the partition position of this thread

	/* The flollowing 6 const are replaced by using Boost::format at Runtime */
	const int Y = %d; //second dimension size of this workgroup, also Partition number
	const int group_total_p = get_local_size(0);

	__private float p[%d];
	const int K = %d;
	const int V = %d;
	const int M = %d;
	const float alpha = %.6f;
	const float beta = %.6f;
	float Vbeta = (float)V * beta;
	float Kalpha = (float)K * alpha;

	int group_id = get_group_id(0); //dimension should be {PN , N }, so the group id should be different on the second dimension
	int x = get_local_id(0);
	//int y = get_local_id(1);
	int pid = x;

	int iter_count = _iter_partition[0];
	int row = group_id; //partition row position 
	int col = (iter_count + row) %% Y;

	int offset_index = col + row * Y;
	int offset = partition_offset[offset_index];
	int word_count = partition_word_count[offset_index];
	

	//copy nwsum to local
	if (pid < K) { //assume that local work item will always be more than K
		nwsum_local[pid] = nwsum_global[pid];
	}



	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, get_global_id(0) * K, word_count * K);

	int sample_count = 0;
	int swi = pid + sample_count * group_total_p; //swi, the word id being sampled by this work item
	//sampling start
	int z_i = offset + swi;



	//debug[get_global_id(0)] = get_global_id(0);
	while (swi < word_count) {
		//barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
		int wi = (swi + offset) * 2;
		int m = w[wi];
		int v = w[wi + 1];



		int topic = z[z_i];
		
		int old_topic = topic;
		int k;
		for (k = 0; k < K; k++) {
			float A, B, C, D;
			if (k != topic) {
				A = nw[k + v * K];
				B = nwsum_local[k];
				C = nd[k + m * K];
			}
			else {
				A = nw[k + v * K] - 1;
				B = nwsum_local[k] - 1;
				C = nd[k + m * K] - 1;
			}
			D = ndsum[m] - 1;
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
		barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
		if (topic != old_topic) {
			atomic_dec(&nd[m*K + old_topic]);
			atomic_dec(&nw[v*K + old_topic]);
			atomic_dec(&nwsum_local[old_topic]);


			atomic_inc(&nd[m*K + topic]);
			atomic_inc(&nw[v*K + topic]);
			atomic_inc(&nwsum_local[topic]);

			atomic_dec(&debug[v*K + old_topic]);
			atomic_inc(&debug[v*K + topic]);

			

			z[z_i] = topic;
		}

		z_i++;
		sample_count++;
		swi = pid + sample_count * group_total_p;
	}; //exit if there is more workitem than words to sample

	/*if (debug[get_global_id(0)] == get_global_id(0)) {
		debug[pid + group_id * K] = nwsum_unsync[pid + group_id * K];
	}*/

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
	//copy nwsum to global
	if (pid < K) {
		nwsum_unsync[pid + group_id * K] = nwsum_local[pid];

	}

}


__kernel void nwsum_allreduce(

	__global int *iter_partition, //iteration number
	__global int *nwsum_unsync, //size of K * P, access as nwsum[k + P * K]
	__global int *nwsum_global //
	) {

	/* The flollowing 2 const are replaced by using Boost::format at Runtime */
	const int K = %d;
	const int P = %d;

	int k = get_global_id(0);
	if (k >= K) {
		return;
	}

	int nwsum_k = nwsum_global[k];
	int new_nwsum_k = nwsum_k;
	int p;
	for (p = 0; p < P; p++) {
		int diff = nwsum_unsync[k + p*K] - nwsum_k;
		new_nwsum_k = new_nwsum_k + diff;
	}
	nwsum_global[k] = new_nwsum_k;

	if (k == 0) {
		iter_partition[0]++;
	}
}