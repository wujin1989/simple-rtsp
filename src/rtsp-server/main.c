#include "rtsp-server/rtsp-server.h"
#include "rtsp-common/nalu.h"

int main(void) {

	nalu_queue_t q;

	cdk_logger_init(NULL, true);

	cdk_cnd_init(&q.cnd);
	cdk_mtx_init(&q.mtx);
	cdk_queue_create(&q.queue);

	run_rtspserver(&q);

	cdk_cnd_destroy(&q.cnd);
	cdk_mtx_destroy(&q.mtx);
	cdk_logger_destroy();
	return 0;
}