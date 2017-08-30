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
