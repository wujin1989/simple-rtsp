#include "cdk.h"
#include "rtsp-client/rtsp-client.h"
#include "rtsp-common/rtsp-common.h"

static bool _initialize_rtsp_req(rtsp_msg_t* req, const char* method) {

	int           ret;
	static size_t cseq;
	char          uri[64];

	memset(uri, 0, sizeof(uri));
	ret = cdk_sprintf(uri, sizeof(uri), "rtsp://%s:%s/live", RTSP_SERVER_ADDRESS, RTSP_PORT);
	if (ret < 0) {
		return false;
	}
	req->type           = TYPE_RTSP_REQ;
	req->ver            = cdk_strdup("RTSP/1.0");
	req->msg.req.method = cdk_strdup(method);
	req->msg.req.uri    = cdk_strdup(uri);
	req->payload        = NULL;
	
	cdk_list_create(&req->attrs);

	char  cseq_str[64];
	memset(cseq_str, 0, sizeof(cseq_str));

	ret = cdk_sprintf(cseq_str, sizeof(cseq_str), "%zu", ++cseq);
	if (ret < 0) {
		return false;
	}
	rtsp_insert_attr(req, "CSeq", cseq_str); 
    rtsp_insert_attr(req, "User-Agent", USER_AGENT_ATTR);

	if (!strncmp(method, "DESCRIBE", strlen(method))) {

		char* accept = "application/sdp";
		rtsp_insert_attr(req, "Accept", accept);
	}
	return true;
}

static bool _send_rtsp_req(rtsp_msg_t* rsp, const char* method) {

	rtsp_msg_t    req;
	char*         smsg;
	char*         rmsg;
	int           smsg_len;
	sock_t        c;

	c = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	if (_initialize_rtsp_req(&req, method)) {
		smsg     = rtsp_marshaller_msg(&req);
		smsg_len = strlen(smsg);

		rtsp_send_msg(c, smsg, smsg_len);

		rmsg = rtsp_recv_msg(c, false);
		rtsp_demarshaller_msg(rsp, rmsg);

		rtsp_release_msg(&req);
		cdk_net_close(c);
		return true;
	}
	rtsp_release_msg(&req);
	cdk_net_close(c);
	return false;
}

static void _rtsp_handshake(void) {

	rtsp_msg_t rsp;

	_send_rtsp_req(&rsp, "OPTIONS");
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("options request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	rtsp_release_msg(&rsp);

	_send_rtsp_req(&rsp, "DESCRIBE");
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("describe request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	rtsp_release_msg(&rsp);

}

void run_rtspclient(void) {

	_rtsp_handshake();
}



