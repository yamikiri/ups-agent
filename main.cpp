#include "uartinterface.hpp"

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

#define QUEUE_LENGTH		 16

#define RECV_BUFFER_SIZE	 128
#define RECV_BUFFER_QUEUE_LEN QUEUE_LENGTH
static uint8_t *RECV_BUFFER_POOL[RECV_BUFFER_QUEUE_LEN] = {0};

#define CMD_BUFFER_SIZE		 16
#define CMD_BUFFER_QUEUE_LEN QUEUE_LENGTH
static uint8_t *CMD_BUFFER_POOL[CMD_BUFFER_QUEUE_LEN] = {0};

typedef struct CMD_UNIT {
	uint8_t *cmd;
	int32_t cmd_len;
	uint8_t *result;
	int32_t result_len;
	bool processed;
	bool (*callback)(void*private_data);
} cmd_unit;

struct READER_SETTING {
	int32_t readerFD;
	volatile bool terminateReader;
	volatile bool startWaitingCmdResult;
} gReaderSetting;

bool initCmdEngine(void)
{
	RECV_BUFFER_POOL[0] = (uint8_t *)malloc(RECV_BUFFER_SIZE*RECV_BUFFER_QUEUE_LEN);
	if (RECV_BUFFER_POOL[0] == NULL) {
		LOGE("Can't acquire CMD recieve buffer\n");
		return false;
	}
	for (uint32_t i = 0; i < RECV_BUFFER_QUEUE_LEN; i++) {
		RECV_BUFFER_POOL[i] = RECV_BUFFER_POOL[0] + RECV_BUFFER_SIZE*i;
	}
	memset(RECV_BUFFER_POOL[0], 0x0, RECV_BUFFER_SIZE*RECV_BUFFER_QUEUE_LEN);

	CMD_BUFFER_POOL[0] = (uint8_t *)malloc(CMD_BUFFER_SIZE*CMD_BUFFER_QUEUE_LEN);
	if (CMD_BUFFER_POOL[0] == NULL) {
		LOGE("Can't acquire CMD recieve buffer\n");
		free(RECV_BUFFER_POOL[0]);
		RECV_BUFFER_POOL[0] = NULL;
		return false;
	}
	for (uint32_t i = 0; i < CMD_BUFFER_QUEUE_LEN; i++) {
		CMD_BUFFER_POOL[i] = CMD_BUFFER_POOL[0] + CMD_BUFFER_SIZE*i;
	}
	memset(CMD_BUFFER_POOL[0], 0x0, CMD_BUFFER_SIZE*CMD_BUFFER_QUEUE_LEN);

	gReaderSetting.readerFD = -1;
	gReaderSetting.terminateReader = false;
	gReaderSetting.startWaitingCmdResult = false;

	return true;
}

void* cmd_reader_func(void* arg)
{
	while(!gReaderSetting.terminateReader) {
		while(gReaderSetting.startWaitingCmdResult) {
			if (gReaderSetting.readerFD >= 0) {
				
			}
		}
		sleep(2);
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

	if (initCmdEngine()) {
		return -1;
	}

	usleep(10000);
	char query_status[] = { 'D', 'Q', '1', '\r' };
	upsCom->write((uint8_t *)query_status, 4);
	upsCom->flush();
	char readBuffer[500] = {0};
	usleep(1000000);
	upsCom->read((uint8_t *)readBuffer, 55);
	LOGI("%s\n", readBuffer);

	upsCom->write((uint8_t*)ups_cmd_table[UPS_INFO], strlen(ups_cmd_table[UPS_INFO]));
	usleep(1000000);
	upsCom->read((uint8_t *)readBuffer, 55);
	LOGI("%s\n", readBuffer);


	delete upsCom;
	return 0;
}
