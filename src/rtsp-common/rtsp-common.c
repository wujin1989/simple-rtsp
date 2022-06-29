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
		cdk_sprintf(msg, size, "%s %s %s\r\n",
			rtsp_msg->ver, rtsp_msg->msg.rsp.code, rtsp_msg->msg.rsp.status);
	}
	for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

		rtsp_attr_t* cur_attr = cdk_list_data(n, rtsp_attr_t, node);

		cdk_strcat(msg, size, cur_attr->key);
		cdk_strcat(msg, size, ": ");
		cdk_strcat(msg, size, cur_attr->val);
		cdk_strcat(msg, size, "\r\n");

		n = cdk_list_next(&cur_attr->node);
	}
	cdk_strcat(msg, size, "\r\n");

	if (rtsp_msg->payload) {
		cdk_strcat(msg, size, rtsp_msg->payload);
	}
	return msg;
}

void rtsp_demarshaller_msg(rtsp_msg_t* rtsp_msg, char* msg) {

	char* target;
	char* tmp;
	char  cseq[64];
	char  ua[64];
	char  pub[64];
	char  serv[64];
	char  date[64];
	char  ctype[64];
	char  clen[64];
	char  transport[64];

	memset(cseq, 0, sizeof(cseq));
	memset(ua, 0, sizeof(ua));
	memset(pub, 0, sizeof(pub));
	memset(serv, 0, sizeof(serv));
	memset(date, 0, sizeof(date));
	memset(ctype, 0, sizeof(ctype));
	memset(clen, 0, sizeof(clen));
	memset(transport, 0, sizeof(transport));

	
	/* response */
	if (!strncmp(msg, "RTSP", strlen("RTSP"))) {
		char  code[64];
		char  status[64];
		char  ver[64];

		memset(code, 0, sizeof(code));
		memset(status, 0, sizeof(status));
		memset(ver, 0, sizeof(ver));

		/* add payload. */
		if (strstr(msg, "Content-Type: application/sdp")) {

			target = strstr(msg, "\r\n\r\n");
			if (target != NULL) {
				rtsp_msg->payload = cdk_strdup(target + strlen("\r\n\r\n"));
			}
		}
		else {
			rtsp_msg->payload = NULL;
		}
		/* parse code, status, version */
		target = cdk_strtok(msg, "\r\n", &tmp);
		if (target != NULL) {
			cdk_sscanf(target, "%s %s %s\r\n", ver, code, status);
		}
		rtsp_msg->type           = TYPE_RTSP_RSP;
		rtsp_msg->ver            = cdk_strdup(ver);
		rtsp_msg->msg.rsp.code   = cdk_strdup(code);
		rtsp_msg->msg.rsp.status = cdk_strdup(status);
	}
	else { /* request */
		char  method[64];
		char  uri[64];
		char  ver[64];

		memset(method, 0, sizeof(method));
		memset(uri, 0, sizeof(uri));
		memset(ver, 0, sizeof(ver));

		/* parse method, uri, version */
		target = cdk_strtok(msg, "\r\n", &tmp);
		if (target != NULL) {
			cdk_sscanf(target, "%s %s %s\r\n", method, uri, ver);
		}
		rtsp_msg->payload        = NULL;
		rtsp_msg->type           = TYPE_RTSP_REQ;
		rtsp_msg->ver            = cdk_strdup(ver);
		rtsp_msg->msg.req.method = cdk_strdup(method);
		rtsp_msg->msg.req.uri    = cdk_strdup(uri);
	}
	/* parse request and response attrs */
	cdk_list_create(&rtsp_msg->attrs);
	do {
		target = cdk_strtok(NULL, "\r\n", &tmp);
		if (target != NULL) {
			if (!strncmp(target, "CSeq", strlen("CSeq"))) {

				cdk_sscanf(target, "CSeq: %s\r\n", cseq);
				rtsp_insert_attr(rtsp_msg, "CSeq", cseq);
				continue;
			}
			if (!strncmp(target, "User-Agent", strlen("User-Agent"))) {

				cdk_sscanf(target, "User-Agent: %s\r\n", ua);
				rtsp_insert_attr(rtsp_msg, "User-Agent", ua);
				continue;
			}
			if (!strncmp(target, "Public", strlen("Public"))) {

				cdk_sscanf(target, "Public: %s\r\n", pub);
				rtsp_insert_attr(rtsp_msg, "Public", pub);
				continue;
			}
			if (!strncmp(target, "Server", strlen("Server"))) {

				cdk_sscanf(target, "Server: %s\r\n", serv);
				rtsp_insert_attr(rtsp_msg, "Server", serv);
				continue;
			}
			if (!strncmp(target, "Date", strlen("Date"))) {

				cdk_sscanf(target, "Date: %s\r\n", date);
				rtsp_insert_attr(rtsp_msg, "Date", date);
				continue;
			}
			if (!strncmp(target, "Content-Type", strlen("Content-Type"))) {

				cdk_sscanf(target, "Content-Type: %s\r\n", ctype);
				rtsp_insert_attr(rtsp_msg, "Content-Type", ctype);
				continue;
			}
			if (!strncmp(target, "Content-Length", strlen("Content-Length"))) {

				cdk_sscanf(target, "Content-Length: %s\r\n", clen);
				rtsp_insert_attr(rtsp_msg, "Content-Length", clen);
				continue;
			}
			if (!strncmp(target, "Transport", strlen("Transport"))) {

				cdk_sscanf(target, "Transport: %s\r\n", transport);
				rtsp_insert_attr(rtsp_msg, "Transport", transport);
				continue;
			}
		}
	} while (target != NULL);
	
	cdk_free(msg);
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

	mss = (cdk_net_af(s) == AF_INET) ? TCPv4_SAFE_MSS : TCPv6_SAFE_MSS;
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

static void _transcat_line(char* buf, char* line) {

	while (*buf != '\n') {
		*line = *buf;
		line++;
		buf++;
	}
	*line = '\n'; ++line;
	*line = '\0'; ++buf;
}

char* rtsp_recv_msg(sock_t s) {

	int      r;
	int      offset;
	char*    msg;

	r = offset = 0;
	msg = cdk_malloc(1024*1024);

	while (true) {

		r = recv(s, &msg[offset], 4096, 0);
		if (r <= 0) {
			return NULL;
		}
		offset += r;
		
		/* rtsp msg hdr end */
		if (strstr(msg, "\r\n\r\n")) {
			/* has payload. recv payload */
			if (strstr(msg, "Content-Type: application/sdp")) {
				/* parse payload length */
				char* tmp, *s1, *s3;
				int   len;
				char  s2[64];

				len = 0;
				memset(s2, 0, sizeof(s2));

				s1 = strstr(msg, "Content-Length");
				_transcat_line(s1, s2);
				cdk_sscanf(s2, "Content-Length: %d", &len);

				s3 = strstr(msg, "\r\n\r\n");
				if (s3 != NULL) {
					/* check payload if recv completly. */
					if (strlen(s3) < (len + strlen("\r\n\r\n"))) {
						continue;
					}
				}
			}
			/* has no payload, exit recv loop */
			break;
		}
	}
	return msg;
}

void rtsp_release_msg(rtsp_msg_t* rtsp_msg) {

	cdk_free(rtsp_msg->ver);
	cdk_free(rtsp_msg->payload);

	if (rtsp_msg->type == TYPE_RTSP_REQ) {
		cdk_free(rtsp_msg->msg.req.uri);
		cdk_free(rtsp_msg->msg.req.method);
	}
	if (rtsp_msg->type == TYPE_RTSP_RSP) {
		cdk_free(rtsp_msg->msg.rsp.code);
		cdk_free(rtsp_msg->msg.rsp.status);
	}
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

	attr->key = cdk_strdup(key);
	if (!attr->key) {
		cdk_free(attr);
		return;
	}
	attr->val = cdk_strdup(val);
	if (!attr->val) {
		cdk_free(attr->key);
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
		cnt += strlen(rtsp_msg->msg.rsp.status);
		cnt += strlen(rtsp_msg->msg.rsp.code);
	}
	for (list_node_t* n = cdk_list_head(&rtsp_msg->attrs); n != cdk_list_sentinel(&rtsp_msg->attrs);) {

		rtsp_attr_t* cur_attr = cdk_list_data(n, rtsp_attr_t, node);

		cnt += strlen(cur_attr->key);
		cnt += strlen(cur_attr->val);

		/* :[space]\r\n */
		cnt += COLON_LEN;
		cnt += SPACE_LEN;
		cnt += CRLF_LEN;

		n = cdk_list_next(&cur_attr->node);
	}
	cnt += CRLF_LEN;

	if (rtsp_msg->payload) {
		cnt += strlen(rtsp_msg->payload);
	}
	return cnt;
}