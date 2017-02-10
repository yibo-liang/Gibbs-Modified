#include "job.h"

Job::Job(JobConfig config, Corpus corpus)
{
	this->config = config;
	this->corpus = corpus;
}

Job::~Job()
{
}

vector<Task> Job::generateSimpleTasks()
{

	vector<Task> result;
	if (config.parallelType == P_MPI) {
		int taskNumber = config.process_number;

		int groupSize = corpus.totalWordCount / taskNumber;

		//divide corpus equally according to word number, rather than document number



		result.push_back(Task());
		int taskNumber = 0;
		int wordIterator = 0;
		for (int doc_i = 0; doc_i < corpus.documents.size(); doc_i++) {
			Document doc = corpus.documents[doc_i];
			if (doc.wordCount > 0) {
				
				for (int docWord_i = 0; docWord_i < doc.wordCount; docWord_i++) {
					result[taskNumber].source
				}
			}
		}

	}

	return;
}
