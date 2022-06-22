#include "rtsp-common/rtsp-common.h"

#define TCPv4_SAFE_MSS    536
#define TCPv6_SAFE_MSS    1220

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

static char* _extendbuffer(char* buf, size_t size) {

	char* new = realloc(buf, size);
	if (new == NULL && buf != NULL) {
		free(buf);
	}
	if (!new) {
		abort();
	}
	return new;
}

int rtsp_send_msg(sock_t s, char* restrict msg, int len) {

	uint32_t mss;
	uint32_t st;    /* sent bytes */
	uint32_t sz;    /* msg size */

	mss = (_cdk_net_af(s) == AF_INET) ? TCPv4_SAFE_MSS : TCPv6_SAFE_MSS;
	st = 0;
	sz = len;

	while (st < sz) {
		int sd = ((sz - st) > mss) ? mss : (sz - st);
		int r = send(s, &((const char*)msg)[st], sd, 0);
		if (r <= 0) {
			return st;
		}
		st += sd;
	}
	cdk_free(msg);
	return sz;
}

char* rtsp_recv_msg(sock_t s) {

	int   r;
	int   len;
	int   offset;
	char* msg;

	r = len = offset = 0;
	msg = NULL;

	while (true) {

		if (offset >= len) {
			len = offset + 4096;
			msg = _extendbuffer(msg, len);
		}
		r = recv(s, &msg[offset], len - offset, 0);
		if (r <= 0) {
			return NULL;
		}
		offset += r;
		/* rtsp msg end */
		if (msg[offset - 1] == '\n' && msg[offset -2] == '\r' 
			&& msg[offset - 3] == '\n' && msg[offset - 4] == '\r') {
			break;
		}
	}
	return msg;
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