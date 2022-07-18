_Pragma("once")

#include "cdk.h"
#include "rtsp-common/rtp.h"

#define SERVER_IP       "0.0.0.0"
#define RTSP_PORT       "8554"

#define RTSP_VRTP_PORT  "20000"
#define RTSP_ARTP_PORT  "20002"

#define RTSP_VRTCP_PORT "20001"
#define RTSP_ARTCP_PORT "20003"


typedef struct rtsp_ctx_t {
	sock_t   conn;
	sock_t   listen;
	sock_t   video_rtp;
	sock_t   video_rtcp;
	sock_t   audio_rtp;
	sock_t   audio_rtcp;
}rtsp_ctx_t;

extern void run_rtspserver(nalu_queue_t* nalu_queue);