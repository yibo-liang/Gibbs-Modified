#include "task_generator.h"
#include "task_executor.h"
#include "mpi_helper.h"
#include <random>

TaskGenerator::TaskGenerator(JobConfig &config)
{
	this->config = config;
}

TaskGenerator::~TaskGenerator()
{
}




void TaskGenerator::startMasterJob(
	TaskExecutor &executor)
{

	cout << "Loading Corpus ..." << endl;
	loadCorpus();
	cout << "" << corpus.documents.size() << " documents" << endl;
	cout << "" << corpus.totalWordCount << " words in total" << endl;
	cout << "------------------------------" << endl;

	cout << "Initialising LDA Model ... ";
	model = createInitialModel();
	cout << "------------------------------" << endl;

	cout << "Creating tasks ... " << endl;
	vector<vector<TaskPartition>> taskGroups = generateSimpleTasks(model);
	cout << "" << taskGroups.size() << " task Groups created, each has " << taskGroups.at(0).size() << "tasks" << endl;
	cout << "------------------------------" << endl;
	cout << "Sending tasks to workers ...." << endl;

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
	cout << "------------------------------" << endl;
	cout << endl;

}

int RandInteger(int min, int max)
{
	int range = max - min + 1;
	int num = rand() % range + min;

	return num;
}

Model TaskGenerator::createInitialModel()
{
	Model model;
	//initial values
	model.K = config.hierarchStructure[0];
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

	//assign inital topic to models;
	for (int m = 0; m < model.M; m++) {
		Document &doc = corpus.documents[m];
		model.z[m] = vector<int>(doc.wordCount());
		model.ndsum[m] = corpus.documents[m].wordCount();

		for (int i = 0; i < model.z[m].size(); i++) {
			//random topic
			int topic = RandInteger(0, model.K - 1);
			int w = doc.words[i];

			model.z[m][i] = topic;
			model.nwsum[topic]++;
			model.nw[w][topic]++;
			model.nd[m][topic]++;

		}
	}
	model.corpus = &corpus;
	return model;
}

void TaskGenerator::loadCorpus()
{
	if (config.filetype == "txt") {
		corpus.fromTextFile(config.filename, config.documentNumber, 4, map<int, string>());
	}
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

vector<vector<TaskPartition>> TaskGenerator::generateSimpleTasks(Model &initial_model) {


	//initialise prototype partition
	TaskPartition prototypePartition;
	prototypePartition.K = model.K;
	prototypePartition.V = model.V;
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

	int word_count = corpus.totalWordCount;
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
	for (int doc_i = 0; doc_i < model.M; doc_i++) {
		Document& doc = corpus.documents.at(doc_i);
		int doc_partition = getPartitionID(nd_partition_offsets, doc_i);
		for (int wordIndexInDoc = 0; wordIndexInDoc < doc.wordCount(); wordIndexInDoc++) {
			int w = doc.words.at(wordIndexInDoc);
			int word_partition = getPartitionID(nw_partition_offsets, w);

			TaskPartition * partition = &(*(&partitions[doc_partition]))[word_partition];
			int z_word = initial_model.z[doc_i][wordIndexInDoc];

			int w_offset = nw_partition_offsets[word_partition];
			int m_offset = nd_partition_offsets[doc_partition];

			int partition_doc_i = doc_i - m_offset;
			int partition_w = w - w_offset;

			(*partition).words.push_back(
				vector<int>({
					partition_doc_i ,
					partition_w,
					z_word })
				);

		}
	}


	return partitions;

}

