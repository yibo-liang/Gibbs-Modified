#pragma once
#ifndef REQUEST_HANDLER
#define REQUEST_HANDLER

#include "model.h"
using namespace ParallelHLDA;

class RequestHandler
{
public:
	Model * model;
	RequestHandler(Model * model);
	~RequestHandler();

	string respond(string url);

private:

};

string RequestHandler::respond(string url) {
	return "";
}

RequestHandler::RequestHandler(Model * model)
{
	this->model = model;
}

RequestHandler::~RequestHandler()
{
}

#endif // !REQUEST_HANDLER
