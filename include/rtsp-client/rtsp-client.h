_Pragma("once")

#define RTSP_SERVER_ADDRESS    "192.168.1.200"
#define RTSP_PORT              "8554"

#define MAX_MEDIA_NUM          16

typedef struct rtsp_port_t {
	char rtp[64];
	char rtcp[64];
}rtsp_port_t;

typedef struct rtsp_codec_t {
	char name[64];
	char samplerate[64];
	char parameter[64];
}rtsp_codec_t;

typedef struct rtsp_media_t {

	char         type[64];
	char         trans_mode[64];
	char         track[64];

	rtsp_port_t  port;
	rtsp_codec_t codec;
}rtsp_media_t;

typedef struct rtsp_ctx_t {

	sock_t       conn;
	char         session[64];
	
	uint8_t      media_num;
	rtsp_media_t media[MAX_MEDIA_NUM];
	
}rtsp_ctx_t;

typedef struct thrd_mng_t {

	thrd_t v_rcv_thrd;
	thrd_t a_rcv_thrd;
}thrd_mng_t;

extern void run_rtspclient(void);