.SUFFIXES : .cpp .o 

CXX = g++ 
INCS = -I./ -I../ -I./Stream -I./Network/Listen -I./Network/Session -I../Common -I../Codec -I../Multiplex -I../FFMPEG/Linux/include
LIBS = -lpthread \
       ../FFMPEG/Linux/lib/libavcodec.so \
	   ../FFMPEG/Linux/lib/libavformat.so \
	   ../FFMPEG/Linux/lib/libavfilter.so \
	   ../FFMPEG/Linux/lib/libavutil.so \
	   ../FFMPEG/Linux/lib/libpostproc.so \
	   ../FFMPEG/Linux/lib/libswscale.so \
	   ../FFMPEG/Linux/lib/libswresample.so \
	   ../FFMPEG/Linux/lib/libfdk-aac.so \
#	   ../FFMPEG/Linux/lib/libmp3lame.so \
#	   ../FFMPEG/Linux/lib/libopus.so \
#	   ../FFMPEG/Linux/lib/libvpx.so \
	   ../FFMPEG/Linux/lib/libx264.so \
	   ../FFMPEG/Linux/lib/libx265.so

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT -D_THREAD_SAFE	
NETWORK_OBJS = ListenSocket.o SessionThread.o SocketThread.o SessionSocket.o ConnectSocket.o
STREAM_OBJS = StreamThread.o StreamSource.o ObjectManager.o Demuxer.o Muxer.o Decoder.o Encoder.o Codec.o
TOTAL_OBJS = StreamServer.o stdafx.o $(NETWORK_OBJS) $(STREAM_OBJS)
SRCS = StreamServer.cpp stdafx.cpp
TARGET = StreamServer
#TOTAL_OBJS = StreamSourceTest.o stdafx.o $(NETWORK_OBJS) $(STREAM_OBJS)
#SRCS = StreamSourceTest.cpp stdafx.cpp
#TARGET = StreamSourceTest

all : _COMMON _CODEC _MULTIPLEX _STREAM _SESSION _LISTEN $(TARGET)

_COMMON : 
	cd ../Common; $(MAKE)

_CODEC : 
	cd ../Codec; $(MAKE)

_MULTIPLEX : 
	cd ../Multiplex; $(MAKE)
	
_STREAM :
	cd ./Stream/; $(MAKE)
	
_SESSION :
	cd ./Network/Session; $(MAKE)
	
_LISTEN :
	cd ./Network/Listen; $(MAKE)
	
#$(TARGET) : $(OBJS)
#	$(CXX) $(OBJS) -D_REENTRANT -o $@ -lpthread

#.cpp .o:
#	$(CXX) $(CXXFLAGS) $< -o $@
	
$(TARGET) :
	$(CXX) $(SRCS) -c $< $(CXXFLAGS) $(INCS)
	$(CXX) $(CXXFLAGS) $(INCS) -o $@ $(TOTAL_OBJS) $(LIBS)
	
clean : 
	rm -rf $(TOTAL_OBJS) $(TARGET)
