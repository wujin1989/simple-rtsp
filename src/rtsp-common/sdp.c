#include "rtsp-common/sdp.h"

#define CRLF_LEN             2
#define SPACE_LEN            1
#define COLON_LEN            1

static void _sdp_itoa(char* buf, size_t bufsz, uint64_t n) {

	memset(buf, 0, bufsz);
	cdk_sprintf(buf, bufsz, "%llu", n);
}

void sdp_create(sdp_t* sdp, const char* addr_type, const char* addr) {

	char session_id[64];
	char session_version[64];

	_sdp_itoa(session_id, sizeof(session_id), cdk_timespec_get() / 1000);
	_sdp_itoa(session_version, sizeof(session_version), cdk_timespec_get() / 1000);

	sdp->version                     = cdk_strdup("0");

	sdp->origin.user                 = cdk_strdup("-");
	sdp->origin.session_id           = cdk_strdup(session_id);
	sdp->origin.session_version      = cdk_strdup(session_version);
	sdp->origin.net_type             = cdk_strdup("IN");
	sdp->origin.addr_type            = cdk_strdup(addr_type);
	sdp->origin.addr                 = cdk_strdup(addr);

	sdp->session_name                = cdk_strdup("simple-rtsp");
	
	sdp->time.start_time             = cdk_strdup("0");
	sdp->time.stop_time              = cdk_strdup("0");

	sdp->media[0].desc.media         = cdk_strdup("video");
	sdp->media[0].desc.port          = cdk_strdup("0");
	sdp->media[0].desc.transport     = cdk_strdup("RTP/AVP");
	sdp->media[0].desc.fmt           = cdk_strdup("96");

	cdk_list_create(&sdp->media[0].attrs);
	sdp_insert_attr(&sdp->media[0].attrs, "rtpmap", "96 H265/90000");
	sdp_insert_attr(&sdp->media[0].attrs, "control", "video");

	sdp->media_num = 1;

	sdp->media[1].desc.media         = cdk_strdup("audio");
	sdp->media[1].desc.port          = cdk_strdup("0");
	sdp->media[1].desc.transport     = cdk_strdup("RTP/AVP");
	sdp->media[1].desc.fmt           = cdk_strdup("97");

	cdk_list_create(&sdp->media[1].attrs);
	sdp_insert_attr(&sdp->media[1].attrs, "rtpmap", "97 OPUS/48000/2");
	sdp_insert_attr(&sdp->media[1].attrs, "control", "audio");

	sdp->media_num++;
}

void  sdp_destroy(sdp_t* sdp) {

	cdk_free(sdp->version);

	cdk_free(sdp->origin.session_id);
	cdk_free(sdp->origin.session_version);
	cdk_free(sdp->origin.net_type);
	cdk_free(sdp->origin.addr_type);
	cdk_free(sdp->origin.addr);

	cdk_free(sdp->time.start_time);
	cdk_free(sdp->time.stop_time);

	for (int i = 0; i < sdp->media_num; i++) {
		cdk_free(sdp->media[i].desc.media);
		cdk_free(sdp->media[i].desc.port);
		cdk_free(sdp->media[i].desc.transport);
		cdk_free(sdp->media[i].desc.fmt);

		for (list_node_t* n = cdk_list_head(&sdp->media[i].attrs); n != cdk_list_sentinel(&sdp->media[i].attrs);) {

			sdp_attr_t* attr = cdk_list_data(n, sdp_attr_t, node);
			n = cdk_list_next(n);

			cdk_free(attr->key);
			cdk_free(attr->val);
			cdk_free(attr);
		}
	}
}

void sdp_insert_attr(list_t* attrs, char* key, char* val) {

	sdp_attr_t* attr;
	attr = cdk_malloc(sizeof(sdp_attr_t));

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
	cdk_list_insert_tail(attrs, &attr->node);
}

void sdp_demarshaller_msg(char* payload, sdp_t* sdp) {

	char* target;
	char* tmp;
	char* p;

	char  user[64];
	char  session_id[64];
	char  session_version[64];
	char  net_type[64];
	char  addr_type[64];
	char  addr[64];
	char  start_time[64];
	char  stop_time[64];
	char  media[64];
	char  port[64];
	char  transport[64];
	char  fmt[64];
	char  key[64];
	char  val[64];

	memset(user, 0, sizeof(user));
	memset(session_id, 0, sizeof(session_id));
	memset(session_version, 0, sizeof(session_version));
	memset(net_type, 0, sizeof(net_type));
	memset(addr_type, 0, sizeof(addr_type));
	memset(addr, 0, sizeof(addr));
	memset(start_time, 0, sizeof(start_time));
	memset(stop_time, 0, sizeof(stop_time));

	sdp->media_num = 0;

	target = cdk_strtok(payload, "\r\n", &tmp);
	do {
		p = strchr(target, '=');
		p++;
		if (!strncmp(target, "v=", strlen("v="))) {

				sdp->version = cdk_strdup(p);
		}
		if (!strncmp(target, "o=", strlen("o="))) {
		
			cdk_sscanf(p, "%s %s %s %s %s %s %s", 
				user, session_id, session_version, 
				net_type, addr_type, addr);

			sdp->origin.user            = cdk_strdup(user);
			sdp->origin.session_id      = cdk_strdup(session_id);
			sdp->origin.session_version = cdk_strdup(session_version);
			sdp->origin.net_type        = cdk_strdup(net_type);
			sdp->origin.addr_type       = cdk_strdup(addr_type);
			sdp->origin.addr            = cdk_strdup(addr);
		}
		if (!strncmp(target, "s=", strlen("s="))) {

			sdp->session_name = cdk_strdup(p);
		}
		if (!strncmp(target, "t=", strlen("t="))) {

			cdk_sscanf(p, "%s %s", start_time, stop_time);
			sdp->time.start_time = cdk_strdup(start_time);
			sdp->time.stop_time  = cdk_strdup(stop_time);
		}
		if (!strncmp(target, "m=", strlen("m="))) {
		
			memset(media, 0, sizeof(media));
			memset(port, 0, sizeof(port));
			memset(transport, 0, sizeof(transport));
			memset(fmt, 0, sizeof(fmt));

			cdk_sscanf(p, "%s %s %s %s", media, port, transport, fmt);

			sdp->media[sdp->media_num].desc.media     = cdk_strdup(media);
			sdp->media[sdp->media_num].desc.port      = cdk_strdup(port);
			sdp->media[sdp->media_num].desc.transport = cdk_strdup(transport);
			sdp->media[sdp->media_num].desc.fmt       = cdk_strdup(fmt);

			cdk_list_create(&sdp->media[sdp->media_num].attrs);

			while ((target = cdk_strtok(NULL, "\r\n", &tmp))) {

				if (!strncmp(target, "a=", strlen("a="))) {
					p = strchr(target, '=');
					p++;

					char* temp;
					char* key = cdk_strtok(p, ":", &temp);
					char* val = cdk_strtok(NULL, ":", &temp);

					sdp_insert_attr(&sdp->media[sdp->media_num].attrs, key, val);
				}
				else {
					break;
				}
			}
			sdp->media_num++;
			continue;
		}
		target = cdk_strtok(NULL, "\r\n", &tmp);

	} while (target != NULL);
}

size_t sdp_calc_msg_size(sdp_t* sdp) {

	/* initialize to 1 for null terminator */
	size_t cnt = 1;

	cnt += strlen("v=");
	cnt += strlen(sdp->version);
	cnt += CRLF_LEN;

	cnt += strlen("o=");
	cnt += strlen(sdp->origin.user);
	cnt += strlen(sdp->origin.session_id);
	cnt += strlen(sdp->origin.session_version);
	cnt += strlen(sdp->origin.net_type);
	cnt += strlen(sdp->origin.addr_type);
	cnt += strlen(sdp->origin.addr);
	cnt += CRLF_LEN;
	cnt += SPACE_LEN * 5;

	cnt += strlen("s=");
	cnt += strlen(sdp->session_name);
	cnt += CRLF_LEN;

	cnt += strlen("t=");
	cnt += strlen(sdp->time.start_time);
	cnt += strlen(sdp->time.stop_time);
	cnt += CRLF_LEN;
	cnt += SPACE_LEN;

	for (int i = 0; i < sdp->media_num; i++) {
	
		cnt += strlen("m=");
		cnt += strlen(sdp->media[i].desc.media);
		cnt += strlen(sdp->media[i].desc.port);
		cnt += strlen(sdp->media[i].desc.transport);
		cnt += strlen(sdp->media[i].desc.fmt);
		cnt += CRLF_LEN;
		cnt += SPACE_LEN * 3;

		for (list_node_t* n = cdk_list_head(&sdp->media[i].attrs); n != cdk_list_sentinel(&sdp->media[i].attrs);) {

			sdp_attr_t* cur_attr = cdk_list_data(n, sdp_attr_t, node);

			cnt += strlen("a=");
			cnt += strlen(cur_attr->key);
			cnt += strlen(cur_attr->val);
			cnt += COLON_LEN;
			cnt += CRLF_LEN;

			n = cdk_list_next(&cur_attr->node);
		}
	}
	return cnt;
}

char* sdp_marshaller_msg(sdp_t* sdp) {

	size_t size = sdp_calc_msg_size(sdp);
	char* msg   = cdk_malloc(size);
	
	cdk_strcat(msg, size, "v=");
	cdk_strcat(msg, size, sdp->version);
	cdk_strcat(msg, size, "\r\n");

	cdk_strcat(msg, size, "o=");
	cdk_strcat(msg, size, sdp->origin.user);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->origin.session_id);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->origin.session_version);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->origin.net_type);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->origin.addr_type);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->origin.addr);
	cdk_strcat(msg, size, "\r\n");

	cdk_strcat(msg, size, "s=");
	cdk_strcat(msg, size, sdp->session_name);
	cdk_strcat(msg, size, "\r\n");

	cdk_strcat(msg, size, "t=");
	cdk_strcat(msg, size, sdp->time.start_time);
	cdk_strcat(msg, size, " ");
	cdk_strcat(msg, size, sdp->time.stop_time);
	cdk_strcat(msg, size, "\r\n");

	for (int i = 0; i < sdp->media_num; i++) {
	
		cdk_strcat(msg, size, "m=");
		cdk_strcat(msg, size, sdp->media[i].desc.media);
		cdk_strcat(msg, size, " ");
		cdk_strcat(msg, size, sdp->media[i].desc.port);
		cdk_strcat(msg, size, " ");
		cdk_strcat(msg, size, sdp->media[i].desc.transport);
		cdk_strcat(msg, size, " ");
		cdk_strcat(msg, size, sdp->media[i].desc.fmt);
		cdk_strcat(msg, size, "\r\n");

		for (list_node_t* n = cdk_list_head(&sdp->media[i].attrs); n != cdk_list_sentinel(&sdp->media[i].attrs);) {

			sdp_attr_t* cur_attr = cdk_list_data(n, sdp_attr_t, node);

			cdk_strcat(msg, size, "a=");
			cdk_strcat(msg, size, cur_attr->key);
			cdk_strcat(msg, size, ":");
			cdk_strcat(msg, size, cur_attr->val);
			cdk_strcat(msg, size, "\r\n");

			n = cdk_list_next(&cur_attr->node);
		}
	}
	return msg;
}