#include "document.h"

using namespace ParallelHLDA;
Document::Document()
{
}

Document::~Document()
{
}

int Document::wordCount()
{
	return this->words.size();
}
