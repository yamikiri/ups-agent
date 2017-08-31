#include "uartinterface.hpp"
#include "circular_buffer.hpp"
#include <vector>

#define UPS_COM "/dev/ttyUSB0"

static char* ups_cmd_table[] = {
	"Q1\r",		// Status inqury
	"DQ1\r",	// Status inqury
	"T\r",		// 10 sec test
	"TL\r",		// test until battery low
	"T%%\r",	// test for specified time period
	"Q\r",		// turn on/off beep
	"S%%\r",	// shutdown command
	"S%%R%%%%\r",	// shutdown and restore command
	"C\r",		// cancel shutdown command
	"CT\r",		// cancel test command
	"I\r",		// UPS information command
	"F\r",		// UPS rating information
	"V\r"		// display AVR voltage
};

enum ups_cmd {
	QUERY = 0,
	QUERY1,
	TEST_10S,
	TEST_TILL_BATTERY_LOW,
	TEST_FOR_SPECIFIED_MIN,
	TOGGLE_BEEP,
	SHUTDOWN_BY_MIN,
	SHUDOWN_N_RESTORE_BY_MIN,
	CANCEL_SHUTDOWN,
	CANCEL_TEST,
	UPS_INFO,
	RATING_INFO,
	AVR_VOLTAGE
};

#define _QUEUE_LENGTH		 (16)

#define RECV_BUFFER_SIZE	 (128)
#define RECV_BUFFER_QUEUE_LEN _QUEUE_LENGTH
static uint8_t *RECV_BUFFER_POOL[RECV_BUFFER_QUEUE_LEN] = {0};

#define CMD_BUFFER_SIZE		 (16)
#define CMD_BUFFER_QUEUE_LEN _QUEUE_LENGTH
static uint8_t *CMD_BUFFER_POOL[CMD_BUFFER_QUEUE_LEN] = {0};

typedef struct RECV_UNIT {
	bool filled;
	uint8_t *content;// receive packet content
	int32_t content_len;// receive packet content length, fill by receiver
} recv_unit;

typedef struct CMD_UNIT {
	uint8_t *cmd;
	int32_t cmd_len;
	recv_unit *result;
	pthread_mutex_t lock;
	bool (*callback)(void*private_data);
} cmd_unit;

struct ENGINE_SETTING {
	bool engineInited;
	UartInterface *term;
	int32_t readerFD;
	volatile bool terminateReader;
	volatile bool startWaitingPacket;

	uint8_t recvHeader[8];// receive packet header magic, filled at init
	int32_t recvHeader_len;// receive header length, 0 means no header, filled at init
	uint8_t recvFooter[8];// receive packet footer magic, null means none, filled at init
	int32_t recvFooter_len;// receive packet footer length, 0 means no footer, filled at init, usually have footer
	std::vector<recv_unit> recvQueue;
	circular_buffer<uint8_t> *preBuffer;
} gEngineSetting;

int32_t getEmptySlot(std::vector<recv_unit>* queue)
{
	int32_t idx = -1;
	for (int32_t i = 0; i < queue->size(); i++) {
		if (!queue->at(i).filled) {
			idx = i;
			break;
		}
	}
	return idx;
}

void dumpQueue(std::vector<recv_unit>* queue)
{
	for (int32_t i = 0; i < queue->size(); i++) {
		if (queue->at(i).filled) {
			LOGD("%d(%d): \"%s\"\n", i, queue->at(i).content_len, (char *)queue->at(i).content);
		}
	}
}

bool initXferEngine(UartInterface* uart)
{
	gEngineSetting.engineInited = false;
	circular_buffer<uint8_t> *t = NULL;
	if (uart <= 0)
		return false;

	if (uart->getFD() < 0)
		return false;

	RECV_BUFFER_POOL[0] = (uint8_t *)malloc(RECV_BUFFER_SIZE*RECV_BUFFER_QUEUE_LEN);
	if (RECV_BUFFER_POOL[0] == NULL) {
		LOGE("Can't acquire CMD recieve buffer\n");
		goto REQ_RECV_BUFFER_POOL_FAILED;
	}
	for (uint32_t i = 0; i < RECV_BUFFER_QUEUE_LEN; i++) {
		RECV_BUFFER_POOL[i] = RECV_BUFFER_POOL[0] + RECV_BUFFER_SIZE*i;
	}
	memset(RECV_BUFFER_POOL[0], 0x0, RECV_BUFFER_SIZE*RECV_BUFFER_QUEUE_LEN);

	CMD_BUFFER_POOL[0] = (uint8_t *)malloc(CMD_BUFFER_SIZE*CMD_BUFFER_QUEUE_LEN);
	if (CMD_BUFFER_POOL[0] == NULL) {
		LOGE("Can't acquire CMD recieve buffer\n");
		goto REQ_CMD_BUFFER_POOL_FAILED;
	}
	for (uint32_t i = 0; i < CMD_BUFFER_QUEUE_LEN; i++) {
		CMD_BUFFER_POOL[i] = CMD_BUFFER_POOL[0] + CMD_BUFFER_SIZE*i;
	}
	memset(CMD_BUFFER_POOL[0], 0x0, CMD_BUFFER_SIZE*CMD_BUFFER_QUEUE_LEN);

	t = new circular_buffer<uint8_t>(RECV_BUFFER_SIZE*8);
	if (t == NULL)
		goto REQ_CIRCULAR_BUFFER_FAILED;

	gEngineSetting.term = uart;
	gEngineSetting.readerFD = uart->getFD();
	gEngineSetting.terminateReader = false;
	gEngineSetting.startWaitingPacket = false;
	memset(gEngineSetting.recvHeader, 0x0, sizeof(gEngineSetting.recvHeader));
	gEngineSetting.recvHeader_len = 0;
	memset(gEngineSetting.recvFooter, 0x0, sizeof(gEngineSetting.recvFooter));
	gEngineSetting.recvFooter[0] = '\r'; // ups terminal
	gEngineSetting.recvFooter_len = 1;
	gEngineSetting.recvQueue.clear();
	for(int32_t i = 0; i < RECV_BUFFER_QUEUE_LEN; i++) {
		recv_unit temp;
		temp.filled = false;
		temp.content = RECV_BUFFER_POOL[i];
		temp.content_len = 0;
		gEngineSetting.recvQueue.push_back(temp);
	}
	gEngineSetting.preBuffer = t;
	gEngineSetting.engineInited = true;

	return true;
REQ_CIRCULAR_BUFFER_FAILED:
	free(CMD_BUFFER_POOL[0]);
	CMD_BUFFER_POOL[0] = NULL;
REQ_CMD_BUFFER_POOL_FAILED:
	free(RECV_BUFFER_POOL[0]);
	RECV_BUFFER_POOL[0] = NULL;
REQ_RECV_BUFFER_POOL_FAILED:
	return false;
}

void deinitXferEngine()
{
	if (gEngineSetting.engineInited) {
		delete gEngineSetting.preBuffer;
		free(CMD_BUFFER_POOL[0]);
		CMD_BUFFER_POOL[0] = NULL;
		free(RECV_BUFFER_POOL[0]);
		RECV_BUFFER_POOL[0] = NULL;
		gEngineSetting.readerFD = -1;
		gEngineSetting.term = NULL;
	}
}

void* reader_func(void* arg)
{
	const uint32_t SEC2TIMEOUT = 2;
	const uint32_t TIMEOUT_CNT_LIMIT = 10;
	const uint32_t BUFFER_SIZE = RECV_BUFFER_SIZE;
	fd_set fds;
	struct timeval tv = {0};
	int32_t result;
	uint32_t continueSelectTimeoutCnt = 0;
	uint8_t localBuffer[BUFFER_SIZE] = {0};
	// uint8_t *pLocalBuffer = &(localBuffer[0]);
	int32_t offset = 0;
	UartInterface *term = gEngineSetting.term;
	while(!gEngineSetting.terminateReader) {
		while(gEngineSetting.startWaitingPacket) {
			if (gEngineSetting.readerFD >= 0) {
				FD_ZERO(&fds);
				FD_SET(gEngineSetting.readerFD, &fds);
				tv.tv_sec = SEC2TIMEOUT;
				tv.tv_usec = 0;
				result = select(gEngineSetting.readerFD + 1, &fds, NULL, NULL, &tv);
				if (result == -1) {
					LOGD("select error: 0x%08X\n", errno);
					if (errno != EINTR)	{
						LOGD("Not allowed error, stopping\n");
						gEngineSetting.startWaitingPacket = false;
						// pLocalBuffer = &(localBuffer[0]);
						offset = 0;
					}
					continue;
				} else if (result == 0) {
					LOGE("select timeout(%d sec)\n", SEC2TIMEOUT);
					if (++continueSelectTimeoutCnt > TIMEOUT_CNT_LIMIT) {
						LOGE("Timed out over %d times, stop waiting.\n", TIMEOUT_CNT_LIMIT);
						gEngineSetting.startWaitingPacket = false;
						continueSelectTimeoutCnt = 0;
						// pLocalBuffer = &(localBuffer[0]);
						offset = 0;
					}
					continue;
				}
				continueSelectTimeoutCnt = 0;

				while(1) {
					int32_t length = term->read(localBuffer + offset, BUFFER_SIZE - offset);
					if (length > 0) {
						// offset += length;
						int32_t idx = 0;
						while(length-- > 0) {
							gEngineSetting.preBuffer->put(localBuffer[idx++]);
							if (gEngineSetting.preBuffer->full()) {
								offset = idx;
								LOGI("circular_buffer full!\n");
								goto FORCE_OUT;
							}
						};
					} else
						break;
				};
				FORCE_OUT:
				uint8_t *peekCB = NULL;
				int32_t wlen = gEngineSetting.preBuffer->peek(&peekCB);
				if (wlen == 0)
					continue;

				int32_t startIdx = -1;
				if (gEngineSetting.recvHeader_len > 0) {
					// TODO: check header, if found, set startIdx
				} else {
					startIdx = 0;
				}

				int32_t endIdx = -1;
				for(int32_t i = 0; i < wlen - gEngineSetting.recvFooter_len ; i++) {
					if (strncmp((const char *)(peekCB + i), (const char *)gEngineSetting.recvFooter, gEngineSetting.recvFooter_len) == 0) {
						endIdx = i;
						break;
					}
				}
				if (endIdx == -1) {
					LOGE("no packet in window(size:%d)\n", wlen);
					if (gEngineSetting.preBuffer->full()) {
						LOGE("full, clean up buffer\n");
						for(int32_t i = 0; i < wlen; i++)
							gEngineSetting.preBuffer->get();
					}
				} else if(endIdx > 0) {
					if (endIdx > RECV_BUFFER_SIZE) {
						LOGI("data more than expected packet length, data will be truncate.\n");
						startIdx = endIdx - RECV_BUFFER_SIZE;
					}
					int32_t idx = getEmptySlot(&(gEngineSetting.recvQueue));
					if (idx != -1) {
						gEngineSetting.recvQueue.at(idx).content_len = endIdx - startIdx + 1;
						memcpy(gEngineSetting.recvQueue.at(idx).content, peekCB + startIdx,
						gEngineSetting.recvQueue.at(idx).content_len);
						gEngineSetting.recvQueue.at(idx).filled = true;
					}
					LOGD("%d(%d): \"%s\"\n", idx, gEngineSetting.recvQueue.at(idx).content_len,
					 (char *)gEngineSetting.recvQueue.at(idx).content);
				} else { //endIdx == 0, ignore
				}

				// TODO: ?
			} else {
				gEngineSetting.startWaitingPacket = false;
				LOGE("readerFD error! stop receiving data\n");
				// pLocalBuffer = &(localBuffer[0]);
				offset = 0;
			}
		}
		sleep(1);
	}
	return arg;
}

int main(int32_t argc, char** argv)
{
	UartInterface *upsCom = new UartInterface(UPS_COM);
	if (!upsCom) {
		LOGE("failed to open %s!\n", UPS_COM);
		return -1;
	}
	// upsCom->setBaudRate(B2400);
	upsCom->init();
	upsCom->setBaudRate(2400);

	if (initXferEngine(upsCom)) {
		return -1;
	}
	pthread_t recvThread;
	pthread_create(&recvThread, NULL, reader_func, NULL);

	// usleep(10000);
	char query_status[] = { 'D', 'Q', '1', '\r' };
	upsCom->write((uint8_t *)query_status, 4);
	upsCom->flush();
	// char readBuffer[500] = {0};
	usleep(1000);
	// upsCom->read((uint8_t *)readBuffer, 55);
	// LOGI("%s\n", readBuffer);

	upsCom->write((uint8_t*)ups_cmd_table[UPS_INFO], strlen(ups_cmd_table[UPS_INFO]));
	upsCom->flush();
	// usleep(1000000);
	// upsCom->read((uint8_t *)readBuffer, 55);
	// LOGI("%s\n", readBuffer);

	deinitXferEngine();
	delete upsCom;
	return 0;
}
