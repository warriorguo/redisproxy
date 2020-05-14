
CXXFLAGS= -Wall -DNDEBUG -g -O3 -std=c++0x

LIB= -lm
OBJ= buffer_reader.o buffer.o cmd.o connections.o epoll.o error.o logger.o mem_alloc.o server.o session.o netio.o utils.o upstream.o options.o main.o
TARGET= redisproxy

$(TARGET):$(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIB)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) 
