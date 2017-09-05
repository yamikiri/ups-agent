#include "uartXferEngine.hpp"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[uartXferEngine]"
#include "log.h"

static uint8_t *RECV_BUFFER_POOL[RECV_BUFFER_QUEUE_LEN] = {0};

static uint8_t *CMD_BUFFER_POOL[CMD_BUFFER_QUEUE_LEN] = {0};

engineSetting gEngineSetting;

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

const char* printQueue(recv_unit* unit, bool show = true);

const char* printQueue(recv_unit* unit, bool show)
{
	static char out[RECV_BUFFER_SIZE*3] = {0};
	if (unit != NULL) {
		if (unit->filled) {
			int32_t offset = 0;

			for (int32_t j = 0; j < unit->content_len; j++)
				offset += sprintf(out + offset, "%02X ", unit->content[j]);
			if (show)
				LOGD("(%d): \"%s\"\n", unit->content_len, out);
			return out;
		} else
			return NULL;
	} else
		return NULL;
}

void dumpQueue(std::vector<recv_unit>* queue)
{
	for (int32_t i = 0; i < queue->size(); i++) {
		if (queue->at(i).filled)
			LOGD("%d(%d): \"%s\"\n", i, queue->at(i).content_len, printQueue(&(queue->at(i)), false));
	}
}

bool initXferEngine(UartInterface* uart, PktParser pktParser, ContHandler contHandler)
{
	gEngineSetting.engineInited = false;
	circular_buffer<uint8_t> *t = NULL;
	if (!uart) {
		LOGE("uart == 0\n");
		return false;
	}

	if (uart->getFD() < 0) {
		LOGE("FD < 0\n");
		return false;
	}

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
	if (t == NULL) {
		LOGE("Can't create circular buffer!\n");
		goto REQ_CIRCULAR_BUFFER_FAILED;
	}
	//clean up data in uart buffer
	uint8_t clean[512];
	int32_t len;
	do {
		len = uart->read(clean, 512);
	} while(len > 0);

	gEngineSetting.term = uart;
	gEngineSetting.readerFD = uart->getFD();
	gEngineSetting.terminateReader = false;
	gEngineSetting.startWaitingPacket = true;
	// memset(gEngineSetting.recvHeader, 0x0, sizeof(gEngineSetting.recvHeader));
	// gEngineSetting.recvHeader_len = 0;
	// memset(gEngineSetting.recvFooter, 0x0, sizeof(gEngineSetting.recvFooter));
	// gEngineSetting.recvFooter[0] = 0; // ups terminal
	// gEngineSetting.recvFooter_len = 1;
	// gEngineSetting.recvFooter[0] = 'B'; // ups terminal
	// gEngineSetting.recvFooter[1] = '\r'; // ups terminal
	// gEngineSetting.recvFooter_len = 2;
	gEngineSetting.recvQueue.clear();
	for(int32_t i = 0; i < RECV_BUFFER_QUEUE_LEN; i++) {
		recv_unit temp;
		temp.filled = false;
		temp.content = RECV_BUFFER_POOL[i];
		temp.content_len = 0;
		gEngineSetting.recvQueue.push_back(temp);
	}
	gEngineSetting.queueInc = 0;
	gEngineSetting.preRecvBuffer = t;
	gEngineSetting.packetParser = pktParser;
	gEngineSetting.contentHandler = contHandler;
	gEngineSetting.engineInited = true;
	pthread_create(&(gEngineSetting.recvThread), NULL, reader_func, NULL);

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
		LOGD("\nDump recQueue:\n");
		dumpQueue(&(gEngineSetting.recvQueue));
		gEngineSetting.startWaitingPacket = false;
		gEngineSetting.terminateReader = true;
		pthread_join(gEngineSetting.recvThread, NULL);
		delete gEngineSetting.preRecvBuffer;
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
	const uint32_t SEC2TIMEOUT = 1;
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
					// LOGE("select timeout(%d sec)\n", SEC2TIMEOUT);
					/*if (++continueSelectTimeoutCnt > TIMEOUT_CNT_LIMIT) {
						LOGE("Timed out over %d times, stop waiting.\n", TIMEOUT_CNT_LIMIT);
						gEngineSetting.startWaitingPacket = false;
						continueSelectTimeoutCnt = 0;
						// pLocalBuffer = &(localBuffer[0]);
						offset = 0;
					}*/
					continue;
				}
				continueSelectTimeoutCnt = 0;

				while(1) {
					//int32_t length = term->read(localBuffer + offset, BUFFER_SIZE - offset);
					int32_t llength = term->read(localBuffer, BUFFER_SIZE);
					if (llength > 0) {
						// offset += length;
						int32_t idx = 0;
						while(llength-- > 0) {
							if (gEngineSetting.preRecvBuffer->full()) {
								offset = idx;
								LOGI("circular_buffer full!\n");
								goto FORCE_OUT;
							}
							// LOGD("llength: %d, idx: %d, char: 0x%02X\n", llength, idx, localBuffer[idx]);
							gEngineSetting.preRecvBuffer->put(localBuffer[idx++]);
						};
						//offset = length;
					} else
						break;
				};
				FORCE_OUT:
				uint8_t *peekCB = NULL;
				int32_t wlen = gEngineSetting.preRecvBuffer->peek(&peekCB);
				if (wlen == 0)
					continue;

				int32_t startIdx = -1;
				int32_t startTokenLen = 0;
				int32_t endIdx = -1;
				int32_t endTokenLen = 0;
				int32_t idx = -1;
#if 0
				if (gEngineSetting.recvHeader_len > 0) {
					// TODO: check header, if found, set startIdx
				} else {
					startIdx = 0;
				}

				if (gEngineSetting.recvFooter_len > 0) {
					for(int32_t i = 0; i <= wlen - gEngineSetting.recvFooter_len; i++) {
						if (strncmp((const char *)(peekCB + i), (const char *)gEngineSetting.recvFooter, gEngineSetting.recvFooter_len) == 0) {
							endIdx = i;
							break;
						}
					}
				}
#else
				if (gEngineSetting.packetParser != NULL) {
					gEngineSetting.packetParser(peekCB, wlen, startIdx, startTokenLen, endIdx, endTokenLen);
				}
#endif
				if (endIdx == -1) {
					// LOGE("no packet in window(size:%d):%s\n", wlen, peekCB);
					if (gEngineSetting.preRecvBuffer->full()) {
						LOGE("full, clean up buffer\n");
						for(int32_t i = 0; i < wlen; i++)
							gEngineSetting.preRecvBuffer->get();
					}
				} else if(endIdx > 0) {
					if (endIdx > RECV_BUFFER_SIZE) {
						LOGI("data more than expected packet length, data will be truncate.\n");
						startIdx = endIdx - RECV_BUFFER_SIZE;
					}
					idx = getEmptySlot(&(gEngineSetting.recvQueue));
					if (idx == -1) {
						idx = gEngineSetting.queueInc % RECV_BUFFER_QUEUE_LEN;
						LOGI("QUEUE already full, replace index:%d\n", idx);
					}
					gEngineSetting.recvQueue.at(idx).content_len = endIdx - (startIdx + startTokenLen);
					memcpy(gEngineSetting.recvQueue.at(idx).content, peekCB + startIdx + startTokenLen,
					gEngineSetting.recvQueue.at(idx).content_len);
					gEngineSetting.recvQueue.at(idx).filled = true;

					printQueue(&(gEngineSetting.recvQueue.at(idx)), true);

					for(int32_t i = 0;
						i < (endIdx + endTokenLen);
						i++)
						gEngineSetting.preRecvBuffer->get();

					gEngineSetting.queueInc++;
				} else { //endIdx == 0, ignore
					LOGD("endIdx == 0\n");
					continue;
				}

				// TODO: ?
				if (gEngineSetting.contentHandler != NULL) {
					gEngineSetting.contentHandler(&(gEngineSetting.recvQueue.at(idx)));
				}
			} else {
				gEngineSetting.startWaitingPacket = false;
				LOGE("readerFD error! stop receiving data\n");
				// pLocalBuffer = &(localBuffer[0]);
				offset = 0;
			}
		}
		sleep(1);
	}
	pthread_exit(NULL);
	return arg;
}
