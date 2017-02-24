
__kernel void sample(

	__global int * _iter_partition, //iteration number
	__global const int *partition_offset, // the starting position of the partition, size P * P, access as [col + row*P]
	__global const int *partition_word_count, //the number of words of the partition, size P * P, access as [col + row*P]
	__global const int *w, // size of 2W , in form of [m_w_0, w_0, ...], SHARED by all threads, access as w[offset+ i*2, offset+i*2+1]
	__global int *z, //topic assignment of word i: z_i, SHARED by all threads, access as z[offset+i]
	__global int *nd, //size of partialM * K, access as nd[m*K+k]
	__global int *nw, //size of partialV * K, access as nw[v*K+k]
	__global const int *ndsum, //size of partialM, access as ndsum[m]
	__global int *nwsum, //size of K, access as nwsum[k + p * K], each thread will have a unique copy of this in local, will be synchronized
	//__global int *nwsumDiff, //size of P * K, access as nwsumDiff[k+p*K], should be all reduced in the end
	__global int *nwsumBefore //size of K, nwsum before this iteration of partition
	)
{
	// Get the index of the current element to be processed
	// the partition position of this thread

	/* The flollowing 5 const are replaced by using Boost::format at Runtime */
	const int total_work_item = %d; //total amount of work item (parallel thread count)
	float p[%d];
	const int K = %d;
	const int V = %d;
	const float alpha = %.6f;
	const float beta = %.6f;


	int iter_partition = _iter_partition[0];
	int row = get_global_id(0);
	int col = (iter_partition + row) %% total_work_item;

	if (row >= total_work_item) {
		return;
	}

	//sample partition_i
	int idx = col + row*total_work_item;
	int offset = partition_offset[idx];
	int word_count = partition_word_count[idx];


	int m;
	int v;
	int topic;

	float Vbeta = (float)V * beta;
	float Kalpha = (float)K * alpha;

	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, word_count * K * total_work_item, word_count * K);

	
	int k;
	//firstly get reduced nwsum from last iteration
	for (k = 0; k < K; k++) {
		nwsum[k + row*K] = nwsumBefore[k];
	}

	int wi;
	int max_wi = offset + 2 * word_count;
	int z_i = 0;
	for (wi = offset; wi < max_wi; wi = wi + 2) {

		m = w[wi];
		v = w[wi + 1];
		topic = z[z_i];

		nd[m*K + topic]--;
		nw[v*K + topic]--;
		int unique_topic_idx = topic + row * k;
		nwsum[unique_topic_idx]--;
		//nwsumDiff[unique_topic_idx]--;

		for (k = 0; k < K; k++) {
			float A = nw[k + v * K];
			float B = nwsum[k];
			float C = nd[k + m * K];
			float D = ndsum[m] - 1;
			p[k] = (A + beta) / (B + Vbeta) *
				(C + alpha) / (D + Kalpha);
		}
		for (k = 1; k < K; k++) {
			p[k] += p[k - 1];
		}
		float u = ((float)MWC64X_NextUint(&rng) / (float)4294967296) * p[K - 1];
		for (topic = 0; topic < K; topic++) {
			if (p[topic] >= u) {
				break;
			}
		}

		z[z_i] = topic;

		nd[m*K + topic]++;
		nw[v*K + topic]++;
		unique_topic_idx = topic + row * K;
		nwsum[unique_topic_idx]++;
		//nwsumDiff[unique_topic_idx]++;
		z_i++;
	}
}


__kernel void nwsum_allreduce(

	__global int *iter_partition, //iteration number
	__global int *nwsum, //size of K, access as nwsum[k + P * K], each thread will have a unique copy of this in local, will be synchronized
						 //__global int *nwsumDiff, //size of P * K, access as nwsumDiff[k+p*K], should be all reduced in the end
	__global int *nwsumBefore //
	) {

	/* The flollowing 2 const are replaced by using Boost::format at Runtime */
	const int K = %d;
	const int P = %d;

	int k = get_global_id(0);
	if (k >= K) {
		return;
	}

	int nwsum_k = nwsumBefore[k];
	int new_nwsum_k = nwsum_k;
	int p;
	for (p = 0; p < P; p++) {
		int diff = nwsum[k + p*K] - nwsum_k;
		new_nwsum_k = new_nwsum_k + diff;
	}
	nwsumBefore[k] = new_nwsum_k;

	if (k == 0) {
		iter_partition[0]++;
	}
}