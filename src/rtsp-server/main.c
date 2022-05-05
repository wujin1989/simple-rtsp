#include "rtsp-server/rtsp-server.h"

#define SERVER_IP    "0.0.0.0"
#define RTSP_PORT    "554"

int main(void) {

	sock_t sfd;

	cdk_logger_init(NULL, true);

	sfd = cdk_tcp_listen(SERVER_IP, RTSP_PORT);
	cdk_tcp_netpoller(sfd, handle_rtsp, false);


	cdk_logger_destroy();
	return 0;
}