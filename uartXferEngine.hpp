#ifndef __UARTXFERENGINE_HPP
#define __UARTXFERENGINE_HPP

#include "circular_buffer.hpp"
#include "uartinterface.hpp"
#include <vector>

#define _QUEUE_LENGTH		 (16)

#define RECV_BUFFER_SIZE	 (128)
#define RECV_BUFFER_QUEUE_LEN _QUEUE_LENGTH
// extern uint8_t *RECV_BUFFER_POOL[RECV_BUFFER_QUEUE_LEN];

#define CMD_BUFFER_SIZE		 (16)
#define CMD_BUFFER_QUEUE_LEN _QUEUE_LENGTH
// extern uint8_t *CMD_BUFFER_POOL[CMD_BUFFER_QUEUE_LEN];

typedef struct RECV_UNIT {
	bool filled;
	struct timeval recvTS;
	uint8_t *content;// receive packet content
	int32_t content_len;// receive packet content length, fill by receiver
} recv_unit;

typedef bool (*ResultChecker)(void* private_data);

typedef struct CMD_UNIT {
	uint8_t *cmd;
	int32_t cmd_len;
	pthread_mutex_t inProcessingLock;
	bool processed;
	struct timeval issueTS;
	unsigned delay;// us
	ResultChecker resultChecker;// for Q&A type protocol
} cmd_unit;

typedef void (*PktParser)(uint8_t *str, int32_t len, int32_t& startIdx, int32_t& slen, int32_t& endIdx, int32_t& elen);
typedef void (*ContHandler)(recv_unit *);// for notification type protocol

typedef struct ENGINE_SETTING {
	bool engineInited;
	UartInterface *term;
	int32_t readerFD;
	volatile bool terminateWriter;
	volatile bool startSendingPacket;
	volatile bool terminateReader;
	volatile bool startWaitingPacket;
	bool logTextMode; // false: default hex mode, otherwise text mode
	pthread_t cmdThread;
	std::vector<cmd_unit> cmdQueue;

	pthread_t recvThread;
	// uint8_t recvHeader[8];// receive packet header magic, filled at init
	// int32_t recvHeader_len;// receive header length, 0 means no header, filled at init
	// uint8_t recvFooter[8];// receive packet footer magic, null means none, filled at init
	// int32_t recvFooter_len;// receive packet footer length, 0 means no footer, filled at init, usually have footer
	PktParser packetParser;
	ContHandler contentHandler;
	std::vector<recv_unit> recvQueue;
	uint32_t queueInc;
	circular_buffer<uint8_t> *preRecvBuffer;
} engineSetting;

extern engineSetting gEngineSetting;

void* reader_func(void* arg);

void* writer_func(void* arg);

const char* printQueue(cmd_unit* unit, bool show = true);

const char* printQueue(recv_unit* unit, bool show = true);

void dumpQueue(std::vector<cmd_unit>* queue);

void dumpQueue(std::vector<recv_unit>* queue);

bool queueCommand(uint8_t *cmd, uint32_t cmd_len, ResultChecker rsChk = NULL);

bool initXferEngine(UartInterface* uart, bool isLogText = false, PktParser pktPasrer = NULL, ContHandler contHandler = NULL);

void deinitXferEngine();

#endif