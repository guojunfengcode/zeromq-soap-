#include "../common.h"
#include "soapH.h"
#include "TestSoapServiceBind.nsmap"

const char server[] = "http://localhost:54321/services/TestSoapService";
GAsyncQueue *queue;
void *soap_processor(void *arg)
{	
	while (1) {
	struct soap *soap = arg;
	struct _ns2__SampleTestReq req;
	struct _ns2__SampleTestRes res;
	//req.Id = "123456";
	req.InfoName = "frist connect";
	req.Id = g_async_queue_pop(queue);
	soap_call___ns1__SampleTest(soap, server, NULL, &req, &res);
	if (soap->error) {
		debug("soap req fault, error code:%d",soap->error);
		soap_print_fault(soap, stderr);
	} else {
		debug("recv soap response");
		debug("id:%s,infodata:%s,des:%s,resultsode:%s", res.Id, res.InfoData, res.Description, res.resultCode);
	}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		debug("%s identity", argv[0]);
		debug("use identity value : a1,b2,c3,d4,e5");
		exit(0);
	}
	pthread_t soap_thread;
	queue = g_async_queue_new();
	struct soap soap;
	soap_init1(&soap, SOAP_XML_INDENT);
	char *identity = argv[1];
	void *context = zmq_ctx_new();
	void *socket = zmq_socket(context, ZMQ_DEALER);
	zmq_setsockopt(socket, ZMQ_IDENTITY, identity, strlen(identity));

	zmq_connect(socket, FRONT_ADDR);

    char msg[50] = {0}; 
	snprintf(msg, sizeof(msg), "%s_connect_success", identity);
	zm_send(socket, msg, strlen(msg), 0);

	pthread_create(&soap_thread, NULL, soap_processor, &soap);

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
				g_async_queue_push(queue, data);
				snprintf(buffer, sizeof(buffer), "%s_data", identity); 
				zm_send(socket, buffer, strlen(buffer), 0);
				debug("send %s", buffer);
			} else {
				debug("data is NULL");
			}	

		}
	}
	pthread_join(soap_thread, NULL);
	soap_destroy(&soap);
	soap_end(&soap);
	soap_free(&soap);

}
