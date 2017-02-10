#include "document.h"

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

int Document::textLength()
{
	return this->text.length();
}
