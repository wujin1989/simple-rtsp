#include "cdk.h"
#include "rtsp-client/rtsp-client.h"
#include "rtsp-common/rtsp-common.h"
#include "rtsp-common/sdp.h"

static bool _initialize_rtsp_req(rtsp_msg_t* req, const char* method, const char* track, const char* session) {

	int           ret;
	static size_t cseq;
	char          uri[64];

	memset(uri, 0, sizeof(uri));
	if (track == NULL) {
		ret = cdk_sprintf(uri, sizeof(uri), "rtsp://%s:%s/live", RTSP_SERVER_ADDRESS, RTSP_PORT);
		if (ret < 0) {
			return false;
		}
	}
	else {
		ret = cdk_sprintf(uri, sizeof(uri), "rtsp://%s:%s/live/%s", RTSP_SERVER_ADDRESS, RTSP_PORT, track);
		if (ret < 0) {
			return false;
		}
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

		if (strcmp(session, "")) {
			rtsp_insert_attr(req, "Session", session);
		}
	}
	return true;
}

static void _find_port(rtsp_msg_t* msg, rtsp_ctx_t* pctx, int idx) {

	char rtcp[64];
	char rtp[64];

	memset(rtcp, 0, sizeof(rtcp));
	memset(rtp, 0, sizeof(rtp));

	for (list_node_t* n = cdk_list_head(&msg->attrs); n != cdk_list_sentinel(&msg->attrs);) {

		rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
		n = cdk_list_next(n);

		if (!strncmp(attr->key, "Transport", strlen("Transport"))) {

			char* p = strstr(attr->val, ";server_port=");
			p += strlen(";server_port=");
			cdk_sscanf(p, "%[^-]-%s", rtp, rtcp);

			memcpy(pctx->media[idx].port.rtcp, rtcp, strlen(rtcp));
			memcpy(pctx->media[idx].port.rtp, rtp, strlen(rtp));
		}
	}
}

static void _find_session(rtsp_msg_t* msg, rtsp_ctx_t* pctx) {

	for (list_node_t* n = cdk_list_head(&msg->attrs); n != cdk_list_sentinel(&msg->attrs);) {

		rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
		n = cdk_list_next(n);

		if (!strncmp(attr->key, "Session", strlen("Session"))) {
			memcpy(pctx->session, attr->val, strlen(attr->val));
		}
	}
}

static void _find_mediainfo(sdp_t* sdp, rtsp_ctx_t* pctx) {

	char* tmp;
	char* fmt;
	char* name;
	char* samplerate;
	char* parameter;

	for (int i = 0; i < pctx->media_num; i++) {
	
		memcpy(pctx->media[i].type, sdp->media[i].desc.media, strlen(sdp->media[i].desc.media));
		memcpy(pctx->media[i].trans_mode, sdp->media[i].desc.transport, strlen(sdp->media[i].desc.transport));

		/* only support first fmt. */
		fmt = cdk_strtok(sdp->media[i].desc.fmt, " ", &tmp);

		for (list_node_t* n = cdk_list_head(&sdp->media[i].attrs); n != cdk_list_sentinel(&sdp->media[i].attrs);) {

			sdp_attr_t* attr = cdk_list_data(n, sdp_attr_t, node);
			n = cdk_list_next(n);

			if (!strncmp(attr->key, "control", strlen("control"))) {
				if (strstr(attr->val, "://")) {
					char* p = strrchr(attr->val, '/');
					p++;
					memcpy(pctx->media[i].track, p, strlen(p));
				}
				else {
					memcpy(pctx->media[i].track, attr->val, strlen(attr->val));
				}
			}
			if (!strncmp(attr->key, "rtpmap", strlen("rtpmap"))) {
				char* s = strstr(attr->val, fmt);
				s += strlen(fmt);
				if (s) {
					name       = cdk_strtok(s, "/", &tmp);
					samplerate = cdk_strtok(NULL, "/", &tmp);
					parameter  = cdk_strtok(NULL, "/", &tmp);

					memcpy(pctx->media[i].codec.name, name, strlen(name));
					memcpy(pctx->media[i].codec.samplerate, samplerate, strlen(samplerate));
					if (parameter != NULL) {
						memcpy(pctx->media[i].codec.parameter, parameter, strlen(parameter));
					}
				}
			}
		}
	}
}

static bool _send_rtsp_req(rtsp_ctx_t* pctx, rtsp_msg_t* rsp, const char* method, const char* track) {

	rtsp_msg_t    req;
	char*         smsg;
	char*         rmsg;
	int           smsg_len;

	if (_initialize_rtsp_req(&req, method, track, pctx->session)) {
		smsg     = rtsp_marshaller_msg(&req);
		smsg_len = strlen(smsg);

		rtsp_send_msg(pctx->conn, smsg, smsg_len);

		rmsg = rtsp_recv_msg(pctx->conn);
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
	sock_t     conn;
	rtsp_ctx_t ctx;

	memset(&ctx, 0, sizeof(ctx));
	conn = cdk_tcp_dial(RTSP_SERVER_ADDRESS, RTSP_PORT);

	ctx.conn = conn;

	_send_rtsp_req(&ctx, &rsp, "OPTIONS", NULL);
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("options request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	rtsp_release_msg(&rsp);

	_send_rtsp_req(&ctx, &rsp, "DESCRIBE", NULL);
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("describe request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	sdp_demarshaller_msg(rsp.payload, &sdp);
	rtsp_release_msg(&rsp);

	ctx.media_num = sdp.media_num;
	_find_mediainfo(&sdp, &ctx);

	for (int i = 0; i < sdp.media_num; i++) {
		
		_send_rtsp_req(&ctx, &rsp, "SETUP", ctx.media[i].track);
		if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
			cdk_loge("setup request failed.\n");
			rtsp_release_msg(&rsp);
			return;
		}
		_find_session(&rsp, &ctx);
		_find_port(&rsp, &ctx, i);

		rtsp_release_msg(&rsp);
	}

	// create rtp-rtcp socket and start recive thread.
	// init decoder by codec name and start decode thread.

	sdp_destroy(&sdp);

	cdk_net_close(conn);
}



