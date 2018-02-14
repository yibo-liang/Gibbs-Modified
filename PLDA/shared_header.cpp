#include "shared_header.h"

namespace ParallelHLDA {
	std::mt19937 rng;
}

double ParallelHLDA::get_random()
{

	static std::uniform_real_distribution<double> dis(0, 1); // rage 0 - 1
	double r = dis(rng);
	return r;
}

int ParallelHLDA::RandInteger(int min, int max)
{
	int range = max - min + 1;
	int num = (int)(get_random() * range) + min;

	return num;
}

void ParallelHLDA::randomSeed(int seed)
{
	if (seed > -1) {
		rng = std::mt19937();
		rng.seed(seed);
		cout << "Using Seed = " << seed << "\n";
	}
}
