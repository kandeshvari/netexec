#include <stdio.h>
#include <string.h>

#include "http_parser.h"
#include "list.h"
#include "http.h"
#include "log.h"

static int on_info(http_parser *p) {
	return 0;
}

static int on_data(http_parser *p, const char *at, size_t length) {
	return 0;
}

static int on_url(http_parser *p, const char *at, size_t length) {
	if (length > HTTP_MAX_URL_SIZE-1) {
		return -1;
	}
	request_t *r = p->data;
	r->url = malloc(length+1);
	bzero((char*)r->url, length+1);
	snprintf((char*)r->url, length+1, "%s", at);
	return 0;
}


static int on_body(http_parser *p, const char *at, size_t length) {
	if (length > HTTP_MAX_BODY_SIZE-1) {
		return -1;
	}
	request_t *r = p->data;
	r->body = malloc(length+1);
	bzero((char*)r->body, length+1);
	snprintf((char*)r->body, length+1, "%s", at);
	r->body_length = length+1;
	return 0;
}

static int on_header_field(http_parser *p, const char *at, size_t length) {
	request_t *r = p->data;

	if (length > HTTP_MAX_HEADER_NAME_SIZE-1)
		return -1;
	header_t *header = malloc(sizeof(header_t));
	bzero(header, sizeof(header_t));
	snprintf(header->name, length+1, "%s", at);
	list_append(r->headers, header);

	return 0;
}

static int on_header_value(http_parser *p, const char *at, size_t length) {
	if (length > HTTP_MAX_HEADER_VALUE_SIZE-1)
		return -1;
	request_t *r = p->data;
	list_t *pos = r->headers;
	while (pos->next)
		pos = pos->next;
	header_t *h = (header_t*)pos->data;
	snprintf(h->value, length+1, "%s", at);
	return 0;
}

static http_parser_settings settings = {
	.on_message_begin = on_info,
	.on_headers_complete = on_info,
	.on_message_complete = on_info,
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_url = on_url,
	.on_status = on_data,
	.on_body = on_body
};

/*
 * Parse http request
 */
request_t *parse_http_request(const char *data, ssize_t data_len) {
	struct http_parser parser;
	size_t parsed;
	request_t *request = malloc(sizeof(request_t));
	*request = (request_t){};

	http_parser_init(&parser, HTTP_REQUEST);
	request->headers = list_new();

	parser.data = request;
	parsed = http_parser_execute(&parser, &settings, data, (size_t)data_len);
	if (parsed != data_len) {
		log_error("Invalid HTTP request");
		list_free(request->headers, true, NULL);
		free(request);
		return NULL;
	}

	return request;
}

/*
 * Dispose request structure
 */
void free_request(request_t *request) {
	list_free(request->headers, true, NULL);
	if (request->url)
		free(request->url);
	if (request->body)
		free(request->body);
	free(request);
}
