#include "rtsp-server/rtsp-server.h"
#include "rtsp-common/rtsp-common.h"

static size_t _cseq;
static char   _uri[128];

static bool _parse_options(char* msg) {

	return true;
}

static bool _send_options_rsp(sock_t s) {

	return true;
}

void handle_rtsp(sock_t s) {

	net_msg_t* rmsg;
	char*      msg;
	bool       ret;

	while (true) {
		rmsg = cdk_tcp_recv(s);
		if (!rmsg) {
			cdk_loge("network error.\n");
			return;
		}
		msg = cdk_malloc(rmsg->h.p_s);

		switch (rmsg->h.p_t) {
		case TYPE_OPTIONS_REQ:
		{
			cdk_tcp_demarshaller(rmsg, msg);

			ret = _parse_options(msg);
			if (!ret) {
				cdk_loge("parse options failed.\n");
				cdk_free(msg);
				return;
			}
			cdk_free(msg);
			_send_options_rsp(s);
			break;
		}
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
}