_Pragma("once")

#include "cdk.h"

#define CRLF_LEN        2
#define SPACE_LEN       1
#define COLON_LEN       1

#define TYPE_RTSP_REQ   0x4
#define TYPE_RTSP_RSP   0x8

typedef enum RTSP_MSG_TYPE {
	TYPE_OPTIONS_REQ    ,
	TYPE_DESCRIBE_REQ   ,
	TYPE_SETUP_REQ      ,
	TYPE_PLAY_REQ       ,
	TYPE_TEARDOWN_REQ   ,

	TYPE_OPTIONS_RSP    ,
	TYPE_DESCRIBE_RSP   ,
	TYPE_SETUP_RSP      ,
	TYPE_PLAY_RSP       ,
	TYPE_TEARDOWN_RSP
}RTSP_MSG_TYPE;

typedef struct rtsp_attr_t {
	char*        key;
	char*        val;
	list_node_t* node;
}rtsp_attr_t;

typedef struct rtsp_msg_t {
	RTSP_MSG_TYPE type;
	char*         ver;
	list_t        attrs;

    union {
        struct {
            char* method;
            char* uri;
        } req;
        struct {
            char* status;
            int   code;
        } rsp;
    } msg;
}rtsp_msg_t;

extern void   rtsp_send_msg(sock_t c, RTSP_MSG_TYPE reqtype, char* msg, int len, rtsp_msg_t* rsp);
extern void   rtsp_insert_attr(rtsp_msg_t* rtsp_msg, char* key, char* val);
extern size_t rtsp_calc_msg_size(rtsp_msg_t* rtsp_msg);
extern char*  rtsp_marshaller_msg(rtsp_msg_t* rtsp_msg);
extern bool   rtsp_parse_msg(rtsp_msg_t* rtsp_msg, char* rspbuf);
extern void   rtsp_release_msg(rtsp_msg_t* rtsp_msg);