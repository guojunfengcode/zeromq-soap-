#include "../common.h"
#include "soapH.h"
#include "soapStub.h"
#include "TestSoapServiceBind.nsmap"

GThreadPool *req_process_thread_pool;
int port = 54321;

gpointer req_process(gpointer arg)
{
	struct soap* soap = arg;
	debug("start server");
	soap_serve(soap);
	debug("finish server");
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	return NULL;
}

int main(int argc, char *argv[]) 
{
	g_thread_init(NULL);
	struct soap soap;
	int fd;
	soap_init(&soap);
	soap_bind(&soap, NULL, port, 100);

	req_process_thread_pool = g_thread_pool_new(req_process, NULL, 5, FALSE, NULL);

	while (1) {
		fd = soap_accept(&soap);
		if (fd < 0) {
			soap_print_fault(&soap, stderr);
			exit(0);
		}
		debug("has new accept");
		void *arg = (void*)soap_copy(&soap);
		if (arg) {
			g_thread_pool_push(req_process_thread_pool, arg, NULL);
		}
	}

	soap_close(&soap);

}

int __ns1__SampleTest(struct soap *soap, struct _ns2__SampleTestReq* ns2__SampleTestReq, struct _ns2__SampleTestRes *ns2__SampleTestRes)
{
	ns2__SampleTestRes->Id = ns2__SampleTestReq->Id;
	ns2__SampleTestRes->InfoData = "yes";
	ns2__SampleTestRes->Description = "des";
	ns2__SampleTestRes->resultCode = "resultcode";
	debug("recv a requset id:%s infoname:%s", ns2__SampleTestReq->Id, ns2__SampleTestReq->InfoName);

	return 0;
}
