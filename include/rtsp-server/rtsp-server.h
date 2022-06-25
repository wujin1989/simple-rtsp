_Pragma("once")

#include "cdk.h"

#define SERVER_IP       "0.0.0.0"
#define RTSP_PORT       "8554"

#define RTSP_VIDEO_PORT "9500"
#define RTSP_AUDIO_PORT "9502"

#define SDP_BUFFER_SIZE 1024


typedef struct rtsp_ctx {
	sock_t cfd;
	sock_t afd;
	sock_t vfd;
}rtsp_ctx;

extern void run_rtspserver(void);