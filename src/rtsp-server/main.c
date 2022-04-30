#include "cdk.h"
#include "rtsp-common/rtsp-common.h"

#define SERVER_IP    "0.0.0.0"
#define RTSP_PORT    "554"

void handle_rtsp(sock_t s) {

	net_msg_t* msg;
	msg = cdk_tcp_recv(s);

	switch (msg->h.p_t) {
	case TYPE_RTSP_OPTIONS:
		break;
	case TYPE_RTSP_DESCRIBE:
		break;
	case TYPE_RTSP_SETUP:
		break;
	case TYPE_RTSP_PLAY:
		break;
	case TYPE_RTSP_TEARDOWN:
		break;
	default:
		break;
	}
}

int main(void) {

	sock_t sfd;

	cdk_logger_init(NULL, true);

	sfd = cdk_tcp_listen(SERVER_IP, RTSP_PORT);
	cdk_tcp_netpoller(sfd, handle_rtsp, false);


	cdk_logger_destroy();
	return 0;
}