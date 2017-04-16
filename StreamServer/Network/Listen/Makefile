CXX = g++ 
INCS = -I../ -I../../ -I../Session -I../../Stream -I../../../Common 
LIBS = -lpthread 

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = ListenSocket.cpp

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Listen Socket Object Start"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS) $(LIBS)
	mv *.o ../../
	@echo "---------------------------------------"
	@echo "Make Listen Socket Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
