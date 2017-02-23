
/* Should be used with concat of rng_simplified.cl */
__kernel void vector_add(__global const int *A, __global const int *B, __global int *C) {
	// Get the index of the current element to be processed
	int i = get_global_id(0);
	mwc64x_state_t rng;
	MWC64X_SeedStreams(&rng, 0, i);

	// Do the operation
	int j;
	for (j = 0; j < 100; j++) {
		C[i] = A[i] + B[i] + MWC64X_NextUint(&rng);
	}
}