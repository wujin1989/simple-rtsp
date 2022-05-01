#include "cdk.h"
#include "rtsp-client/rtsp-client.h"

static size_t cur_cseq;

void rtsp_handshake(void) {

	sock_t c;
	c = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	rtsp_msg_t rsp;
	send_options(c, &rsp);
}

void send_rtsp_msg(sock_t c, RTSP_REQ_TYPE reqtype, char* msg, int len, rtsp_msg_t* rsp) {

	net_msg_t* smsg;
	net_msg_t* rmsg;
	int        sent;

	smsg = cdk_tcp_marshaller(msg, reqtype, len);
	sent = cdk_tcp_send(c, smsg);
	if (sent != (smsg->h.p_s + sizeof(smsg->h))) {
		cdk_loge("send rtsp msg failed\n");
		return;
	}
	rmsg = cdk_tcp_recv(c);
	if (rmsg == NULL) {
		cdk_loge("recv rtsp msg failed\n");
		return;
	}
	cdk_tcp_demarshaller(rmsg, rsp->msgbuf);
}

bool send_options(sock_t c, rtsp_msg_t* rsp) {

	rtsp_msg_t    rtsp_req;
	char*         rtsp_msg;
	uint32_t      rtsp_msg_len;
	
	if (initialize_rtsp_req(&rtsp_req, "OPTIONS")) {
	
		rtsp_msg     = marshaller_rtsp_msg(&rtsp_req);
		rtsp_msg_len = strlen(rtsp_msg) + 1;

		send_rtsp_msg(c, TYPE_OPTIONS_REQ, rtsp_msg, rtsp_msg_len, rsp);
		return true;
	}
	return false;
}

bool initialize_rtsp_req(rtsp_msg_t* req, char* method) {

	req->cseq = cur_cseq;
	return true;
}

char* marshaller_rtsp_msg(rtsp_msg_t* req) {

	return "";
}