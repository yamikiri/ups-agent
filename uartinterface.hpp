#pragma once

#ifndef __UARTINTERFACE_HPP
#define __UARTINTERFACE_HPP

// #include "bcv_common.hpp"
extern "C" {
	#include <dlfcn.h>
	#include <fcntl.h>
	#include <linux/hdreg.h>
	#include <pthread.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <strings.h>
	#include <sys/ioctl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/times.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <errno.h>
	#include <assert.h>
	#include <getopt.h>             /* getopt_long() */
	#include <stdint.h>
	#include <semaphore.h>
	#include <linux/videodev2.h>

	#include <termios.h>
	#include <signal.h>

#ifndef OS_UBUNTU
	#include <asm/termbits.h>
#endif
}

#include "log.h"

class UartInterface
{
#define ERROR_END(f) f##END

#define tryLockUart(func) do {\
	if (mProtocolLock)\
		goto func##END;\
	else\
		mProtocolLock = true;\
} while(0)

#define unlockUart() do {\
	mProtocolLock = false;\
} while(0)

public:
	UartInterface(const char* devPath);
	~UartInterface();
	bool init();
	bool setBaudRate(uint32_t rate);
	int32_t write(uint8_t* from, size_t size);
	int32_t read(uint8_t* to, size_t size);
	void flush();
	void setReaderCallback(void (*cb)(int));
	bool getLockState();
	int32_t getFD() { if(mInited) return mTermFD; else return -1; }
private:
	//const uint32_t MAX_BUFFER_SIZE = 127;
	//char buf[MAX_BUFFER_SIZE+1];
	int32_t mTermFD;
	struct termios mOriginalTermIOSettings;
	struct termios mTermIOSettings;
	struct sigaction mTermSaio;
	uint32_t mBaudrate;
	bool mInited;
	bool mProtocolLock;
	static const uint32_t mCFlags = CS8 | CLOCAL | CREAD;
	static const uint32_t mIFlags = IGNPAR;
	static const uint32_t mOFlags = 0;
	static const uint32_t mLFlags = 0;
	static const uint32_t mCC_VTIME = 0;
	static const uint32_t mCC_VMIN = 1;//minimal charaters for read() to return
	//optional
	static const uint32_t mCC_VINTR = 0;
	static const uint32_t mCC_VQUIT = 0;
	static const uint32_t mCC_VERASE = 0;
	static const uint32_t mCC_VKILL = 0;
	static const uint32_t mCC_VEOF = 4;
	static const uint32_t mCC_VSWTC = 0;
	static const uint32_t mCC_VSTART = 0;
	static const uint32_t mCC_VSTOP = 0;
	static const uint32_t mCC_VSUSP = 0;
	static const uint32_t mCC_VEOL = 0;
	static const uint32_t mCC_VREPRINT = 0;
	static const uint32_t mCC_VDISCARD = 0;
	static const uint32_t mCC_VWERASE = 0;
	static const uint32_t mCC_VLNEXT = 0;
	static const uint32_t mCC_VEOL2 = 0;
};

#endif
