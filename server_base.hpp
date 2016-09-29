#ifndef __SERVER_BASE_HPP
#define __SERVER_BASE_HPP

#include <boost/asio.hpp>
#include <unordered_map>
#include <thread>
#include <regex>
#include <iostream>

namespace fWeb{

	struct Request{
		// Request method[POST, GET], Request path, HTTP version
		std::string method, path, http_version;
		// Use intelligent pointer to count how many times content used
		std::shared_ptr<std::istream> content;
		std::unordered_map<std::string, std::string> header;
		// Use regex to deal with path match
		std::smatch path_match;
	};

	typedef std::map<std::string, std::unordered_map<std::string,
		std::function<void(std::ostream&, Request&)>>> resource_type;
		
	template <typename socket_type>
	class ServerBase{
	public:
		void start(){
			// Default resources are pushed at the end of vector to be the default response
			for (auto it=resource.begin(); it!=resource.end(); it++)
				all_resources.push_back(it);
			for (auto it=default_resource.begin(); it!=default_resource.end(); it++)
				all_resources.push_back(it);
			// Subclass need to implement accept()
			accept();
			// If num_threads > 1, then m_io_service.run()
			for (size_t c = 1; c < num_threads; c++){
				threads.emplace_back([this](){
						m_io_service.run();
					});
			}

			// Main thread
			m_io_service.run();
			for (auto &t : threads){
				t.join();
			}
		}
		resource_type resource;
		resource_type default_resource;

		ServerBase(unsigned short port, size_t num_threads=1):
			endpoint(boost::asio::ip::tcp::v4(), port),
			acceptor(m_io_service, endpoint),
			num_threads(num_threads){}
	protected:
		boost::asio::io_service m_io_service;
		boost::asio::ip::tcp::endpoint endpoint;
		boost::asio::ip::tcp::acceptor acceptor;
		
		size_t num_threads;
		std::vector<std::thread> threads;
		std::vector<resource_type::iterator> all_resources;
		
		virtual void accept() {}

		Request parse_request(std::istream & stream) const{
			Request request;
			std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
			std::smatch sub_match;

			std::string line;
			getline(stream, line);
			line.pop_back();
			if (std::regex_match(line, sub_match, e)){
				request.method       = sub_match[1];
				request.path         = sub_match[2];
				request.http_version = sub_match[3];

				bool matched;
				e = "^([^:]*): ?(.*)$";
				do{
					getline(stream, line);
					line.pop_back();
					matched = std::regex_match(line, sub_match, e);
					if (matched){
						request.header[sub_match[1]] = sub_match[2];
					}
				} while(matched);
			}
			return request;
		}

		void respond(std::shared_ptr<socket_type> socket,
					 std::shared_ptr<Request> request)const{
			for (auto res_it: all_resources){
				std::regex e(res_it->first);
				std::smatch sm_res;
				if (std::regex_match(request->path, sm_res, e)){
					if (res_it->second.count(request->method) > 0){
						request->path_match = move(sm_res);
						auto write_buffer = std::make_shared<boost::asio::streambuf>();
						std::ostream response(write_buffer.get());
						res_it->second[request->method](response, *request);
						boost::asio::async_write(*socket, *write_buffer,
												 [this, socket, request, write_buffer](const boost::system::error_code& ec,
																					   size_t bytes_transferred){
													 if (!ec && stof(request->http_version) > 1.05)
														 process_request_and_respond(socket);
													 
												 });
						return;
					}
				}
			}
		}
	
		void process_request_and_respond(std::shared_ptr<socket_type> socket) const{
			// Create new buffer for async_read_until()
			auto read_buffer = std::make_shared<boost::asio::streambuf>();

			boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
				  [this, socket, read_buffer](const boost::system::error_code& ec, size_t bytes_transferred){
					  if (!ec){
						  size_t total = read_buffer->size();
						  std::istream stream(read_buffer.get());
						  auto request = std::make_shared<Request>();
						  *request = parse_request(stream);
						  size_t num_additional_bytes = total - bytes_transferred;
						  if (request->header.count("Content-Length") > 0){
							  boost::asio::async_read(*socket, *read_buffer,
							  boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes),
													  [this, socket, read_buffer, request](const boost::system::error_code &ec,
																	   size_t bytes_transferred){
										  if (!ec){
											  request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
											  respond(socket, request);
										  }
								  });
						  }else{
							  respond(socket, request);
						  }
					  }
			  });
		}
	};

	template<typename socket_type>
	class Server: public ServerBase<socket_type> {};
}

#endif  /* __SERVER_BASE_HPP */
