#include "common.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		debug("%s identity", argv[0]);
		debug("use identity value : a1,b2,c3,d4,e5");
		exit(0);
	}
	char *identity = argv[1];
	void *context = zmq_ctx_new();
	void *socket = zmq_socket(context, ZMQ_DEALER);
	zmq_setsockopt(socket, ZMQ_IDENTITY, identity, strlen(identity));

	zmq_connect(socket, FRONT_ADDR);

    char msg[50] = {0}; 
	snprintf(msg, sizeof(msg), "%s_connect_success", identity);
	zm_send(socket, msg, strlen(msg), 0);

	zmq_pollitem_t items [] = {
		{socket,0,ZMQ_POLLIN,0}
	};

	while (1) {
		zmq_poll(items, 1, 10);
		if (items[0].revents & ZMQ_POLLIN) {
			void *msg = NULL;
			int msg_len = 0;
			msg = zm_recv(socket,&msg_len,0);
			if (msg) {
				char *data = (char *)msg;
				char buffer[50] = {0};
				debug("%s recv data is [%s]", identity, data);
				snprintf(buffer, sizeof(buffer), "%s_data", identity); 
				zm_send(socket, buffer, strlen(buffer), 0);
				debug("send %s", buffer);
			} else {
				debug("data is NULL");
			}	

		}
	}


}
