#include "common.h"


void get_time_string(char* buf) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	struct tm mytm={0};
	struct tm* p_tm = localtime_r(&tv.tv_sec,&mytm);
	sprintf(buf, TIMEFMT, p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday,
				p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, (tv.tv_usec)/1000);
}

void debug(char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char time_buf[50] = {0};
	get_time_string(time_buf);
	printf("%s :", time_buf);

	va_list ap2;
	va_copy(ap2, ap);
	vprintf(fmt, ap2);
	printf("\n");
	fflush(stdout);
	va_end(ap);
	va_end(ap2);
}

#if 0
uint64_t debug_printf(const char *fmt, ...)
{
	char buf[4096] = {0};
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vsnprintf(buf, 4096, fmt, ap);
	va_end(ap);
	write(1, buf, n);
	return 0;
}
#endif
void *zm_recv(void *socket,int *payload_len,int nonblock) {
	zmq_msg_t message;
	zmq_msg_init(&message);
	if (nonblock)
		*payload_len = zmq_msg_recv( &message,socket,ZMQ_NOBLOCK);
	else
		*payload_len = zmq_msg_recv( &message,socket,0);
	if (*payload_len < 0)
		return NULL;
	*payload_len = zmq_msg_size(&message);
	if(*payload_len==0){
		zmq_msg_close(&message);
		return NULL;
	}
	void *payload = malloc(*payload_len);
	memcpy(payload, zmq_msg_data(&message), *payload_len);
	zmq_msg_close(&message);
	return (payload);
}

int zm_send(void *socket, void *payload,int payload_size,int nonblock) {
	int rc;
	zmq_msg_t message;
	zmq_msg_init_size(&message, payload_size);
	memcpy(zmq_msg_data(&message), payload, payload_size);
	if (nonblock)
		rc = zmq_sendmsg(socket, &message, ZMQ_NOBLOCK);
	else
		rc = zmq_sendmsg(socket, &message, 0);
	zmq_msg_close(&message);
	return rc;
}

void construct_heartbeat_ans(void* soc, char* msg, const char* envelop) {
	zmq_msg_t addr;
	zmq_msg_init_size(&addr, strlen(envelop));
	memcpy(zmq_msg_data(&addr), envelop, strlen(envelop));

	zmq_msg_t body;
	zmq_msg_init_size(&body, strlen(msg));
	memcpy(zmq_msg_data(&body), msg, strlen(msg));
	
	zmq_sendmsg(soc, &addr, ZMQ_SNDMORE);
	zmq_sendmsg(soc, &body, 0);

	zmq_msg_close(&addr);
	zmq_msg_close(&body);

}

int monitor_socket_data(void *sock, monitor_event *event)
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);

	if(zmq_msg_recv(&msg, sock, 0) == -1)
		return -1;
	char *data = (char *) zmq_msg_data (&msg);
	event->event = *(short *)data;
	event->value = *(int *)(data+2);
	zmq_msg_close (&msg);

	zmq_msg_init (&msg);
	if(zmq_msg_recv(&msg, sock, 0) == -1)
		return -1;
	char *addr_data = (char *) zmq_msg_data (&msg);
	size_t size = zmq_msg_size (&msg);
	memcpy(event->addr, addr_data, size);
	event->addr[size] = 0;
	zmq_msg_close(&msg);
	return 0;
}
