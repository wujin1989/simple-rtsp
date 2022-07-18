#include "rtsp-common/rtp.h"
#include "rtsp-common/hevc.h"

#define MAX_PAYLOAD_SIZE   1024
#define FIXED_RTP_HDR_SIZE 12


uint16_t _seq;

void _w8(uint8_t* buf, uint8_t b)
{
	*buf++ = b;
}

void _wb16(uint8_t* buf, uint32_t val)
{
	_w8(buf, (uint8_t)val >> 8);
	_w8(buf, (uint8_t)val);
}

void _wb32(uint8_t* buf, uint32_t val)
{
	_w8(buf, val >> 24);
	_w8(buf, (uint8_t)(val >> 16));
	_w8(buf, (uint8_t)(val >> 8));
	_w8(buf, (uint8_t)val);
}

rtp_t* rtp_marshaller(nalu_t* nalu) {

	rtp_t* rtp = cdk_malloc(nalu->size + FIXED_RTP_HDR_SIZE);

	/* build rtp header. */
	_w8(rtp->buffer, (0x2 << 6));
	_w8(rtp->buffer, (HEVC_PAYLOAD_TYPE & 0x7f) | ((nalu->marker & 0x01) << 7));
	_wb16(rtp->buffer, _seq);
	_wb32(rtp->buffer, nalu->timestamp);
	_wb32(rtp->buffer, VIDEO_SSRC);
    _seq = (_seq + 1) & 0xffff;

	memcpy(rtp->buffer + FIXED_RTP_HDR_SIZE, nalu->buffer, nalu->size);
    rtp->size = FIXED_RTP_HDR_SIZE + nalu->size;
    cdk_queue_init_node(&rtp->node);

	return rtp;
}

static void rtp_queue_post(rtp_queue_t* rtp_queue, rtp_t* rtp) {

    cdk_mtx_lock(&rtp_queue->mtx);
    cdk_queue_enqueue(&rtp_queue->queue, &rtp->node);
    cdk_cnd_signal(&rtp_queue->cnd);
    cdk_mtx_unlock(&rtp_queue->mtx);
}

void rtp_nalu2rtp(rtp_queue_t* rtp_queue, nalu_t* nalu) {

	if (nalu->size < MAX_PAYLOAD_SIZE) {

        rtp_queue_post(rtp_queue, rtp_marshaller(nalu));
	}
	else {

        int flag_byte;
        int header_size;
        uint8_t hdr[3];

        memset(hdr, 0, sizeof(hdr));
        uint8_t nal_type = (nalu->buffer[0] >> 1) & 0x3F;
        /*
         * create the HEVC payload header and transmit the buffer as fragmentation units (FU)
         *
         *    0                   1
         *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
         *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *   |F|   Type    |  LayerId  | TID |
         *   +-------------+-----------------+
         *
         *      F       = 0
         *      Type    = 49 (fragmentation unit (FU))
         *      LayerId = 0
         *      TID     = 1
         */
        hdr[0] = 49 << 1;
        hdr[1] = 1;

        /*
         *     create the FU header
         *
         *     0 1 2 3 4 5 6 7
         *    +-+-+-+-+-+-+-+-+
         *    |S|E|  FuType   |
         *    +---------------+
         *
         *       S       = variable
         *       E       = variable
         *       FuType  = NAL unit type
         */
        hdr[2] = nal_type;
        /* set the S bit: mark as start fragment */
        hdr[2] |= 1 << 7;

        /* pass the original NAL header */
        nalu->buffer += 2;
        nalu->size   -= 2;

        flag_byte   = 2;
        header_size = 3;

        nalu_t* nal;
        while (nalu->size + header_size > MAX_PAYLOAD_SIZE) {

            nal = cdk_malloc(MAX_PAYLOAD_SIZE);
            memcpy(nal->buffer, hdr, sizeof(hdr));
            memcpy(nal->buffer[header_size], nalu->buffer, MAX_PAYLOAD_SIZE - header_size);
            nal->size = MAX_PAYLOAD_SIZE;
            nal->marker = 0;
            nal->timestamp = nalu->timestamp;
            nal->type = nalu->type;

            rtp_queue_post(rtp_queue, rtp_marshaller(nal));

            nalu->buffer += MAX_PAYLOAD_SIZE - header_size;
            nalu->size   -= MAX_PAYLOAD_SIZE - header_size;
            nal->buffer[flag_byte] &= ~(1 << 7);
        }
        nal = cdk_malloc(MAX_PAYLOAD_SIZE);
        memcpy(nal->buffer, hdr, sizeof(hdr));
        memcpy(nal->buffer[header_size], nalu->buffer, nalu->size);
        nal->size = nalu->size;
        nal->marker = nalu->marker;
        nal->timestamp = nalu->timestamp;
        nal->type = nalu->type;
        nal->buffer[flag_byte] |= 1 << 6;
        
        rtp_queue_post(rtp_queue, rtp_marshaller(nal));
	}
	cdk_free(nalu);
}