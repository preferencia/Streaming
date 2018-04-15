CXX = g++ 
INCS = -I./ -I../ -I../Codec -I../Multiplex -I../StreamServer
LIBS = -lpthread 

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = SocketThread.cpp ConnectSocket.cpp ObjectManager.cpp HWAccel.cpp

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Common Object Start"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS) $(LIBS)
	mv *.o ../StreamServer
	@echo "---------------------------------------"
	@echo "Make Common Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
