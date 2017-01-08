
CXX = g++ 
CXXFLAGS = -c -Wall
CPPFLAGS  = -I../Common

SRCS = SimpleServer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = SimpleServer

all : $(TARGET)

#$(TARGET) : $(OBJS)
#	$(CXX) $(OBJS) -D_REENTRANT -o $@ -lpthread

#.cpp .o:
#	$(CXX) $(CXXFLAGS) $< -o $@
	
$(TARGET) :
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRCS) -o $(TARGET)
	
clean : 
	rm -rf $(OBJS) $(TARGET)
