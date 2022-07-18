_Pragma("once")

#include <stdint.h>
#include "cdk.h"

typedef struct nalu_t {

    uint8_t        type;
    uint8_t*       buffer;
    uint64_t       size;
    bool           marker;  /* last nalu of frame */
    uint32_t       timestamp;
    fifo_node_t    node;
}nalu_t;

typedef struct nalu_queue_t {

    fifo_t         queue;
    cnd_t          cnd;
    mtx_t          mtx;
}nalu_queue_t;