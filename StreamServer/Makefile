.SUFFIXES : .cpp .o 

CXX = g++ 
INCS = -I./ -I../ -I./Stream -I./Network/Listen -I./Network/Session -I../Common -I../Codec -I../Multiplex -I/usr/include
LIBS = -lavdevice		\
		-lavformat		\
		-lavfilter		\
		-lavcodec 		\
		-lswresample 	\
		-lswscale 		\
		-lavutil 			\
		-lpostproc 		\
		-lfreetype		\
		-lfdk-aac 		\
		-lx264 				\
		-lva-drm 			\
		-lva-x11 			\
		-lva 					\
		-lX11 				\
		-ldl 					\
		-llzma 				\
		-lz 					\
		-lpthread 		\
		-lm
LIBDIR = -L/usr/lib64

CXXFLAGS = -Wall -g -O2 -m64 -D_REENTRANT -D_THREAD_SAFE	
NETWORK_OBJS = ListenSocket.o SessionThread.o SocketThread.o SessionSocket.o ConnectSocket.o
STREAM_OBJS = StreamThread.o StreamSource.o ObjectManager.o Demuxer.o Muxer.o Decoder.o Encoder.o Codec.o HWAccel.o
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
	$(CXX) $(CXXFLAGS) $(LIBDIR) -o $@ $(TOTAL_OBJS) $(LIBS)
	
clean : 
	rm -rf $(TOTAL_OBJS) $(TARGET)
