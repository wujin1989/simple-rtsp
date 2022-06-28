#include "rtsp-server/rtsp-server.h"
#include "rtsp-common/rtsp-common.h"

static void _gmt_time(char* tmp) {

	struct tm tm;
	time_t    t;

	t = time(NULL);
	cdk_localtime(&t, &tm);
	strftime(tmp, 64, "%a, %d %b %Y %H:%M:%S GMT", &tm);
}

static bool _initialize_rtsp_rsp(rtsp_msg_t* rsp, rtsp_msg_t* rtsp_msg) {

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

	char gtime[64];
	memset(gtime, 0, sizeof(gtime));
	_gmt_time(gtime);

	cdk_list_create(&rsp->attrs);

	rtsp_insert_attr(rsp, "CSeq", cseq);
	rtsp_insert_attr(rsp, "Date", gtime);
	rtsp_insert_attr(rsp, "Server", SERVER_ATTR);

	if (!strncmp(rtsp_msg->msg.req.method, "OPTIONS", strlen("OPTIONS"))) {

		rsp->payload   = NULL;
		char* smethods = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY";

		rtsp_insert_attr(rsp, "Public", smethods);
	}
	if (!strncmp(rtsp_msg->msg.req.method, "DESCRIBE", strlen("DESCRIBE"))) {

		char len[64];
		rsp->payload = cdk_strdup(rtsp_msg->payload);
		cdk_sprintf(len, sizeof(len), "%d", strlen(rsp->payload));

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
		cdk_sprintf(session, sizeof(session), "%lld", time(NULL));
		
		for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

			rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
			if (!strncmp(attr->key, "Transport", strlen("Transport"))) {

				memcpy(transport, attr->val, strlen(attr->val));
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

static bool _send_rtsp_rsp(sock_t s, rtsp_msg_t* rtsp_msg) {

	rtsp_msg_t rsp;
	char*      smsg;
	int        smsg_len;

	if (_initialize_rtsp_rsp(&rsp, rtsp_msg)) {
		
		if (rsp.payload != NULL) {
			smsg     = rtsp_marshaller_msg(&rsp);
			smsg_len = strlen(smsg);
		}
		else {
			smsg     = rtsp_marshaller_msg(&rsp);
			smsg_len = strlen(smsg);
		}
		rtsp_send_msg(s, smsg, smsg_len);

		rtsp_release_msg(&rsp);
		return true;
	}
	rtsp_release_msg(&rsp);
	return false;
}

static char* _generate_sdp(sock_t cfd) {

	addrinfo_t ai;
	char*      sdp; 
	
	sdp = cdk_malloc(SDP_BUFFER_SIZE);
	cdk_net_obtain_addr(cfd, &ai, false);

	cdk_sprintf(sdp, SDP_BUFFER_SIZE,
		"v=0\r\n"
		"o=- %lld 0 IN %s %s\r\n"
		"s=simple-rtsp\r\n"
		"t=0 0\r\n"
		"m=video 0 RTP/AVP 96\r\n"
		"a=rtpmap:96 H265/90000\r\n"
		"a=control:video\r\n"
		"m=audio 0 RTP/AVP 97\r\n"
		"a=rtpmap:97 OPUS/48000/2\r\n"
		"a=control:audio\r\n"
		"\r\n\r\n",
		time(NULL), cdk_net_af(cfd) == AF_INET ? "IPv4" : "IPv6", ai.a);

	return sdp;
}

static int _handle_rtsp(void* arg) {

	char*       msg;
	bool        ret;
	rtsp_msg_t  req;
	rtsp_ctx*   pctx;
	rtsp_ctx    ctx;

	pctx = arg;
	ctx  = *pctx;

	while (true) {
		msg = rtsp_recv_msg(ctx.cfd);
		if (!msg) {
			cdk_loge("network error.\n");
			cdk_net_close(ctx.cfd);
			return -1;
		}
		rtsp_demarshaller_msg(&req, msg);

		if (!strncmp(req.msg.req.method, "DESCRIBE", strlen("DESCRIBE"))) {

			req.payload = _generate_sdp(ctx.cfd);
		}
		if (!strncmp(req.msg.req.method, "TEARDOWN", strlen("TEARDOWN"))) {

		}
		_send_rtsp_rsp(ctx.cfd, &req);

		rtsp_release_msg(&req);
	}
	cdk_net_close(ctx.cfd);
	return 0;
}

void run_rtspserver(void) {

	sock_t   cfd, sfd;
	sock_t   vrtp, vrtcp;
	sock_t   artp, artcp;
	thrd_t   t;

	sfd = cdk_tcp_listen(SERVER_IP, RTSP_PORT);

	vrtp = cdk_udp_listen(SERVER_IP, RTSP_VRTP_PORT);
	artp = cdk_udp_listen(SERVER_IP, RTSP_ARTP_PORT);
	vrtcp = cdk_udp_listen(SERVER_IP, RTSP_VRTCP_PORT);
	artcp = cdk_udp_listen(SERVER_IP, RTSP_ARTCP_PORT);

	while (true) {

		cfd = cdk_tcp_accept(sfd);

		rtsp_ctx ctx = {
			.cfd   = cfd,
			.vrtp  = vrtp,
			.artp  = artp,
			.vrtcp = vrtcp,
			.artcp = artcp
		};

		if (!cdk_thrd_create(&t, _handle_rtsp, &ctx)) {
			break;
		}
		cdk_thrd_detach(t);
	}
	cdk_net_close(cfd);
	cdk_net_close(sfd);
}