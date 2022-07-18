_Pragma("once")

#include <stdint.h>
#include "nalu.h"

#define AUDIO_SSRC    0x11223344
#define VIDEO_SSRC    0x44332211

typedef struct rtp_t {

	uint8_t*       buffer;
	uint64_t       size;
    fifo_node_t    node;
}rtp_t;

typedef struct rtp_queue_t {

    fifo_t         queue;
    cnd_t          cnd;
    mtx_t          mtx;
}rtp_queue_t;

extern rtp_t* rtp_marshaller(nalu_t* nalu);
extern void rtp_nalu2rtp(rtp_queue_t* rtp_queue, nalu_t* nalu);
extern void rtp_demarshaller();
