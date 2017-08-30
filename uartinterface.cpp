#include "uartinterface.hpp"

UartInterface::UartInterface(const char* devPath)
	 : mInited(false), mBaudrate(B9600), mTermFD(-1), mProtocolLock(false)
{
	int32_t result = 0;
	mTermFD = open(devPath, O_RDWR | O_NOCTTY);
	//mTermFD = open(devPath, O_RDWR );
	if (mTermFD < 0) {
		LOGE("%s open failed.\n", devPath);
		mTermFD = -1;
		return;
	}
	//memset(buf, 0x00, sizeof(buf));
	bzero(&mTermIOSettings, sizeof(mTermIOSettings));
	bzero(&mTermSaio, sizeof(mTermSaio));

	mTermIOSettings.c_cflag = mBaudrate | mCFlags;
	mTermIOSettings.c_iflag = mIFlags;
	mTermIOSettings.c_oflag = mOFlags;
	mTermIOSettings.c_lflag = mLFlags;

	mTermIOSettings.c_cc[VINTR]		= mCC_VINTR;
	mTermIOSettings.c_cc[VQUIT]		= mCC_VQUIT;
	mTermIOSettings.c_cc[VERASE]	= mCC_VERASE;
	mTermIOSettings.c_cc[VKILL]		= mCC_VKILL;
	mTermIOSettings.c_cc[VEOF]		= mCC_VEOF;
	mTermIOSettings.c_cc[VTIME]		= mCC_VTIME;
	mTermIOSettings.c_cc[VMIN]		= mCC_VMIN;
	mTermIOSettings.c_cc[VSWTC]		= mCC_VSWTC;
	mTermIOSettings.c_cc[VSTART]	= mCC_VSTART;
	mTermIOSettings.c_cc[VSTOP]		= mCC_VSTOP;
	mTermIOSettings.c_cc[VSUSP]		= mCC_VSUSP;
	mTermIOSettings.c_cc[VEOL]		= mCC_VEOL;
	mTermIOSettings.c_cc[VREPRINT]	= mCC_VREPRINT;
	mTermIOSettings.c_cc[VDISCARD]	= mCC_VDISCARD;
	mTermIOSettings.c_cc[VWERASE]	= mCC_VWERASE;
	mTermIOSettings.c_cc[VLNEXT]	= mCC_VLNEXT;
	mTermIOSettings.c_cc[VEOL2]		= mCC_VEOL2;
}

UartInterface::~UartInterface()
{
	if (mTermFD >= 0) {

		if (mInited) {
			while(mProtocolLock) ;
			tcsetattr(mTermFD, TCSANOW, &mOriginalTermIOSettings);
		}
		::close(mTermFD);
	}
}

bool UartInterface::init()
{
	if (mTermFD >= 0) {
		if (mInited)
			return false;
		else {
#if 0
	/* Configure device */
	{
		struct termios cfg;
		LOGD("Configuring serial port");
		if (tcgetattr(mTermFD, &cfg))
		{
			LOGE("tcgetattr() failed");
			close(mTermFD);
			/* TODO: throw an exception */
			return false;
		}

		cfmakeraw(&cfg);
		cfsetispeed(&cfg, B9600);
		cfsetospeed(&cfg, B9600);

		if (tcsetattr(mTermFD, TCSANOW, &cfg))
		{
			LOGE("tcsetattr() failed");
			close(mTermFD);
			/* TODO: throw an exception */
			return false;
		}
	}

#else
			tcgetattr(mTermFD, &mOriginalTermIOSettings);
			tcflush(mTermFD, TCIFLUSH);
			tcsetattr(mTermFD, TCSANOW, &mTermIOSettings);
			int flags = fcntl(mTermFD, F_GETFL, 0);
			fcntl(mTermFD, F_SETFL, flags | O_NONBLOCK);
#endif
			mInited = true;
			return true;
		}
	}
	return false;
}

bool UartInterface::setBaudRate(uint32_t rate)
{
	if (mInited) {
		if (rate != 115200 &&
			rate != 57600 &&
			rate != 38400 &&
			rate != 19200 &&
			rate != 9600 &&
			rate != 4800 &&
			rate != 2400
			) {
			LOGE("Baudrate 0x%X not support!\n", rate);
			return false;
		}
		switch(rate) {
		case 115200: mBaudrate = B115200; break;
		case 57600: mBaudrate = B57600; break;
		case 38400: mBaudrate = B38400; break;
		case 19200: mBaudrate = B19200; break;
		case 9600: mBaudrate = B9600; break;
		case 4800: mBaudrate = B4800; break;
		case 2400: mBaudrate = B2400; break;
		}

		mTermIOSettings.c_cflag = mBaudrate | mCFlags;

		tryLockUart(__func__);

		tcflush(mTermFD, TCIFLUSH);
		tcsetattr(mTermFD, TCSANOW, &mTermIOSettings);

		unlockUart();
		return true;
	}
ERROR_END(__func__):
	return false;
}

int32_t UartInterface::write(uint8_t* from, size_t size)
{
	int32_t ret = 0;
	if (!mInited) {
		LOGE("Terminal not inited.\n");
		goto ERROR_END(__func__);
	}

	if (from == NULL) {
		LOGE("Target write pointer is NULL!\n");
		goto ERROR_END(__func__);
	}

	tryLockUart(__func__);

	ret = ::write(mTermFD, from, size);

	unlockUart();

	return ret;

ERROR_END(__func__):
	return -1;
}

void UartInterface::flush()
{
	if (!mInited) {
		LOGE("Terminal not inited.\n");
		return;
	}

	tryLockUart(__func__);

	tcflush(mTermFD, TCIFLUSH);

	unlockUart();

	return;

ERROR_END(__func__):
	LOGE("Flush failed.\n");
}

int32_t UartInterface::read(uint8_t* to, size_t size)
{
	int32_t ret = -1;
	if (!mInited) {
		LOGE("Terminal not inited.\n");
		return -1;
	}

	if (to == NULL) {
		LOGE("Target read pointer is NULL!\n");
		return -1;
	}
#if 0
	volatile bool flag = false;
	int32_t result = 0;
	int32_t total = 0;
	if (size > MAX_BUFFER_SIZE) {
		LOGD("Request receive buffer %d too large.\n", size);
		return -1;
	}

	while (flag == false) {
		result = read(mTermFD, buf, MAX_BUFFER_SIZE);
		if (result < 0) {
			LOGE("Read error: 0x%X\n", result);
			return -1;
		}
		total += result;
		//buf[result] = 0;
		//LOGD("Data received:%s:%d\n", buf, res);
		if (total == size)
			break;
	}
	return total;
#else
	tryLockUart(__func__);

	ret = ::read(mTermFD, to, size);

	unlockUart();

ERROR_END(__func__):
	return ret;
#endif
}

void UartInterface::setReaderCallback(void (*cb)(int))
{
	if (!mInited) {
		LOGE("Terminal not inited.\n");
		return;
	}

	if (cb == NULL) {
		LOGE("Target callback pointer is NULL!\n");
		return;
	}

	mTermSaio.sa_handler = cb;
	//mTermSaio.sa_mask = 0;
	mTermSaio.sa_flags = 0;
//	mTermSaio.sa_restorer = NULL;
	sigaction(SIGIO, &mTermSaio, NULL);

	fcntl(mTermFD, F_SETOWN, getpid());
	fcntl(mTermFD, F_SETFL, FASYNC);
}
