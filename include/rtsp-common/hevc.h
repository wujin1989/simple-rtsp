_Pragma("once")

#include <stdint.h>

#define HEVC_PAYLOAD_TYPE    96

extern uint8_t* hevc_find_startcode(uint8_t* p, uint8_t* end);