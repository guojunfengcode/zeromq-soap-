#include "common.h"

void *back_proccess(void *arg)
{
	void *context = arg;
	void *recv = zmq_socket(context, ZMQ_DEALER);
	zmq_connect(recv, BACK_ADDR);

	void* heart_beat = zmq_socket(context, ZMQ_ROUTER);
	zmq_socket_monitor(heart_beat, "inproc://monitor", ZMQ_EVENT_ALL);
	zmq_bind(heart_beat, HEART_BEAT_IPC);

	zmq_pollitem_t items[] = {
		{recv, 0, ZMQ_POLLIN, 0},
		{heart_beat, 0, ZMQ_POLLIN, 0}
	};

	while (1) {
		int64_t more;
		zmq_poll (items, 2, -1);
		if (items[0].revents & ZMQ_POLLIN) {
			int addr_len = 0;
			int buff_len = 0;
			char msg[100] = {0};
			char addr_info[10] = {0};


			void *addr = zm_recv(recv, &addr_len, 0);
			if (addr == NULL) {
				debug("envelop is null");
				continue;
			}
			memcpy(addr_info, addr, addr_len);
			free(addr);
			addr = NULL;

			void *buff = zm_recv(recv, &buff_len, 0);
			if (buff == NULL) {
				debug("recv data is NULL");
				continue;
			}
			
			memcpy(msg, buff, buff_len);
			//debug("recv envelop addr:[%s]  msg:[%s]\n", addr_info, msg);
			debug("recv envelop addr:[%s]  msg:[%s]", addr_info, msg);
			construct_heartbeat_ans(heart_beat, msg, addr_info);
			free(buff);
			buff = NULL;
		}
		if (items[1].revents & ZMQ_POLLIN) {
			while(1) {
				char buff[50] = {0};
				zmq_msg_t msg;
				zmq_msg_init(&msg);
				zmq_recvmsg(heart_beat, &msg, 0);
				memcpy(buff, zmq_msg_data(&msg), zmq_msg_size(&msg));
				size_t more_size = sizeof (more);
				zmq_getsockopt (heart_beat, ZMQ_RCVMORE, &more, &more_size);
				//debug("%s %ld\n",buff,more);
				zmq_sendmsg (recv, &msg, more? ZMQ_SNDMORE: 0);
				zmq_msg_close(&msg);
				if (!more) {
					debug("heart_beat_ipc recv [%s]", buff);
					break;
				}
			}
		}
	}
}

void *monitor_proccess(void *arg) 
{
	void *context = arg;
	void *monitor_sock = zmq_socket(context, ZMQ_PAIR);
	zmq_connect(monitor_sock, "inproc://monitor");

	zmq_pollitem_t items[] = {
		{monitor_sock, 0, ZMQ_POLLIN, 0 }
	};

	while(1) {
		zmq_poll(items, 1, -1);
		if (items[0].revents & ZMQ_POLLIN) {
			monitor_event event;
			memset(event.addr, 0, sizeof(event.addr));

			monitor_socket_data(monitor_sock, &event);
			switch (event.event) {
				case ZMQ_EVENT_LISTENING:
					debug("listening socket descriptor %d, address is %s\n", event.value, event.addr);
					break;
				case ZMQ_EVENT_ACCEPTED:
					debug("accepted socket descriptor %d, address is %s\n", event.value, event.addr);
					break;
				case ZMQ_EVENT_CLOSED:
					debug("closed socket descriptor %d, address is %s\n", event.value, event.addr);
					break;
				case ZMQ_EVENT_DISCONNECTED:
					debug("disconnected socket descriptor %d, address is %s\n", event.value, event.addr);
					break;
			}

		}
	}
	zmq_close(monitor_sock);

}

int main(int argc, char *argv[])
{
	pthread_t back_thread;
	pthread_t monitor_thread;
	int ret = 0;
	void *context = zmq_ctx_new();

	void *front = zmq_socket(context, ZMQ_ROUTER);
	zmq_setsockopt(front, ZMQ_IDENTITY, "queue", 5);
	zmq_bind(front, FRONT_ADDR);

	void *back = zmq_socket(context, ZMQ_DEALER);
	zmq_bind(back, BACK_ADDR);
	
	ret = pthread_create(&back_thread, NULL, back_proccess, context);
	if (ret) {
		perror("front thread create fail: ");
		return 0;
	}
	ret = pthread_create(&monitor_thread, NULL, monitor_proccess, context);
	if (ret) {
		perror("front thread create fail: ");
		return 0;
	}

	zmq_device(ZMQ_QUEUE, front, back);
	pthread_join(back_thread, NULL);
	pthread_join(monitor_thread, NULL);

	zmq_close(front);
	zmq_close(back);
	zmq_ctx_destroy(context);
	return 0;
	
}
