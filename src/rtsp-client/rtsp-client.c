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

static bool _send_options_req(rtsp_msg_t* rsp) {

	rtsp_msg_t    req;
	char*         smsg;
	char*         rmsg;
	int           smsg_len;
	sock_t        c;

	c = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	if (_initialize_rtsp_req(&req, "OPTIONS")) {
		smsg     = rtsp_marshaller_msg(&req);
		smsg_len = strlen(smsg);

		rtsp_send_msg(c, smsg, smsg_len);

		rmsg = rtsp_recv_msg(c, false);

		rtsp_release_msg(&req);
		cdk_net_close(c);
		return true;
	}
	cdk_net_close(c);
	return false;
}

void rtsp_handshake(void) {

	rtsp_msg_t rsp;
	_send_options_req(&rsp);
	rtsp_release_msg(&rsp);
}





