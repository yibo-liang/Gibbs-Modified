#pragma once

#ifndef LDA_INTERFACE
#define LDA_INTERFACE

#include "shared_header.h"
#include "job_config.h"
#include "task_initiator.h"
#include "task_executor.h"
#include "mpi_helper.h"
namespace ParallelHLDA {
	class LdaInterface
	{
	public:

		int master(JobConfig &config);
		int slave(JobConfig config);

		LdaInterface(JobConfig config);
		LdaInterface();
		~LdaInterface();

	private:

		void recursiveEstimation(Model & model, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level);
		void recursiveInference(Model & inferModel, Model & newModel, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level);
		void masterHierarchical(JobConfig &config);
		void masterHierarchicalInference(JobConfig & config);

		string nameModel(JobConfig &config);
	};

}
#endif // !LDA_INTERFACE
