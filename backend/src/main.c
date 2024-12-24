#include <microhttpd.h>
#include <hiredis/hiredis.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
	"</head><body>libmicrohttpd demo</body></html>"

void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

redisContext *rc;

void redis_set(const char *name, const char *value)
{
	if(!redisCommand(rc, "SET %s %s", name, value)) printf("Writing to redis failed!\n");

}
char *redis_get(const char *name)
{
	redisReply *reply = redisCommand(rc, "GET %s", name);
	if(!reply) {
		printf("%s: failed to read\n", name);
		return NULL;
	}
	else if(reply->type == REDIS_REPLY_NIL) {
		printf("%s: doesn't exists\n", name);
		return NULL;
	}
	assert(reply->type == REDIS_REPLY_STRING);
	return reply->str;

}

enum MHD_Result return_notfound(struct MHD_Connection *connection)
{
	 	struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
		const int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
		MHD_destroy_response(response);
		return ret;

}	

static enum MHD_Result
ahc_echo(void *cls,
		struct MHD_Connection *connection,
		const char *url,
		const char *method,
		const char *version,
		const char *upload_data,
		size_t *upload_data_size,
		void **ptr) {
	static int dummy;
	static char short_url[5];
	const char *page = cls;
	struct MHD_Response *response;
	int ret;
	char buff[64];
	strcpy(buff, url);

	//strtok(buff, "/");
	const char *prefix = strtok(buff, "/");
	const char *target = strtok(NULL, "/");

	if(0 != strcmp(prefix, "api")) {
			return MHD_NO;
	}

	if (!*ptr)
	{
		*ptr = &dummy;
		return MHD_YES;
	}
	if(*upload_data_size) {
		char buff[256];
		strcpy(buff, upload_data);
		const char *name = strtok(buff, "=");
		const char *value = strtok(NULL, "=");
		printf("%s = %s\n", name, value);
		if(0 != strcmp(name, "url")) {
			return MHD_NO;
		}
		rand_str(short_url, 5);
		redis_set(short_url, value);

		*upload_data_size = 0;
		return MHD_YES;
	}
	else if(0 == strcmp(method, "GET")) {
		*ptr = NULL; 
		if(target == NULL) return return_notfound(connection);
		const char *result = redis_get(target);
		if(result == NULL) return return_notfound(connection);
		printf("%s\n", result);
		response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
		MHD_add_response_header(response, "LOCATION", result); 
		ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
		MHD_destroy_response(response);
		return ret;
	}
	*ptr = NULL; 
	response = MHD_create_response_from_buffer(strlen(short_url), (void*) short_url, MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(response, "Access-Control-Allow-Origin", "*"); 
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}

int main(int argc, char **argv) {
	struct MHD_Daemon * d;
	if (argc != 2) {
		printf("%s PORT\n", argv[0]);
		return 1;
	}
	rc = redisConnect("127.0.0.1", 6379);
	if (rc == NULL || rc->err) {
		if (rc) {
			printf("Error: %s\n", rc->errstr);
			// handle error
		} else {
			printf("Can't allocate redis context\n");
		}
	}
	d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
			atoi(argv[1]),
			NULL,
			NULL,
			&ahc_echo,
			PAGE,
			MHD_OPTION_END);
	if (d == NULL)
		return 1;
	(void) getc (stdin);
	MHD_stop_daemon(d);
	return 0;
}
