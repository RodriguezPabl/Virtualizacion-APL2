CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread
LDFLAGS = -lrt

TARGETS = servidor cliente
SRCS = servidor.cpp cliente.cpp utils.cpp
OBJS = $(SRCS:.cpp=.o)
HDRS = shared_mem.hpp utils.hpp

SER_OBJS = servidor.o utils.o
CLI_OBJS = cliente.o utils.o

all: $(TARGETS)

servidor: $(SER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

cliente: $(CLI_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)
