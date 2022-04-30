_Pragma("once")

typedef enum RTSP_REQ_TYPE{
	TYPE_RTSP_OPTIONS_REQ    ,
	TYPE_RTSP_DESCRIBE_REQ   ,
	TYPE_RTSP_SETUP_REQ      ,
	TYPE_RTSP_PLAY_REQ       ,
	TYPE_RTSP_TEARDOWN_REQ
}RTSP_REQ_TYPE;

typedef enum RTSP_RSP_TYPE {
	TYPE_RTSP_OPTIONS_RSP    ,
	TYPE_RTSP_DESCRIBE_RSP   ,
	TYPE_RTSP_SETUP_RSP      ,
	TYPE_RTSP_PLAY_RSP       ,
	TYPE_RTSP_TEARDOWN_RSP
}RTSP_RSP_TYPE;

typedef struct rtsp_msg_t {
	char*    ver;
    size_t   cseq;
	char*    msgbuf;

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