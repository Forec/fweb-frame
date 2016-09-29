#include "server_base.hpp"
#include <fstream>

using namespace std;
using namespace fWeb;

template<typename SERVER_TYPE>
void start_server(SERVER_TYPE &server){
	// Add methods to server

	// Deal with request for /string, return the string it posted
	server.resource["^/string/?$"]["POST"] = [](ostream& response, Request& request){
		stringstream ss;
		*request.content >> ss.rdbuf();
		string content = ss.str();
		response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
	};

	// Deal with request for /info
	server.resource["^/info/?$"]["GET"] = [](ostream& response, Request& request){
		stringstream content_stream;
		content_stream << "<h1>Request:</h1>";
		content_stream << request.method << " " << request.path << "HTTP/" << request.http_version << "<br>";
		for (auto &header: request.header){
			content_stream << header.first << ": " << header.second << "<br>";
		}
		content_stream.seekp(0, ios::end);
		response << "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp() << "\r\n\r\n" << content_stream.rdbuf();
	};

	// Deal with default GET, index.html
	server.default_resource["^/?(.*)$"]["GET"] = [](ostream& response, Request &request){
		string filename = "web/";
		string path = request.path_match[1];

		// avoid `..`
		size_t last_pos = path.rfind(".");
		size_t current_pos = 0;
		size_t pos;
		while ((pos=path.find('.', current_pos)) != string::npos &&
			   pos != last_pos){
			current_pos = pos;
			path.erase(pos, 1);
			last_pos--;
		}

		filename += path;
		ifstream ifs;
		if (filename.find('.') == string::npos){
			if (filename[filename.length()-1] != '/')
				filename += '/';
			filename += "index.html";
		}
		ifs.open(filename, ifstream::in);

		if (ifs){
			ifs.seekg(0, ios::end);
			size_t length=ifs.tellg();

			ifs.seekg(0, ios::beg);
			response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n" << ifs.rdbuf();
			ifs.close();
		} else{
			string content = "Could not open file " + filename;
			response << "HTTP/1.1 404 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
		}
	};
	server.start();
}
