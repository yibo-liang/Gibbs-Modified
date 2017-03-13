#pragma once
#ifndef HTTP_SESSION
#define HTTP_SESSION
#include "http_header.h"
#include "model.h"
#include <boost/asio.hpp>
#include <string>
#include <memory>

using namespace boost;
using namespace boost::system;
using namespace boost::asio;
using namespace ParallelHLDA;

class session
{
	asio::streambuf buff;
	http_headers headers;
	
	const Model & model;

	static void read_body(std::shared_ptr<session> pThis)
	{
		int nbuffer = 1000;
		std::shared_ptr<std::vector<char>> bufptr = std::make_shared<std::vector<char>>(nbuffer);
		asio::async_read(pThis->socket, boost::asio::buffer(*bufptr, nbuffer), [pThis](const error_code& e, std::size_t s)
		{
		});
	}

	static void read_next_line(std::shared_ptr<session> pThis)
	{
		
		asio::async_read_until(pThis->socket, pThis->buff, '\r', [pThis](const error_code& e, std::size_t s)
		{
			std::string line, ignore;
			std::istream stream{ &pThis->buff };
			std::getline(stream, line, '\r');
			std::getline(stream, ignore, '\n');
			pThis->headers.on_read_header(line);

			if (line.length() == 0)
			{
				if (pThis->headers.content_length() == 0)
				{
					std::shared_ptr<std::string> str = std::make_shared<std::string>(pThis->headers.get_response());
					asio::async_write(pThis->socket, boost::asio::buffer(str->c_str(), str->length()), [pThis, str](const error_code& e, std::size_t s)
					{
						std::cout << "done" << std::endl;
					});
				}
				else
				{
					pThis->read_body(pThis);
				}
			}
			else
			{
				pThis->read_next_line(pThis);
			}
		});
	}

	static void read_first_line(std::shared_ptr<session> pThis)
	{
		asio::async_read_until(pThis->socket, pThis->buff, '\r', [pThis](const error_code& e, std::size_t s)
		{
			std::string line, ignore;
			std::istream stream{ &pThis->buff };
			std::getline(stream, line, '\r');
			std::getline(stream, ignore, '\n');
			pThis->headers.on_read_request_line(line);
			pThis->read_next_line(pThis);
		});
	}

public:

	ip::tcp::socket socket;

	session(io_service& io_service, Model & model)
		:socket(io_service),
		model(model)
	{
		
	}

	static void interact(std::shared_ptr<session> pThis)
	{
		read_first_line(pThis);
	}
};

#endif // !HTTP_SESSION
