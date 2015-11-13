#include <stdlib.h>
#include <signal.h>

#include <libwebsockets.h>

static int callback_increment(struct libwebsocket_context* context,
		struct libwebsocket* wsi, enum libwebsocket_callback_reasons reason,
		void* user, void* in, size_t len);

static volatile int force_exit = 0;
struct per_session_data_increment
{
	int fd;
};
static struct libwebsocket_context *context = NULL;
static struct libwebsocket_protocols protocols[] =
		{ {
			"increment",
			callback_increment,
			sizeof(struct per_session_data_increment),
			10, },
			{
			NULL,
				NULL, 0, 0 } };

static int callback_increment(struct libwebsocket_context* context,
		struct libwebsocket* wsi, enum libwebsocket_callback_reasons reason,
		void* user, void* in, size_t len)
{
	struct per_session_data_increment* pss = user;

	switch (reason)
	{
	case LWS_CALLBACK_ESTABLISHED:
		fprintf(stderr, "established\n");
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE:
		fprintf(stderr, "writeable\n");
		break;
	case LWS_CALLBACK_RECEIVE:
		fprintf(stderr, "receive\n");
		break;
	default:
		break;
	}
	return 0;
}

void sighandler(int sig)
{
	force_exit = 1;
	if (NULL != context)
	{
		libwebsocket_cancel_service(context);
	}
}

int main(void)
{
	struct lws_context_creation_info info;
	int num_clients = 0;

	memset(&info, 0, sizeof(info));

	info.port = 8080;
	info.iface = NULL;
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.gid = -1;
	info.uid = -1;

	signal(SIGINT, sighandler);

	context = libwebsocket_create_context(&info);
	if (NULL == context)
	{
		fprintf(stderr, "Could not create websocket context\n");
		return -1;
	}

	while (num_clients >= 0 && !force_exit)
	{
		num_clients = libwebsocket_service(context, 50);
	}

	libwebsocket_context_destroy(context);

	return 0;
}
