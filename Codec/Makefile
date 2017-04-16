CXX = g++ 
INCS = -I./ -I../ -I../Common -I../StreamServer -I../FFMPEG/Linux/include
LIBS = -lpthread

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT	
SRCS = Codec.cpp Decoder.cpp Encoder.cpp 

OBJS : 
	@echo "---------------------------------------"
	@echo "Make Codec Object Start"
	@echo "---------------------------------------"
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS) $(LIBS)
	mv *.o ../StreamServer
	@echo "---------------------------------------"
	@echo "Make Codec Object Complete"
	@echo "---------------------------------------"

clean : 
	rm -rf *.o
