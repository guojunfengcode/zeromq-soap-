#include "common.h"

#define PTHREAD_NUM    5
#define MILLI_SEC  1000

typedef struct {
	GArray* array;
}Config;

Config conf;
void *context;


char *discard_heartbeat_ans(void* zmq_soc)
{
	char *buff = (char *)malloc(50);
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	zmq_recvmsg(zmq_soc, &msg, 0);
	memcpy(buff, zmq_msg_data(&msg), zmq_msg_size(&msg));
	zmq_msg_close(&msg);
	return buff;
}

void *heartbeat_processor(void *arg) 
{	
	//char *identity = arg;
	char identity[20] = {0};
	memcpy(identity, (char *)arg, strlen((char *)arg));
	//because before call g_array_append_val function array is save malloc address, so nedd free.
	free(arg);
	int flag = 1;
	int failtime = 0;
	int send_heart = 1;
	debug("%s thread start", identity);
	void* heartbeat = zmq_socket(context, ZMQ_DEALER);
	zmq_setsockopt(heartbeat, ZMQ_IDENTITY, identity, strlen(identity));
	zmq_socket_monitor(heartbeat, "inproc://monitor", ZMQ_EVENT_ALL);
	zmq_connect(heartbeat, HEART_BEAT_IPC);

	zmq_pollitem_t items[] = {
		{heartbeat, 0, ZMQ_POLLIN, 0}
	};

	while (1) {
		if (flag) {
			flag = 0;
			char *value = "frist send packet";
			zmq_msg_t msg;
			zmq_msg_init_size(&msg, strlen(value));
			memcpy(zmq_msg_data(&msg), value, strlen(value));
			zmq_sendmsg(heartbeat, &msg, 0);
			zmq_msg_close(&msg);
			debug("frist send heartbeat");
		}
		
		zmq_poll(items, 1, 10*MILLI_SEC);
		if (items[0].revents & ZMQ_POLLIN) {
			failtime = 0;
			send_heart = 1;
			char *data = discard_heartbeat_ans(heartbeat);
			if (data) {
				debug("recv [%s] a heartbeat, success msg :%s", identity,data);
				free(data);
			}
		} else {
			if (send_heart) {
				++failtime;
				debug("Try to establish a heartbeat No.%d with %s", failtime, identity);
				char data[50] = {0};
				snprintf(data, sizeof(data), "heat_beat_packt_%s", identity);
				zmq_msg_t msg;
				zmq_msg_init_size(&msg, strlen(data));
				memcpy(zmq_msg_data(&msg), data, strlen(data));
				zmq_sendmsg(heartbeat, &msg, 0);
				zmq_msg_close(&msg);
				debug("send a heart beat to [%s], msg_data is [%s]", identity, data);
			}
		}
		if (failtime >= 3) {
			failtime = 0;
			send_heart = 0;
		}
	}

}


void *monitor_proccess(void *arg)
{
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
				case ZMQ_EVENT_CONNECTED:
					debug("connectd socket descriptor %d, address is %s", event.value, event.addr);
					break;
				case ZMQ_EVENT_LISTENING:
					debug("listening socket descriptor %d, address is %s", event.value, event.addr);
					break;
				case ZMQ_EVENT_ACCEPTED:
					debug("accepted socket descriptor %d, address is %s", event.value, event.addr);
					break;
				case ZMQ_EVENT_DISCONNECTED:
					debug("disconnected socket descriptor %d, address is %s", event.value, event.addr);
					break;
			}
		}
	}

}

void pars_str(GArray** p_array, const char* str, int i_str_len) {
	*p_array = g_array_new(FALSE, TRUE, sizeof(char*));
	char loc_str[1024] = {0};
	memcpy(loc_str, str, i_str_len);
	const char* loc_str_temp = loc_str;
	char* p_delimiter = NULL;
	char* p_module = NULL;
	int i_module_len = 0;
	while( (p_delimiter = strchr(loc_str_temp, ':')) ) {
		i_module_len = p_delimiter - loc_str_temp;
		p_module = (char*)malloc(i_module_len + 1);
		memset(p_module, 0, i_module_len + 1);
		memcpy(p_module, loc_str_temp, i_module_len);
		
		loc_str_temp = loc_str_temp + i_module_len +1;
		g_array_append_val(*p_array, p_module);
		
	}
	i_module_len = strlen(loc_str_temp);
	p_module = (char*)malloc(i_module_len + 1);
	memset(p_module, 0, i_module_len + 1);
	memcpy(p_module, loc_str_temp, i_module_len);
	g_array_append_val(*p_array, p_module);
}

gboolean init_config(GKeyFile *file, char *filename)
{
	GError *err = NULL;
	gboolean res = g_key_file_load_from_file(file, filename,
		G_KEY_FILE_NONE, &err);
	if (!res) {
		debug("load config file %s error\n", filename);
		if (err != NULL) {
			debug("error is:%s\n", err->message);
			g_error_free(err);
		}
	}
	//return malloc size of space, nedd free.
	char* str_value = g_key_file_get_string(file, "info", "identity", &err);
	debug("get config info  %s", str_value);
	
	pars_str(&(conf.array), str_value, strlen(str_value));
	free(str_value);
	return res;
}



int main(int argc, char *argv[]) 
{
	if (argc != 2) {
		debug("%s config", argv[0]);
		exit(0);
	}
	GKeyFile *file = g_key_file_new();
	context = zmq_ctx_new();

	init_config(file, argv[1]);
	pthread_t back_thread[PTHREAD_NUM];
	pthread_t monitor_thread;
	int i;
	
	for (i = 0; i < conf.array->len; i++) {
		int ret = 0;
		ret = pthread_create(&back_thread[i], NULL, heartbeat_processor, g_array_index(conf.array, char*, i));
		if (ret) {
			perror("front thread create fail: ");
			return 0;
		}
	}
	pthread_create(&monitor_thread, NULL, monitor_proccess, NULL);
	
	for (i = 0; i < conf.array->len; i++) {
		pthread_join(back_thread[i], NULL);
	}
	pthread_join(monitor_thread, NULL);
	return 0;
}
