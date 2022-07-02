#include "rtsp-server/rtsp-server.h"
#include "rtsp-common/rtsp-common.h"
#include "rtsp-common/sdp.h"

static void _gmt_time(char* buf, int bufsz) {

	struct tm tm;
	time_t    t;

	memset(buf, 0, bufsz);
	t = time(NULL);
	cdk_localtime(&t, &tm);
	strftime(buf, bufsz, "%a, %d %b %Y %H:%M:%S GMT", &tm);
}

static void _rtsp_itoa(char* buf, size_t bufsz, size_t n) {

	memset(buf, 0, bufsz);
	cdk_sprintf(buf, bufsz, "%zu", n);
}

static bool _initialize_rtsp_rsp(rtsp_msg_t* rsp, rtsp_msg_t* rtsp_msg, rtsp_ctx_t* pctx) {

	/* parse general attr. (CSeq) */
	char* cseq;
	cseq = NULL;

	for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

		rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
		if (!strncmp(attr->key, "CSeq", strlen("CSeq"))) {
			cseq = attr->val;
		}
		n = cdk_list_next(n);
	}
	rsp->type           = TYPE_RTSP_RSP;
	rsp->ver            = cdk_strdup(rtsp_msg->ver);
	rsp->msg.rsp.code   = cdk_strdup(RTSP_SUCCESS_CODE);
	rsp->msg.rsp.status = cdk_strdup(RTSP_SUCCESS_STRING);

	cdk_list_create(&rsp->attrs);

	char date[64];
	_gmt_time(date, sizeof(date));

	rtsp_insert_attr(rsp, "CSeq", cseq);
	rtsp_insert_attr(rsp, "Date", date);
	rtsp_insert_attr(rsp, "Server", "simple-rtsp server");

	if (!strncmp(rtsp_msg->msg.req.method, "OPTIONS", strlen("OPTIONS"))) {

		rsp->payload   = NULL;
		rtsp_insert_attr(rsp, "Public", "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY");
	}
	if (!strncmp(rtsp_msg->msg.req.method, "DESCRIBE", strlen("DESCRIBE"))) {

		char       len[64];
		sdp_t      sdp;
		addrinfo_t ai;

		cdk_net_obtain_addr(pctx->conn, &ai, false);
		sdp_create(&sdp, (ai.f == AF_INET ? "IP4" : "IP6"), ai.a);

		rsp->payload = sdp_marshaller_msg(&sdp);
		sdp_destroy(&sdp);
		_rtsp_itoa(len, sizeof(len), strlen(rsp->payload));
		
		rtsp_insert_attr(rsp, "Content-Type", "application/sdp");
		rtsp_insert_attr(rsp, "Content-Length", len);
	}
	if (!strncmp(rtsp_msg->msg.req.method, "SETUP", strlen("SETUP"))) {

		char session[64];
		char transport[128];
		char serv_port[64];

		memset(session, 0, sizeof(session));
		memset(transport, 0, sizeof(transport));
		memset(serv_port, 0, sizeof(serv_port));

		if (strstr(rtsp_msg->msg.req.uri, "video")) {
		
			cdk_sprintf(serv_port, sizeof(serv_port), ";server_port=%s-%s", RTSP_VRTP_PORT, RTSP_VRTCP_PORT);
		}
		if (strstr(rtsp_msg->msg.req.uri, "audio")) {

			cdk_sprintf(serv_port, sizeof(serv_port), ";server_port=%s-%s", RTSP_ARTP_PORT, RTSP_ARTCP_PORT);
		}
		cdk_sprintf(session, sizeof(session), "%llu", cdk_timespec_get() / 1000);
		
		for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

			rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
			if (!strncmp(attr->key, "Transport", strlen("Transport"))) {

				memcpy(transport, attr->val, strlen(attr->val));
			}
			if (!strncmp(attr->key, "Session", strlen("Session"))) {

				memset(session, 0, sizeof(session));
				memcpy(session, attr->val, strlen(attr->val));
			}
			n = cdk_list_next(n);
		}
		cdk_strcat(transport, sizeof(transport), serv_port);

		rtsp_insert_attr(rsp, "Session", session);
		rtsp_insert_attr(rsp, "Transport", transport);
		rsp->payload = NULL;
	}
	if (!strncmp(rtsp_msg->msg.req.method, "PLAY", strlen("PLAY"))) {

		rsp->payload = NULL;

	}
	return true;
}

static bool _send_rtsp_rsp(rtsp_ctx_t* pctx, rtsp_msg_t* rtsp_msg) {

	rtsp_msg_t rsp;
	char*      smsg;
	int        smsg_len;

	if (_initialize_rtsp_rsp(&rsp, rtsp_msg, pctx)) {
		smsg = rtsp_marshaller_msg(&rsp);
		smsg_len = strlen(smsg);

		rtsp_send_msg(pctx->conn, smsg, smsg_len);

		rtsp_release_msg(&rsp);
		return true;
	}
	rtsp_release_msg(&rsp);
	return false;
}

static int _handle_rtsp(void* arg) {

	char*         msg;
	bool          ret;
	rtsp_msg_t    req;
	rtsp_ctx_t*   pctx;
	rtsp_ctx_t    ctx;

	pctx = arg;
	ctx  = *pctx;

	while (true) {
		msg = rtsp_recv_msg(ctx.conn);
		if (!msg) {
			cdk_loge("network error.\n");
			cdk_net_close(ctx.conn);
			return -1;
		}
		rtsp_demarshaller_msg(&req, msg);

		if (!strncmp(req.msg.req.method, "TEARDOWN", strlen("TEARDOWN"))) {

		}
		_send_rtsp_rsp(pctx, &req);

		rtsp_release_msg(&req);
	}
	cdk_net_close(ctx.conn);
	return 0;
}

void run_rtspserver(void) {

	sock_t   conn;
	sock_t   listen;
	sock_t   video_rtp;
	sock_t   video_rtcp;
	sock_t   audio_rtp;
	sock_t   audio_rtcp;
	thrd_t   t;

	listen = cdk_tcp_listen(SERVER_IP, RTSP_PORT);

	video_rtp  = cdk_udp_listen(SERVER_IP, RTSP_VRTP_PORT);
	audio_rtp  = cdk_udp_listen(SERVER_IP, RTSP_ARTP_PORT);
	video_rtcp = cdk_udp_listen(SERVER_IP, RTSP_VRTCP_PORT);
	audio_rtcp = cdk_udp_listen(SERVER_IP, RTSP_ARTCP_PORT);

	while (true) {

		conn = cdk_tcp_accept(listen);

		rtsp_ctx_t ctx = {
			.audio_rtcp   = audio_rtcp,
			.audio_rtp    = audio_rtp,
			.conn         = conn,
			.listen       = listen,
			.video_rtcp   = video_rtcp,
			.video_rtp    = video_rtp
		};

		if (!cdk_thrd_create(&t, _handle_rtsp, &ctx)) {
			break;
		}
		cdk_thrd_detach(t);
	}
	cdk_net_close(conn);
	cdk_net_close(listen);
}