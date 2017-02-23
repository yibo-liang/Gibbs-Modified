#include "mwc64x.cl"

__kernel void sample(

	__global int iter_partition, //iteration number
	__global const int *partition_offset, // the starting position of the partition, size P * P, access as [col + row*P]
	__global const int *partition_word_count, //the number of words of the partition, size P * P, access as [col + row*P]
	__global const int *w, // size of 2W , in form of [m_w_0, w_0, ...], SHARED by all threads, access as w[offset+ i*2, offset+i*2+1]
	__global int *z, //topic assignment of word i: z_i, SHARED by all threads, access as z[offset+i]
	__global int *nd, //size of partialM * K, access as nd[m,k]
	__global int *nw, //size of partialV * K, access as nw[v,k]
	__global const int *ndsum, //size of partialM, access as ndsum[m]
	__global int *nwsum, //size of K, access as nwsum[k + P * K], each thread will have a unique copy of this in local, will be synchronized
	__global int *nwsumDiff, //size of P * K, access as nwsumDiff[k+p*K], should be all reduced in the end
	__global int *nwsumBefore //size of K, nwsum before this iteration of partition
	)
{
	// Get the index of the current element to be processed
	// the partition position of this thread
	int row = get_global_id(0);
	int col = (iter_partition + row) % total_work_item;

	//sample partition_i
	int offset = partition_offset[col + row*total_work_item];
	int word_count = partition_word_count;


	int m;
	int v;
	int topic;
	/* The flollowing 4 const are replaced by using Boost::format at Runtime */
	const int total_work_item; //total amount of work item (parallel thread count)
	const int K = %d;
	const int V = %d;
	const float alpha = %.6f;
	const float beta = %.6f;

	float p[K];
	float Vbeta = (float)V * beta;
	float Kalpha = (float)K * alpha;

	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, word_count * K * total_work_item, word_count * K);

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
		nwsumDiff[unique_topic_idx]--;

		for (int k = 0; k < K; k++) {
			float A = nw[v, k];
			float B = nwsum[k];
			float C = nd[m, k];
			float D = ndsum[m] - 1;
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

		z[z_i] = topic;

		nd[m*K + topic]++;
		nw[v*K + topic]++;
		unique_topic_idx = topic + row * K;
		nwsum[unique_topic_idx]++;
		nwsumDiff[unique_topic_idx]++;
		z_i++;
	}

}