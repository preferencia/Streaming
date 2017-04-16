CXX = g++ 
INCS = -I../ -I../../ -I../../Stream -I../../../Common 
LIBS = -lpthread 

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = SessionThread.cpp SessionSocket.cpp

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Session Object Start"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS) $(LIBS)
	mv *.o ../../
	@echo "---------------------------------------"
	@echo "Make Session Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
