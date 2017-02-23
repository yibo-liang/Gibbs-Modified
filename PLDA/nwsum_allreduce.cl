
__kernel void nwsum_allreduce(

	__global int iter_partition, //iteration number
	__global int *nwsum, //size of K, access as nwsum[k + P * K], each thread will have a unique copy of this in local, will be synchronized
	__global int *nwsumDiff, //size of P * K, access as nwsumDiff[k+p*K], should be all reduced in the end
	__global int *nwsumBefore //
) {

	/* The flollowing 2 const are replaced by using Boost::format at Runtime */
	const int K = %d;
	const int totalP = %d;


}