#include "cdk.h"
#include "rtsp-client/rtsp-client.h"

int main(void) {

	cdk_logger_init(NULL, true);

	rtsp_handshake();

	cdk_logger_destroy();
	return 0;
}