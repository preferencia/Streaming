CXX = g++ 
INCS = -I./ -I../ -I../Common -I../Codec -I../StreamServer -I../FFMPEG/Linux/include
LIBS = -lpthread 

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = Demuxer.cpp Muxer.cpp

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Multiplex Object Start"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS)
	mv *.o ../StreamServer
	@echo "---------------------------------------"
	@echo "Make Multiplex Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
