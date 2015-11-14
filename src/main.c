#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include <libwebsockets.h>

#define MAX_PAYLOAD_LEN 1400

static int callback_increment(struct libwebsocket_context* context,
		struct libwebsocket* wsi, enum libwebsocket_callback_reasons reason,
		void* user, void* in, size_t len);

static volatile int force_exit = 0;
struct per_session_data_increment
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_PAYLOAD_LEN
			+ LWS_SEND_BUFFER_POST_PADDING];
	unsigned int len;
};
static struct libwebsocket_context *context = NULL;
enum ws_bridge_protocols
{
	PROTOCOL_INCREMENT = 0
};

static struct libwebsocket_protocols protocols[] =
		{ {
			"increment",
			callback_increment,
			sizeof(struct per_session_data_increment),
			256, },
			{
			NULL,
				NULL, 0, 0 } };

static int callback_increment(struct libwebsocket_context* context,
		struct libwebsocket* wsi, enum libwebsocket_callback_reasons reason,
		void* user, void* in, size_t len)
{
	int n;
	unsigned char transmit_buf[MAX_PAYLOAD_LEN] = { '\0' };
	unsigned int transmit_buf_len = 0;
	struct per_session_data_increment* pss = user;

	switch (reason)
	{
	case LWS_CALLBACK_ESTABLISHED:
		fprintf(stderr, "established\n");
		/* initialize user data */
		pss->buf[0] = '\0';
		pss->len = 0;
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE:
		fprintf(stderr, "writeable\n");

		if (pss->len == 0)
		{
			break;
		}
		transmit_buf_len = snprintf(
				&(transmit_buf[LWS_SEND_BUFFER_PRE_PADDING]),
				MAX_PAYLOAD_LEN, "Retrieving value of variable: %s",
				&(pss->buf[0]));
		n = libwebsocket_write(wsi, &transmit_buf[LWS_SEND_BUFFER_PRE_PADDING],
				transmit_buf_len, LWS_WRITE_TEXT);
		if (n < 0)
		{
			fprintf(stderr, "Error writing to socket.\n");
			return 1;
		}
		if (n < (int) transmit_buf_len)
		{
			fprintf(stderr, "Error: partial write\n");
			return -1;
		}
		break;
	case LWS_CALLBACK_RECEIVE:
		fprintf(stderr, "received: %s\n", (const char*) in);
		/* register variable names to be notified about */
		if (len > MAX_PAYLOAD_LEN)
		{
			fprintf(stderr, "Error: packet is too big.\n");
			return 1;
		}
		memcpy(&(pss->buf[0]), in, len);
		pss->len = (unsigned int) len;
		break;
	case LWS_CALLBACK_CLOSED:
		fprintf(stderr, "closed\n");
		break;
	case LWS_CALLBACK_PROTOCOL_DESTROY:
		fprintf(stderr, "protocol destroy\n");
		break;
	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		/* you could return non-zero here and kill the connection */
		break; /* authorized */
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

void* trigger_thread(void* user_data)
{
	int* num_users = user_data;
	struct timespec req = { 0, 500000000L };
	while (*num_users >= 0 && !force_exit)
	{
		libwebsocket_callback_on_writable_all_protocol(
				&protocols[PROTOCOL_INCREMENT]);
		nanosleep(&req, NULL);
	}
	fprintf(stderr, "Trigger thread shutting down.\n");
	return NULL;
}

int main(void)
{
	struct lws_context_creation_info info;
	int num_clients = 0;
	pthread_t tid = 0;

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

	pthread_create(&tid, NULL, trigger_thread, &num_clients);

	while (num_clients >= 0 && !force_exit)
	{
		num_clients = libwebsocket_service(context, 50);
	}
	pthread_join(tid, NULL);

	libwebsocket_context_destroy(context);

	return 0;
}
