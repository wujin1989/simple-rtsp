_Pragma("once")

#include "cdk.h"

#define MAX_MEDIA_NUM    16

typedef struct sdp_attr_t {

	char*        key;
	char*        val;
	list_node_t  node;
}sdp_attr_t;

typedef struct sdp_media_t {

	struct
	{
		char*    media;
		char*    port;
		char*    transport;
		char*    fmt;
	} desc;

	list_t       attrs;
}sdp_media_t;

typedef struct sdp_t {

	char*        version;
	struct
	{
		char*    user;	                /* user */
		char*    session_id;		    /* session id */
		char*    session_version;	    /* session version */
		char*    net_type;	            /* network type ("IN") */
		char*    addr_type;	            /* address type ("IP4", "IP6") */
		char*    addr;	                /* the address */
	} origin;
	
	char*	     session_name;

	struct 
	{
		char*    start_time;
		char*    stop_time;
	} time;

	uint8_t      media_num;
	sdp_media_t  media[MAX_MEDIA_NUM];
}sdp_t;

extern void  sdp_create(sdp_t* sdp, const char* addr_type, const char* addr);
extern void  sdp_destroy(sdp_t* sdp);
extern void  sdp_insert_attr(list_t* attrs, char* key, char* val);
extern char* sdp_marshaller_msg(sdp_t* sdp);
extern void  sdp_demarshaller_msg(char* payload, sdp_t* sdp);