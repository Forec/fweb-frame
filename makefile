CXX = g++
EXEC_HTTP = fweb_http

SOURCE_HTTP = main_fweb.cpp

OBJECTS_HTTP = main_fweb.o

LDFLAGS_COMMON = -std=c++11 -o3 -pthread -lboost_system
LDFLAGS_HTTP =

LPATH_COMMON = -I/usr/include/boost
LPATH_HTTP =

LLIB_COMMON = -L/usr/lib
LLIB_HTTP =

http:
	$(CXX) $(SOURCE_HTTP) $(LDFLAGS_COMMON) $(LDFLAGS_HTTP) $(LPATH_COMMON) $(LPATH_HTTP) $(LLIB_COMMON) $(LLIB_HTTP) -o $(EXEC_HTTP)

clean:
	rm -f $(EXEC_HTTP) *.o
