
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

	__private float p[%d];
	const int K = %d;
	const int V = %d;
	const int M = %d;
	const float alpha = %.6f;
	const float beta = %.6f;
	debug[get_global_id(0)] = get_global_id(0);

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