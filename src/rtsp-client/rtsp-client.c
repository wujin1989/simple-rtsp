#include "cdk.h"
#include "rtsp-client/rtsp-client.h"
#include "rtsp-common/rtsp-common.h"

static size_t _cseq;
static char   _uri[128];

static bool _initialize_rtsp_req(rtsp_msg_t* req, char* method) {

	int ret;
	ret = cdk_sprintf(_uri, sizeof(_uri), "rtsp://%s:%s/live", RTSP_SERVER_ADDRESS, RTSP_PORT);
	if (ret < 0) {
		return false;
	}
	req->type           = TYPE_RTSP_REQ;
	req->ver            = "RTSP/1.0";
	req->msg.req.method = method;
	req->msg.req.uri    = _uri;
	
	cdk_list_create(&req->attrs);

	char cseq_str[32];
	ret = cdk_sprintf(cseq_str, sizeof(cseq_str), "%zu", _cseq++);
	if (ret < 0) {
		return false;
	}
	rtsp_insert_attr(req, "CSeq", cseq_str);

	return true;
}

static bool _send_options(sock_t c, rtsp_msg_t* rsp) {

	rtsp_msg_t    req;
	char*         msg;
	size_t        msg_len;

	if (_initialize_rtsp_req(&req, "OPTIONS")) {
		msg     = rtsp_marshaller_msg(&req);
		msg_len = strlen(msg) + 1;

		rtsp_send_msg(c, TYPE_OPTIONS_REQ, msg, msg_len, rsp);
		cdk_free(msg);

		rtsp_release_msg(&req);
		return true;
	}
	return false;
}

void rtsp_handshake(void) {

	sock_t c;
	c = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	rtsp_msg_t rsp;
	_send_options(c, &rsp);
	rtsp_release_msg(&rsp);
}





