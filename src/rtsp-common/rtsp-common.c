#include "rtsp-common/rtsp-common.h"

char* rtsp_marshaller_msg(rtsp_msg_t* rtsp_msg) {

	size_t size = rtsp_calc_msg_size(rtsp_msg);
	char* msg   = cdk_malloc(size);

	if (rtsp_msg->type == TYPE_RTSP_REQ) {

		cdk_sprintf(msg, size, "%s %s %s\r\n", 
			rtsp_msg->msg.req.method, rtsp_msg->msg.req.uri, rtsp_msg->ver);
	}
	else {
		cdk_sprintf(msg, size, "%s %d %s\r\n",
			rtsp_msg->ver, rtsp_msg->msg.rsp.code, rtsp_msg->msg.rsp.status);
	}
	rtsp_attr_t* cur_attr = cdk_list_data(cdk_list_head(&rtsp_msg->attrs), rtsp_attr_t, node);
	list_node_t* node     = &cur_attr->node;

	while (node != cdk_list_sentinel(&rtsp_msg->attrs)) {
		strcat(msg, cur_attr->key);
		strcat(msg, ": ");
		strcat(msg, cur_attr->val);
		strcat(msg, "\r\n");
		node = cdk_list_next(&cur_attr->node);
	}
	strcat(msg, "\r\n");
	return msg;
}

bool rtsp_parse_msg(rtsp_msg_t* rtsp_msg, char* rspbuf) {

	cdk_free(rspbuf);

	return true;
}

void rtsp_send_msg(sock_t c, RTSP_MSG_TYPE reqtype, char* msg, int len, rtsp_msg_t* rsp) {

	net_msg_t* smsg;
	net_msg_t* rmsg;
	int        sent;

	/*smsg = cdk_tcp_marshaller(msg, reqtype, len);
	sent = cdk_tcp_send2(c, smsg);*/
	char buf[4096] = { 0 };
	memcpy(buf, msg, len);
	send(c, msg, len, 0);
	cdk_free(msg);
	
	if (sent != len + sizeof(smsg->h)) {
		cdk_loge("send rtsp msg failed\n");
		return;
	}
	if (rsp) {
		rmsg = cdk_tcp_recv(c);
		if (rmsg == NULL) {
			cdk_loge("recv rtsp msg failed\n");
			return;
		}
		char* rspbuf = cdk_malloc(rmsg->h.p_s);
		cdk_tcp_demarshaller(rmsg, rspbuf);

		rtsp_parse_msg(rsp, rspbuf);
	}
}

void rtsp_release_msg(rtsp_msg_t* rtsp_msg) {

	for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

		rtsp_attr_t* attr = cdk_list_data(n, rtsp_attr_t, node);
		n = cdk_list_next(n);

		cdk_free(attr->key);
		cdk_free(attr->val);
		cdk_free(attr);
	}
}

void rtsp_insert_attr(rtsp_msg_t* rtsp_msg, char* key, char* val) {

	rtsp_attr_t* attr;
	attr = cdk_malloc(sizeof(rtsp_attr_t));

	attr->key = strdup(key);
	attr->val = strdup(val);
	if (!attr->key || !attr->val) {
		cdk_free(attr);
		return;
	}
	cdk_list_init_node(&attr->node);
	cdk_list_insert_tail(&rtsp_msg->attrs, &attr->node);
}

size_t rtsp_calc_msg_size(rtsp_msg_t* rtsp_msg) {

	/* initialize to 1 for null terminator */
	size_t cnt = 1;

	cnt += strlen(rtsp_msg->ver);
	cnt += CRLF_LEN;
	cnt += SPACE_LEN * 2;

	if (rtsp_msg->type == TYPE_RTSP_REQ) {
		cnt += strlen(rtsp_msg->msg.req.method);
		cnt += strlen(rtsp_msg->msg.req.uri);
	}
	else {
		char code_str[16];
		cdk_sprintf(code_str, sizeof(code_str), "%d", rtsp_msg->msg.rsp.code);
		cnt += strlen(rtsp_msg->msg.rsp.status);
		cnt += strlen(code_str);
	}
	rtsp_attr_t* cur_attr = cdk_list_data(cdk_list_head(&rtsp_msg->attrs), rtsp_attr_t, node);

	list_node_t* node = &cur_attr->node;

	while (node != cdk_list_sentinel(&rtsp_msg->attrs)) {
		cnt += strlen(cur_attr->key);
		cnt += strlen(cur_attr->val);

		/* :[space]\r\n */
		cnt += COLON_LEN;
		cnt += SPACE_LEN;
		cnt += CRLF_LEN;

		node = cdk_list_next(&cur_attr->node);
	}
	cnt += CRLF_LEN;

	return cnt;
}