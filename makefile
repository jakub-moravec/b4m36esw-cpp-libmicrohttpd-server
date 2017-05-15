CXX = g++
CXXFLAGS = -Wall -std=c++0x -o2
INCLUDES = -I/opt/microhttpd/include -I/opt/zlib/include -I/home/james/school/esw/week4/liburcu/include
LFLAGS = -L/opt/microhttpd/lib -L/opt/zlib/lib  -L/home/james/school/esw/week4/liburcu/lib
LIBS = -lmicrohttpd -lz -lpthread  -lurcu -lrt

install: *.cpp
	$(CXX) *.cpp $(CXXFLAGS) $(INCLUDES) -o c_server $(LFLAGS) $(LIBS)

clean:
	rm c_server