#pragma once

#define HTTP_MAX_HEADER_NAME_SIZE 1024
#define HTTP_MAX_HEADER_VALUE_SIZE 8096
#define HTTP_MAX_BODY_SIZE (1*1024*1024)  // 1M
#define HTTP_MAX_URL_SIZE 4096  // 1M

#include "list.h"

typedef struct {
	char *url;
	list_t *headers;
	char *body;
	size_t body_length;
} request_t;

typedef struct {
	char name[HTTP_MAX_HEADER_NAME_SIZE];
	char value[HTTP_MAX_HEADER_VALUE_SIZE];
} header_t;

request_t *parse_http_request(const char *data, ssize_t data_len);
void free_request(request_t *request);

// TODO: construct headers on-the-fly
static char const response_headers[] = "HTTP/1.1 200 Ok\n"
	"Server: localhost\n"
	"Content-Type: text/plain\n"
	"\n";

static char const bad_response_headers[] = "HTTP/1.1 400 Bad request\n"
	"\n";

static const size_t response_headers_len = sizeof(response_headers)-1;
static const size_t bad_response_headers_len = sizeof(bad_response_headers)-1;


