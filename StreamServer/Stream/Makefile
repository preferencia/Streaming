CXX = g++ 
INCS = -I../ -I../../ -I../../Common -I../../Codec -I../../Multiplex
LIBS = -lpthread 

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = StreamThread.cpp StreamSource.cpp

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Stream Object"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS) $(LIBS)
	mv *.o ../
	@echo "---------------------------------------"
	@echo "Make Stream Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
