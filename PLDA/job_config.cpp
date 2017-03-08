#include "job_config.h"
using namespace ParallelHLDA;
JobConfig::JobConfig(const JobConfig & c)
{
	this->processID = c.processID;
	this->totalProcessCount = c.totalProcessCount;
	this->taskPerProcess = c.taskPerProcess;
	this->iterationNumber = c.iterationNumber;
	this->documentNumber = c.documentNumber;
	this->alpha = c.alpha;
	this->beta = c.beta;
	this->hierarchStructure = c.hierarchStructure;
	this->model_type = c.model_type;
	this->parallelType = c.parallelType;
	this->filename = c.filename;
	this->filetype = c.filetype;
	this->inferCorpusFile = c.inferCorpusFile;
	this->inferedModelFile = c.inferedModelFile;
	this->inferencing = c.inferencing;

	this->documentWordStart = c.documentWordStart;

}
JobConfig::JobConfig()
{
}

JobConfig::~JobConfig()
{
}
