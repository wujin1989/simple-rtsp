_Pragma("once")

#include "cdk.h"

#define SERVER_IP       "0.0.0.0"
#define RTSP_PORT       "8554"

#define RTSP_VRTP_PORT "9500"
#define RTSP_ARTP_PORT "9502"

#define RTSP_VRTCP_PORT "9501"
#define RTSP_ARTCP_PORT "9503"

#define SDP_BUFFER_SIZE 1024


typedef struct rtsp_ctx {
	sock_t cfd;
	sock_t vrtp;
	sock_t vrtcp;
	sock_t artp;
	sock_t artcp;
}rtsp_ctx;

extern void run_rtspserver(void);