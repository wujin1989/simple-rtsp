#include "rtsp-server/rtsp-server.h"
#include "rtsp-common/rtsp-common.h"

void handle_rtsp(sock_t s) {

	net_msg_t* msg;
	msg = cdk_tcp_recv(s);

	switch (msg->h.p_t) {
	case TYPE_OPTIONS_REQ:
		break;
	case TYPE_DESCRIBE_REQ:
		break;
	case TYPE_SETUP_REQ:
		break;
	case TYPE_PLAY_REQ:
		break;
	case TYPE_TEARDOWN_REQ:
		break;
	default:
		break;
	}
}