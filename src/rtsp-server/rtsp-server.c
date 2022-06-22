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

	char*      msg;
	bool       ret;

	while (true) {

		msg = rtsp_recv_msg(s);
		if (!msg) {
			cdk_loge("network error.\n");
			return;
		}
	}
}