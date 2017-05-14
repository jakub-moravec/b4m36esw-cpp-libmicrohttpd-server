CXX = g++
CXXFLAGS = -Wall
INCLUDES = -I/opt/microhttpd/include -I/opt/zlib/include
LFLAGS = -L/opt/microhttpd/lib -L/opt/zlib/lib
LIBS = -lmicrohttpd -lz

install: *.cpp
	$(CXX) *.cpp $(CXXFLAGS) $(INCLUDES) -o c_server $(LFLAGS) $(LIBS)

clean:
	rm c_server