CXX = g++
CXXFLAGS = -Wall
INCLUDES = -I/opt/microhttpd/include
LFLAGS = -L/opt/microhttpd/lib
LIBS = -lmicrohttpd

install: *.cpp
	$(CXX) *.cpp $(CXXFLAGS) $(INCLUDES) -o c_server $(LFLAGS) $(LIBS)

clean:
	rm c_server