#include "rtsp-server/rtsp-server.h"

int main(void) {

	cdk_logger_init(NULL, true);
	run_rtspserver();
	cdk_logger_destroy();
	return 0;
}