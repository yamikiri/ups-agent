#include "uartXferEngine.hpp"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[uartXferEngine]"
#include "log.h"

int32_t getEmptySlot(std::vector<cmd_unit>* queue)
{
	int32_t idx = -1;
	for (int32_t i = 0; i < queue->size(); i++) {
		if (queue->at(i).processed) {
			idx = i;
			break;
		}
	}
	return idx;
}

const char* printQueue(cmd_unit* unit, bool show)
{
	static char out[CMD_BUFFER_SIZE*3] = {0};
	if (unit != NULL) {
		if (unit->cmd_len > 0) {
			if (!gEngineSetting.logTextMode) {
				int32_t offset = 0;

				for (int32_t j = 0; j < unit->cmd_len; j++)
					offset += sprintf(out + offset, "%02X ", unit->cmd[j]);
				if (show)
					LOGD("(%d): \"%s\"\n", unit->cmd_len, out);
				return out;
			} else {
				if (show)
					LOGD("(%d): \"%s\"\n", unit->cmd_len, unit->cmd);
				return (const char*)unit->cmd;
			}
		} else
			return NULL;
	} else
		return NULL;
}

void dumpQueue(std::vector<cmd_unit>* queue)
{
	for (int32_t i = 0; i < queue->size(); i++) {
		if (queue->at(i).cmd_len > 0)
			LOGD("%d(%d): \"%s\"\n", i, queue->at(i).cmd_len, printQueue(&(queue->at(i)), false));
	}
}

void* writer_func(void* arg)
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
	while(!gEngineSetting.terminateWriter) {
		while(gEngineSetting.startSendingPacket) {
			if (gEngineSetting.readerFD >= 0) {
				
			} else {
				gEngineSetting.startSendingPacket = false;
				LOGE("writerFD error! stop sending command\n");
				// pLocalBuffer = &(localBuffer[0]);
				offset = 0;
			}
		}
		sleep(1);
	}
	pthread_exit(NULL);
	return arg;
}
