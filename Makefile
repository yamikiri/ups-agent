# NDK_STD_ROOT=/opt/ndk-standalone-arm-api-21
# NDK_STD_TOOLCHAIN=$(NDK_STD_ROOT)/bin


# PATH=$(NDK_STD_TOOLCHAIN):/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
# CPP=$(NDK_STD_TOOLCHAIN)/arm-linux-androideabi-g++
# CC=$(NDK_STD_TOOLCHAIN)/arm-linux-androideabi-gcc
# LIBRARY_PATH=$(NDK_STD_ROOT)/sysroot/usr/lib
CPP=g++
CC=gcc

# v4l2-xu.o: v4l2-xu.hpp v4l2-xu.cpp
# 		$(CPP) -g -DNO_CVLIB -fPIE -c v4l2-xu.cpp -o out/v4l2-xu.o

# main.o: bcv_common.hpp camerathreads.hpp camerathreadbase.hpp main.cpp
# 		$(CPP) -g -DNO_CVLIB -fPIE -c main.cpp -o out/main.o

# camerathreads.o: bcv_common.hpp camerathreads.hpp camerathreadbase.hpp camerathreads.cpp
# 		$(CPP) -g -DNO_CVLIB -fPIE -c camerathreads.cpp -o out/camerathreads.o

# camerathreadbase.o: bcv_common.hpp camerathreadbase.hpp camerathreadbase.cpp
# 		$(CPP) -g -DNO_CVLIB -fPIE -c camerathreadbase.cpp -o out/camerathreadbase.o

uartinterface.o: log.h uartinterface.hpp uartinterface.cpp
		$(CPP) -g -DOS_UBUNTU -c uartinterface.cpp -o out/uartinterface.o

uartReceiver.o: log.h uartXferEngine.hpp circular_buffer.hpp uartinterface.hpp uartReceiver.cpp
		$(CPP) -g -DOS_UBUNTU -std=c++11 -c uartReceiver.cpp -o out/uartReceiver.o

main.o: log.h uartXferEngine.hpp uartinterface.hpp main.cpp 
		$(CPP) -g -DOS_UBUNTU -std=c++11 -c main.cpp -o out/main.o

# mcuprocessor.o: bcv_common.hpp uartinterface.hpp mcuprocessor.hpp mcuprocessor.cpp
# 		$(CPP) -g -DNO_CVLIB -fPIE -c mcuprocessor.cpp -o out/mcuprocessor.o

# v4l2: main.o camerathreadbase.o camerathreads.o mcuprocessor.o uartinterface.o
# 		$(CPP)  -fPIE -pie out/main.o out/camerathreads.o out/camerathreadbase.o out/mcuprocessor.o out/v4l2-xu.o out/uartinterface.o out/bcv_file.o -llog -o out/v4l2

ups: main.o uartinterface.o uartReceiver.o
		$(CPP) -g out/main.o out/uartinterface.o out/uartReceiver.o -lpthread -o out/ups

out:
	mkdir -p out

all: out ups

clean:
	rm -rf out
