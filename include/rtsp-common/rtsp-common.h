_Pragma("once")

#include "cdk.h"
#include <string.h>

#define CRLF_LEN             2
#define SPACE_LEN            1
#define COLON_LEN            1

#define TYPE_RTSP_REQ        0x4
#define TYPE_RTSP_RSP        0x8

/* rtsp status code */
#define RTSP_SUCCESS_CODE    "200"

/* rtsp status string */
#define RTSP_SUCCESS_STRING  "OK"

typedef struct rtsp_attr_t {
	char*        key;
	char*        val;
	list_node_t  node;
}rtsp_attr_t;

typedef struct rtsp_msg_t {
	int           type;
	char*         ver;
	list_t        attrs;
	char*         payload;

    union {
        struct {
            char* method;
            char* uri;
        } req;
        struct {
            char* status;
            char* code;
        } rsp;
    } msg;
}rtsp_msg_t;

extern int    rtsp_send_msg(sock_t s, char* restrict msg, int len);
extern char*  rtsp_recv_msg(sock_t s);
extern void   rtsp_insert_attr(rtsp_msg_t* rtsp_msg, char* key, char* val);
extern size_t rtsp_calc_msg_size(rtsp_msg_t* rtsp_msg);
extern char*  rtsp_marshaller_msg(rtsp_msg_t* rtsp_msg);
extern void   rtsp_demarshaller_msg(rtsp_msg_t* rtsp_msg, char* msg);
extern void   rtsp_release_msg(rtsp_msg_t* rtsp_msg);