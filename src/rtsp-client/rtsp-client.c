#include "cdk.h"
#include "rtsp-client/rtsp-client.h"
#include "rtsp-common/rtsp-common.h"
#include "rtsp-common/sdp.h"

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
    rtsp_insert_attr(req, "User-Agent", "simple-rtsp client");

	if (!strncmp(method, "DESCRIBE", strlen(method))) {

		char* accept = "application/sdp";
		rtsp_insert_attr(req, "Accept", accept);
	}
	if (!strncmp(method, "SETUP", strlen(method))) {

		char* transport = "RTP/AVP;unicast;client_port=50000-50001";
		rtsp_insert_attr(req, "Transport", transport);
	}
	return true;
}

static bool _send_rtsp_req(sock_t c, rtsp_msg_t* rsp, const char* method) {

	rtsp_msg_t    req;
	char*         smsg;
	char*         rmsg;
	int           smsg_len;

	if (_initialize_rtsp_req(&req, method)) {
		smsg     = rtsp_marshaller_msg(&req);
		smsg_len = strlen(smsg);

		rtsp_send_msg(c, smsg, smsg_len);

		rmsg = rtsp_recv_msg(c);
		rtsp_demarshaller_msg(rsp, rmsg);

		rtsp_release_msg(&req);
		return true;
	}
	rtsp_release_msg(&req);
	return false;
}

void run_rtspclient(void) {

	rtsp_msg_t rsp;
	sdp_t      sdp;
	sock_t     c;

	c = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	_send_rtsp_req(c, &rsp, "OPTIONS");
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("options request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	rtsp_release_msg(&rsp);

	_send_rtsp_req(c, &rsp, "DESCRIBE");
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("describe request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	sdp_demarshaller_msg(rsp.payload, &sdp);

	sdp_destroy(&sdp);
	rtsp_release_msg(&rsp);

	for (int i = 0; i < sdp.media_num; i++) {
		
		_send_rtsp_req(c, &rsp, "SETUP");
		if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
			cdk_loge("setup request failed.\n");
			rtsp_release_msg(&rsp);
			return;
		}
		rtsp_release_msg(&rsp);
	}
	



	cdk_net_close(c);
}



