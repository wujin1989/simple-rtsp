_Pragma("once")

#include "rtsp-common/rtsp-common.h"

#define RTSP_SERVER_ADDRESS    "192.168.1.200"
#define RTSP_PORT              "554"

extern void rtsp_handshake(void);
extern bool send_options(sock_t c, rtsp_msg_t* rsp);
extern bool initialize_rtsp_req(rtsp_msg_t* req, char* method);
extern char* marshaller_rtsp_msg(rtsp_msg_t* req);