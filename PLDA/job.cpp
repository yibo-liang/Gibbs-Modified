#include "job.h"
#include "task_executor.h"
#include "mpi_helper.h"
#include <random>

Job::Job(JobConfig &config)
{
	this->config = config;
}

Job::~Job()
{
}




void Job::startMasterJob(
	TaskExecutor &executor)
{

	cout << "Loading Corpus ...";
	loadCorpus();
	cout << " Done." << endl;
	cout << "\t" << corpus.documents.size() << " documents" << endl;
	cout << "\t" << corpus.totalWordCount << " words in total" << endl;
	cout << "------------------------------" << endl;

	cout << "Initialising LDA Model ... ";
	model = createInitialModel();
	cout << "Done." << endl;
	cout << "------------------------------" << endl;

	cout << "Creating tasks ... " << endl;
	vector<vector<Task>> taskGroups = generateSimpleTasks(model);
	cout << "Done." << endl;
	cout << "\t" << taskGroups.size() << " task Groups created, each has " << taskGroups.at(0).size() << "tasks" << endl;
	cout << "------------------------------" << endl;
	cout << "Sending tasks to workers ...." << endl;

	//send tasks to remote mpi process


	auto iter = taskGroups.begin();

	using namespace MPIHelper;
	for (int proc_n = 0; proc_n < taskGroups.size(); proc_n++) {
		auto& group = taskGroups[proc_n];
		if (proc_n == 0) {
			cout << "send to Master" << endl;
			executor.receiveMasterTasks(group, &model);
		}
		else {
			cout << "send to slave " << proc_n << endl;
			mpiSend(*iter, proc_n);
		}
	}

	cout << "Done." << endl;

}

int RandInteger(int min, int max)
{
	int range = max - min + 1;
	int num = rand() % range + min;

	return num;
}

Model Job::createInitialModel()
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
			int topic = RandInteger(0, model.K-1);
			int w = doc.words[i];

			model.z[m][i] = topic;
			model.nwsum[topic]++;
			model.nw[w][topic]++;
			model.nd[m][topic]++;

		}
	}

	return model;
}

void Job::loadCorpus()
{
	if (config.filetype == "txt") {
		corpus.fromTextFile(config.filename, 4, map<int, string>());
	}
}

vector<vector<Task>> Job::generateSimpleTasks(Model &initial_model)
{
	//cout << "generateSimpleTasks 117" << endl;
	int taskNumber = config.totalProcessCount * config.taskPerProcess;
	//cout << "generateSimpleTasks 127, taskNumber=" << taskNumber << endl;

	Task sampleTask;
	sampleTask.K = model.K;
	sampleTask.V = model.V;
	sampleTask.alpha = initial_model.alpha;
	sampleTask.beta = initial_model.beta;
	sampleTask.nwsum = initial_model.nwsum;

	//cout << "generateSimpleTasks 126, total p=" << config.totalProcessCount << endl;
	vector<vector<Task>> result = vector<vector<Task>>(config.totalProcessCount, vector<Task>(config.taskPerProcess, sampleTask));
	auto& tasksForSingleExecutor = result[0];
	

	if (config.parallelType == P_MPI) {
		int groupSize = corpus.totalWordCount / (taskNumber)+1;

		//divide corpus equally according to word number, rather than document number

		int processNumber_i = 0;
		int taskNumber_i = 0;
		int wordIterator = 0; //word index iterator of the whole corpus, that is, the total set of word instances of all documents
		//cout << "generateSimpleTasks 138, gsize=" << groupSize << endl;
		for (int doc_i = 0; doc_i < corpus.documents.size(); doc_i++) {
			Document& doc = corpus.documents.at(doc_i);
			//cout << "generateSimpleTasks 140, doc_i=" << doc_i<< endl;
			//tasksForSingleExecutor[taskNumber_i].ndsum[doc_i] = (initial_model.ndsum.at(doc_i));
			for (int docWord_i = 0; docWord_i < doc.wordCount(); docWord_i++) {


				int w = doc.words.at(docWord_i);
				tasksForSingleExecutor[taskNumber_i]
					.wordSampling
					.push_back(vector<int>({ doc_i, w, docWord_i }));
				int k = initial_model.z[doc_i][docWord_i]; //k, topic assignment of the word in doc
				tasksForSingleExecutor[taskNumber_i].z.push_back(k);
				tasksForSingleExecutor[taskNumber_i].vocabulary[w] = true;
				tasksForSingleExecutor[taskNumber_i].docCollection[doc_i] = true;

				//cout << "generateSimpleTasks 153, word_i="<< docWord_i << endl;
				//next word
				wordIterator++;
				if (wordIterator % groupSize == 0) {

					//before going to next task, add neccessary nd nw segment 



					taskNumber_i++;
					//next task 
					if (taskNumber_i % config.taskPerProcess == 0 ) {
						processNumber_i++;
						taskNumber_i = 0;
						tasksForSingleExecutor = result[processNumber_i];
						//tasksForSingleExecutor[taskNumber_i].ndsum[doc_i] = (initial_model.ndsum.at(doc_i));

					}

					//cout << "Cur tn=" << taskNumber_i << endl;
				}
				
			}
		}

	}
	for (int pid = 0; pid < result.size();pid++) {
		auto & vec = result[pid];
		for (int task_id = 0; task_id < vec.size();task_id++) {
			auto & task = vec[task_id];
			task.id = task_id;
			for (auto &it : task.vocabulary) {
				task.nw[it.first] = initial_model.nw[it.first];
				
			}

			for (auto &it : task.docCollection) {
				task.nd[it.first] = initial_model.nd[it.first];
				task.ndsum[it.first] = initial_model.ndsum[it.first];
			}

			
		}
	}

	//cout << "generateSimpleTasks 170" << endl;


	return result;
}
