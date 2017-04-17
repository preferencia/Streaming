.SUFFIXES : .cpp .o 

CXX = g++ 
INCS = -I./ -I../ -I./Stream -I./Network/Listen -I./Network/Session -I../Common -I../Codec -I../Multiplex -I../../../FFMPEG/Build/include
LIBS = -lpthread \
       ../../../FFMPEG/Build/lib/libavcodec.so \
	   ../../../FFMPEG/Build/lib/libavformat.so \
	   ../../../FFMPEG/Build/lib/libavfilter.so \
	   ../../../FFMPEG/Build/lib/libavutil.so \
	   ../../../FFMPEG/Build/lib/libpostproc.so \
	   ../../../FFMPEG/Build/lib/libswscale.so \
	   ../../../FFMPEG/Build/lib/libswresample.so \
	   ../../../FFMPEG/Build/lib/libfdk-aac.so \
	   ../../../FFMPEG/Build/lib/libmp3lame.so \
	   ../../../FFMPEG/Build/lib/libopus.so \
	   ../../../FFMPEG/Build/lib/libvpx.so \
	   ../../../FFMPEG/Build/lib/libx264.so \
	   ../../../FFMPEG/Build/lib/libx265.so

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
