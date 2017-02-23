#include "mwc64x.cl"

__kernel void sample(
	__global const int total_work_item, //total amount of work item (parallel thread count)
	__global const int iter_partition, //iteration number
	__global const float alpha, //topic number
	__global const float beta, //topic number
	__global const int K, //topic number
	__global const int V, //topic number
	__global const int *partition_offset, // the starting position of the partition, size P * P, access as [col + row*P]
	__global const int *partition_word_count, //the number of words of the partition, size P * P, access as [col + row*P]
	__global const int *w, // size of 2W , in form of [m_w_0, w_0, ...], SHARED by all threads, access as w[offset+ i*2, offset+i*2+1]
	__global int *z, //topic assignment of word i: z_i, SHARED by all threads, access as z[offset+i]
	__global int *nd, //size of partialM * K, access as nd[m,k]
	__global int *nw, //size of partialV * K, access as nw[v,k]
	__global const int *ndsum, //size of partialM, access as ndsum[m]
	__global int *nwsum, //size of P * K, access as nwsum[k+p*K], each thread will have a unique copy, will be synchronized
	__global int *nwsumDiff //size of P * K, access as nwsumDiff[k+p*K], should be all reduced in the end

	)
{
	// Get the index of the current element to be processed
	int thread_i = get_global_id(0);
	// the partition position of this thread
	int col = (iter_partition + thread_i) % total_work_item;
	int row = thread_i;

	//sample partition_i
	int offset = partition_offset[col + row*total_work_item];
	int word_count = partition_word_count;


	int m;
	int v;
	int topic;
	const int K_local = K;
	float p[K];
	float alpha_ = alpha;
	float beta_ = beta;
	float Vbeta = (float)partialV * beta;
	float Kalpha = (float)K * alpha;

	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, word_count * K_local * total_work_item, word_count * K_local);

	int wi;
	for (wi = 0; wi < word_count; wi++) {
		int idx = offset + wi * 2;
		m = w[idx];
		v = w[idx+1];
		topic = z[offset + wi];

		nd[m*K_local + topic]--;
		nw[v*K_local + topic]--;
		int unique_topic_idx = topic + row * K_local;
		nwsum[unique_topic_idx]--;
		nwsumDiff[unique_topic_idx]--;

		for (int k = 0; k < K; k++) {
			float A = nw[v, k];
			float B = nwsum[k];
			float C = nd[m, k];
			float D = ndsum[m]-1;
			p[k] = (A + beta_) / (B + Vbeta) *
				(C + alpha_) / (D + Kalpha);
		}
		for (int k = 1; k < K; k++) {
			p[k] += p[k - 1];
		}
		float u = ((float)MWC64X_NextUint(&rng) / (float)4294967296) * p[K - 1];
		for (topic = 0; topic < K; topic++) {
			if (p[topic] >= u) {
				break;
			}
		}

		z[offset + wi] = topic;

		nd[m*K_local + topic]++;
		nw[v*K_local + topic]++;
		unique_topic_idx = topic + row * K_local;
		nwsum[unique_topic_idx]++;
		nwsumDiff[unique_topic_idx]++;

	}

}