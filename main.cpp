#include "uartinterface.hpp"
#include "circular_buffer.hpp"
#include "uartXferEngine.hpp"
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
	AVR_VOLTAGE,
	CMD_AMOUNT
};


int main(int32_t argc, char** argv)
{
	UartInterface *upsCom = new UartInterface(UPS_COM);
	if (!upsCom) {
		LOGE("failed to open %s!\n", UPS_COM);
		return -1;
	}
	upsCom->init();
	upsCom->setBaudRate(2400);

	if (!initXferEngine(upsCom)) {
		LOGE("Xfer engine init failed\n");
		return -1;
	}

	// usleep(10000);
	char query_status[] = { 'D', 'Q', '1', '\r' };
	upsCom->write((uint8_t *)query_status, 4);
	upsCom->flush();
	// char readBuffer[500] = {0};
	sleep(1);
	// upsCom->read((uint8_t *)readBuffer, 55);
	// LOGI("%s\n", readBuffer);

	for (int j = 0; j < 10; j++) {
		for (int i = 0; i < 2; i++) {
			if (ups_cmd_table[i][strlen(ups_cmd_table[i])-2] == '%')
				continue;
			upsCom->write((uint8_t*)ups_cmd_table[i], strlen(ups_cmd_table[i]));
			upsCom->flush();
			sleep(1);
		}
	}
	// upsCom->read((uint8_t *)readBuffer, 55);
	// LOGI("%s\n", readBuffer);

	deinitXferEngine();
	delete upsCom;
	return 0;
}
