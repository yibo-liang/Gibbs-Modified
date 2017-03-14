#pragma once
#ifndef HTTP_HEADERS
#define HTTP_HEADERS

#include <boost/asio.hpp>
#include <string>
#include <memory>
#include <iostream>
using namespace boost;
using namespace boost::system;
using namespace boost::asio;

class http_headers
{
	std::string method;
	std::string url;
	std::string version;

	std::map<std::string, std::string> headers;


public:

	std::string getUrl() {
		return url;
	}

	const std::map<std::string, std::string> & getHeader() {
		return headers;
	}

	int content_length()
	{
		auto request = headers.find("content-length");
		if (request != headers.end())
		{
			std::stringstream ssLength(request->second);
			int content_length;
			ssLength >> content_length;
			return content_length;
		}
		return 0;
	}

	void on_read_header(std::string line)
	{
		//std::cout << "header: " << line << std::endl;

		std::stringstream ssHeader(line);
		std::string headerName;
		std::getline(ssHeader, headerName, ':');

		std::string value;
		std::getline(ssHeader, value);
		headers[headerName] = value;
	}

	void on_read_request_line(std::string line)
	{
		std::stringstream ssRequestLine(line);
		ssRequestLine >> method;
		ssRequestLine >> url;
		ssRequestLine >> version;

		std::cout << method << "\t\t" << url << "\t\t" << std::flush;
	}
};

#endif // !HTTP_HEADERS

