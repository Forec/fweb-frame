#include "server_http.hpp"
#include "handler.hpp"

using namespace fWeb;

int main(){
	unsigned short port = 10086;
	Server<HTTP> server(port, 4);
	start_server<Server<HTTP>>(server);
	return 0;
}
