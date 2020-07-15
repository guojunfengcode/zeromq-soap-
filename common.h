#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <zmq.h>
#include <glib.h>

#define FRONT_ADDR     "tcp://127.0.0.1:50000"
#define BACK_ADDR      "tcp://127.0.0.1:50001"
#define HEART_BEAT_IPC "ipc://omc_heart_beat.ipc"
#define TIMEFMT        "%4d-%02d-%02d %02d:%02d:%02d.%03ld"

#if 0
#define debug(fmt, arg...) \
do { \
	debug_printf("%s %s(%d) :"fmt"\n",__DATE__, __FUNCTION__, __LINE__, ##arg); \
}while (0)
uint64_t debug_printf(const char *fmt, ...);
#endif

typedef struct {
	short event;
	int   value;
	char  addr[50];
}monitor_event;

void get_time_string(char* buf);
void debug(char* fmt, ...);

int zm_send(void *socket, void *payload,int payload_size,int nonblock);
void *zm_recv(void *socket,int *payload_len,int nonblock);
void construct_heartbeat_ans(void* soc, char* msg, const char* envelop);
int monitor_socket_data(void *sock, monitor_event *event);

#endif
