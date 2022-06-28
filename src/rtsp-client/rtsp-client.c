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

static void _parse_sdp(char* strsdp, sdp_t* sdp) {

	char*    target;
	char*    tmp;
	uint8_t  ptype;
	uint32_t samplerate;
	uint8_t  channelnum;
	char     tmode[64];
	char     rtp_port[64];
	char     codec[64];
	char     trackid[64];

	ptype = samplerate = channelnum = 0;
	memset(tmode, 0, sizeof(tmode));
	memset(rtp_port, 0, sizeof(rtp_port));
	memset(codec, 0, sizeof(codec));
	memset(trackid, 0, sizeof(trackid));

	target = cdk_strtok(strsdp, "\r\n", &tmp);
	if (target != NULL) {
		if (!strncmp(target, "v=", strlen("v="))) {
			sdp->ver = cdk_strdup(strrchr(target, '=') + 1);
		}
	}
	do {
		target = cdk_strtok(NULL, "\r\n", &tmp);
		if (target != NULL) {
			
			if (!strncmp(target, "o=", strlen("o="))) {
				sdp->server_addr = cdk_strdup(strrchr(target, ' ') + 1);
				continue;
			}
			if (!strncmp(target, "s=", strlen("s="))) {
				sdp->session = cdk_strdup(strrchr(target, '=') + 1);
				continue;
			}
			if (!strncmp(target, "m=video", strlen("m=video"))) {
				cdk_sscanf(target, "m=video %s %s %u", rtp_port, tmode, &ptype);

				sdp->video.mtype    = cdk_strdup("video");
				sdp->video.tmode    = cdk_strdup(tmode);
				sdp->video.ptype    = ptype;

				target = cdk_strtok(NULL, "\r\n", &tmp);
				while (target != NULL) {
					if (!strncmp(target, "a=", strlen("a="))) {
						if (!strncmp(target, "a=rtpmap", strlen("a=rtpmap"))) {
							char  p[64];
							char* q;
							
							memset(p, 0, sizeof(p));
							cdk_sscanf(target, "a=rtpmap:%u %s", &ptype, p);

							q = strchr(p, '/');

							sdp->video.codec      = cdk_strdup(codec);
							sdp->video.samplerate = samplerate;

						}
						if (!strncmp(target, "a=control", strlen("a=control"))) {
							cdk_sscanf(target, "a=control:%s", trackid);

							sdp->video.trackid = cdk_strdup(trackid);
						}
						target = cdk_strtok(NULL, "\r\n", &tmp);
					}
					else {
						break;
					}
				}
			}
			if (!strncmp(target, "m=audio", strlen("m=audio"))) {
				cdk_sscanf(target, "m=audio %s %s %u", rtp_port, tmode, &ptype);

				sdp->audio.mtype    = cdk_strdup("audio");
				sdp->audio.tmode    = cdk_strdup(tmode);
				sdp->audio.ptype    = ptype;
				
				target = cdk_strtok(NULL, "\r\n", &tmp);
				while (target != NULL) {
					if (!strncmp(target, "a=", strlen("a="))) {
						if (!strncmp(target, "a=rtpmap", strlen("a=rtpmap"))) {
							cdk_sscanf(target, "a=rtpmap:%s %[^/]%u/%u", &ptype, codec, &samplerate, &channelnum);

							sdp->audio.codec      = cdk_strdup(codec);
							sdp->audio.samplerate = samplerate;
							sdp->audio.channelnum = channelnum;

						}
						if (!strncmp(target, "a=control", strlen("a=control"))) {
							cdk_sscanf(target, "a=control:%s", trackid);

							sdp->audio.trackid = cdk_strdup(trackid);
						}
						target = cdk_strtok(NULL, "\r\n", &tmp);
					}
					else {
						break;
					}
				}
			}
		}
	} while (target != NULL);
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
	_parse_sdp(rsp.payload, &sdp);
	rtsp_release_msg(&rsp);

	_send_rtsp_req(c, &rsp, "SETUP");
	if (strncmp(rsp.msg.rsp.code, RTSP_SUCCESS_CODE, strlen(RTSP_SUCCESS_CODE))) {
		cdk_loge("setup request failed.\n");
		rtsp_release_msg(&rsp);
		return;
	}
	rtsp_release_msg(&rsp);



	cdk_net_close(c);
}



