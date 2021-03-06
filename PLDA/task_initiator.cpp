#include "task_initiator.h"
#include "task_executor.h"
#include "mpi_helper.h"
#include <random>

TaskInitiator::TaskInitiator(JobConfig &config)
{
	this->config = config;
}

TaskInitiator::~TaskInitiator()
{
}




void TaskInitiator::delieverTasks(TaskExecutor &executor, Model & model)
{

	/*cout << "" << corpus->documents.size() << " documents" << endl;
	cout << "" << corpus->totalWordCount << " words in total" << endl;
	cout << "------------------------------" << endl;
	cout << "Creating tasks ... " << endl;*/
	vector<vector<TaskPartition>> taskGroups = tasksFromModel(model);
	/*cout << "" << taskGroups.size() << " task Groups created, each has " << taskGroups.at(0).size() << "tasks" << endl;
	cout << "------------------------------" << endl;
	cout << "Sending tasks to workers ...." << endl;
	*/
	//send tasks to remote mpi process



	using namespace MPIHelper;

	for (int proc_n = 0; proc_n < taskGroups.size(); proc_n++) {
		auto& group = taskGroups[proc_n];
		if (proc_n == 0) {
			//cout << "send to Master" << endl;
			executor.receiveMasterTasks(group, &model);
		}
		else {
			//cout << "send to slave " << proc_n << endl;
			mpiSend(group, proc_n);
		}
		//cout << "send to PID=" << group.at(0).proc_id << endl;
	}	
	//cout << "------------------------------" << endl;
	//cout << endl;

}


void TaskInitiator::createInitialModel(Corpus & corpus, Model & model, int K)
{

	//cout << "Initialising LDA Model ... " << endl;
	//initial values
	model.id = 0;
	model.super_model_id = -1;
	model.K = K;
	model.M = corpus.documents.size();
	model.V = corpus.indexToWord.size();
	model.alpha = config.alpha;
	model.beta = config.beta;

	//make space for matrices
	model.nw = vec2d<int>(model.V, vector<int>(model.K, 0));
	model.nd = vec2d<int>(model.M, vector<int>(model.K, 0));
	model.nwsum = vector<int>(model.K, 0);
	model.ndsum = vector<int>(model.M, 0);
	model.z = vector<vector<int>>(model.M);
	model.w = vector<vector<int>>(model.M);

	//assign inital topic to models;
	for (int m = 0; m < model.M; m++) {
		Document &doc = corpus.documents[m];
		model.z[m] = vector<int>(doc.wordCount());
		model.w[m] = vector<int>(doc.wordCount());

		model.ndsum[m] = corpus.documents[m].wordCount();

		for (int i = 0; i < model.z[m].size(); i++) {
			//random topic
			int topic = RandInteger(0, model.K - 1);
			int word = doc.words[i];

			model.w[m][i] = word;
			model.z[m][i] = topic;
			model.nwsum[topic]++;
			model.nw[word][topic]++;
			model.nd[m][topic]++;

		}
	}
	model.corpus = &corpus;

	//cout << "------------------------------" << endl;

}

void TaskInitiator::createInitialInferModel(Corpus & corpus, Model & inferModel, Model & newModel)
{

	newModel.id = 0;
	newModel.super_model_id = -1;
	newModel.K = inferModel.K;
	newModel.M = corpus.inferDocuments.size();
	newModel.V = inferModel.V;
	newModel.alpha = config.alpha;
	newModel.beta = config.beta;

	newModel.corpus = &corpus;
	//make space for matrices
	newModel.nw = vec2d<int>(inferModel.V, vector<int>(inferModel.K, 0));
	newModel.nd = vec2d<int>(inferModel.M, vector<int>(inferModel.K, 0));
	newModel.nwsum = vector<int>(inferModel.K, 0);
	newModel.ndsum = vector<int>(inferModel.M, 0);
	newModel.z = vector<vector<int>>(inferModel.M);
	newModel.w = vector<vector<int>>(inferModel.M);

	for (int m = 0; m < newModel.M; m++) {
		Document &doc = corpus.inferDocuments[m];
		newModel.z[m] = vector<int>(doc.wordCount());
		newModel.w[m] = vector<int>(doc.wordCount());

		newModel.ndsum[m] = corpus.inferDocuments[m].wordCount();

		for (int i = 0; i < newModel.z[m].size(); i++) {
			//random topic
			int topic = RandInteger(0, newModel.K - 1);
			int word = doc.words[i];

			newModel.w[m][i] = word;
			newModel.z[m][i] = topic;
			newModel.nwsum[topic]++;
			newModel.nw[word][topic]++;
			newModel.nd[m][topic]++;

		}
	}

}

void TaskInitiator::loadCorpus(Corpus & corpus, JobConfig & config)
{
	//cout << "Loading Corpus ..." << endl;
	if (config.filetype == "txt") {
		corpus.fromTextFile(config.filename, config.documentNumber, 4, map<int, string>());
	}
}

void TaskInitiator::loadInferencingText(Corpus & corpus, JobConfig & config)
{
	if (config.filetype == "txt") {
		corpus.inferencingTextFile(config.filename, config.documentNumber, 4, map<int, string>());
	}
}

void TaskInitiator::loadSerializedCorpus(string corpusFilename, Corpus & corpus)
{
	corpus = loadSerialisable<Corpus>(corpusFilename);
}

int getPartitionID(vector<size_t> partitionVec, int i) {
	if (partitionVec.size() == 1) return 0;
	for (int j = 1; j < partitionVec.size(); j++) {
		size_t p = partitionVec[j];
		if (i < p) {
			return j - 1;
		}
		else if (i >= p && j == partitionVec.size() - 1) {
			return j;
		}
	}
	cout << "Error: task partitioning failed!" << endl;
	throw 20;
}

vector<vector<TaskPartition>> TaskInitiator::tasksFromModel(Model &initial_model) {


	//initialise prototype partition
	TaskPartition prototypePartition;
	prototypePartition.K = initial_model.K;
	prototypePartition.V = initial_model.V;
	prototypePartition.alpha = config.alpha;
	prototypePartition.beta = config.beta;

	int nd_partition_count = config.totalProcessCount;
	int nw_partition_count = config.taskPerProcess;

	vector<vector<TaskPartition>> partitions =
		vector<vector<TaskPartition>>(
			nd_partition_count,
			vector<TaskPartition>(
				nw_partition_count,
				prototypePartition
				)
			);


	/*
		calculate the partition size vector
		this vector should equally divide nd and nw matrix into n^2 partitions by word count
		so each column or row have similiar count of words

		partition nd vs nw as row vs col, so non of two partitions share the same nw or nd

	*/
	vector<size_t> nd_partition_offsets;
	vector<size_t> nw_partition_offsets;

	int word_count = 0;

	for (int k = 0; k < initial_model.nwsum.size(); k++) {
		word_count += initial_model.nwsum[k];
	}


	int row_average = word_count / nd_partition_count;
	int col_average = word_count / nw_partition_count;

	int sum = 0;
	nd_partition_offsets.push_back(0);
	for (int m = 0; m < initial_model.M; m++) {
		int sum_row = 0;
		for (int k = 0; k < initial_model.K; k++) {
			sum_row += initial_model.nd[m][k];
		}
		sum += sum_row;
		if (sum > row_average) {
			sum = 0;
			nd_partition_offsets.push_back(m);
		}
	}

	sum = 0;
	nw_partition_offsets.push_back(0);
	for (int v = 0; v < initial_model.V; v++) {
		int sum_col = 0;
		for (int k = 0; k < initial_model.K; k++) {
			sum_col += initial_model.nw[v][k];
		}
		sum += sum_col;
		if (sum > col_average) {
			sum = 0;
			nw_partition_offsets.push_back(v);
		}
	}
	if (nw_partition_offsets.size() < nw_partition_count) {
		//fill up to partition with empty partition offset, if there is not enough 
		for (int v = nw_partition_offsets.size(); v < nw_partition_count; v++) {
			nw_partition_offsets.push_back(nw_partition_offsets[nw_partition_offsets.size() - 1]);
		}
	}

	//assign offsets for each partition
	for (int row = 0; row < partitions.size(); row++) {
		vector<TaskPartition> * row_p = &partitions[row];
		int temp_id = 0;

		for (int col = 0; col < row_p->size(); col++) {
			TaskPartition * p = &(*row_p)[col];
			p->proc_id = row;
			p->partition_id = temp_id;

			temp_id++;

			p->offsetM = nd_partition_offsets[row];
			p->offsetV = nw_partition_offsets[col];

			if (row == nd_partition_offsets.size() - 1) { // the last row
				p->partitionM = initial_model.M - nd_partition_offsets[row];
			}
			else {
				p->partitionM = nd_partition_offsets[row + 1] - nd_partition_offsets[row];
			}

			if (col == nw_partition_offsets.size() - 1) { // the last col
				p->partitionV = initial_model.V - nw_partition_offsets[col];
			}
			else {
				p->partitionV = nw_partition_offsets[col + 1] - nw_partition_offsets[col];
			}

			p->nd = vector<vector<int>>(p->partitionM, vector<int>(p->K));
			p->nw = vector<vector<int>>(p->partitionV, vector<int>(p->K));
			p->ndsum = vector<int>(p->partitionM, 0);
			p->nwsum = vector<int>(p->K, 0);

			for (int m = 0; m < p->partitionM; m++) {
				for (int k = 0; k < p->K; k++) {
					p->nd[m][k] = initial_model.nd[m + p->offsetM][k];
				}
				p->ndsum[m] = initial_model.ndsum[m + p->offsetM];
			}

			for (int v= 0; v < p->partitionV; v++) {
				for (int k = 0; k < p->K; k++) {
					p->nw[v][k] = initial_model.nw[v + p->offsetV][k];
				}
			}

			for (int k = 0; k < p->K; k++) {
				p->nwsum[k] = initial_model.nwsum[k];
			}


		/*	cout << "Partition row=" << row << ", col=" << col << endl;
			cout << "\tSet offsetM=" << p->offsetM << ", offsetV=" << p->offsetV << endl;
			cout << "\tSet partitionM=" << p->partitionM << ", partitionV=" << p->partitionV << endl;*/

		}
	}
	//assign words for each partition
	for (int doc_i = 0; doc_i < initial_model.M; doc_i++) {
		int doc_partition = getPartitionID(nd_partition_offsets, doc_i);
		for (int wi = 0; wi < initial_model.w.at(doc_i).size(); wi++) {

			int w = initial_model.w.at(doc_i).at(wi);
			int word_partition = getPartitionID(nw_partition_offsets, w);

			TaskPartition * partition = &(*(&partitions[doc_partition]))[word_partition];
			int z_word = initial_model.z[doc_i][wi];
			//int word_index= initial_model.wi[doc_i][wordIndexInDoc]; //word index in that document

			int w_offset = nw_partition_offsets[word_partition];
			int m_offset = nd_partition_offsets[doc_partition];

			int partition_doc_i = doc_i - m_offset;
			int partition_w = w - w_offset;

			(*partition).words.push_back(
				vector<int>({
					partition_doc_i ,
					partition_w,
					z_word})
				);

		}
	}


	return partitions;

}

