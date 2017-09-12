
#include "jsonrpc.h"

extern ctx_t ctx;

static int send_response(sock_data_t *fds, char *response) {
	log_debug("JSON Response: %s", response);
	SSL_write(fds->ssl_sock, response_headers, response_headers_len);
	SSL_write(fds->ssl_sock, response, (int) strlen(response));
	return 0;
}

static int send_result(sock_data_t *fds, cJSON *result, cJSON *id) {
	int return_value = 0;
	cJSON *result_root = cJSON_CreateObject();
	if (result)
		cJSON_AddItemToObject(result_root, "result", result);
	cJSON_AddItemToObject(result_root, "id", id);

	char *str_result = cJSON_PrintUnformatted(result_root);
	return_value = send_response(fds, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	return return_value;
}


static int send_error(sock_data_t *fds, int code, char *message, cJSON *id) {
	int return_value = 0;
	cJSON *result_root = cJSON_CreateObject();
	cJSON *error_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(error_root, "code", code);
	cJSON_AddStringToObject(error_root, "message", message);
	cJSON_AddItemToObject(result_root, "error", error_root);
	cJSON_AddItemToObject(result_root, "id", id);
	char *str_result = cJSON_PrintUnformatted(result_root);
	return_value = send_response(fds, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	free(message);
	return return_value;
}

static int invoke_procedure(char *name, cJSON *params, cJSON *id, sock_data_t *fds) {
	cJSON *returned = NULL;
	int procedure_found = 0;
	jrpc_context jrpc_ctx;
	jrpc_ctx.error_code = 0;
	jrpc_ctx.error_message = NULL;
	int i = ctx.jsonrpc.procedure_count;
	while (i--) {
		if (!strcmp(ctx.jsonrpc.procedures[i].name, name)) {
			procedure_found = 1;

			/* validate parameters */
			if (params->type != cJSON_Object)
				return send_error(fds, JRPC_INVALID_PARAMS, strdup("Invalid params. Parameters must be object."), id);

//			log_debug("XXX: MANDATORY COOUNT: %d", ctx.jsonrpc.procedures[i].p_count);
			for (int j = 0; j < ctx.jsonrpc.procedures[i].p_count; j++) {
//				log_debug("XXX: param: [%s]", ctx.jsonrpc.procedures[i].p_names[j]);
				if (!cJSON_GetObjectItem(params, ctx.jsonrpc.procedures[i].p_names[j]))
					return send_error(fds, JRPC_INVALID_PARAMS, strdup("Invalid params. Mandatory param not found."),
							  id);
			}

			jrpc_ctx.data = ctx.jsonrpc.procedures[i].data;
			returned = ctx.jsonrpc.procedures[i].function(&jrpc_ctx, params, id, fds);
			break;
		}
	}
	if (!procedure_found)
		return send_error(fds, JRPC_METHOD_NOT_FOUND, strdup("Method not found."), id);
	else {
		if (jrpc_ctx.error_code)
			return send_error(fds, jrpc_ctx.error_code, jrpc_ctx.error_message, id);
		else
			return send_result(fds, returned, id);
	}
}

static int eval_request(cJSON *root, sock_data_t *fds) {
	cJSON *method, *params, *id;
	method = cJSON_GetObjectItem(root, "method");
	if (method != NULL && method->type == cJSON_String) {
		params = cJSON_GetObjectItem(root, "params");
		if (params == NULL || params->type == cJSON_Array
		    || params->type == cJSON_Object) {
			id = cJSON_GetObjectItem(root, "id");
			if (id == NULL || id->type == cJSON_String
			    || id->type == cJSON_Number) {
				//We have to copy ID because using it on the reply and deleting the response Object will also delete ID
				cJSON *id_copy = NULL;
				if (id != NULL)
					id_copy = (id->type == cJSON_String) ? cJSON_CreateString(id->valuestring) :
						  cJSON_CreateNumber(id->valueint);
				log_debug("Method Invoked: %s", method->valuestring);
				return invoke_procedure(method->valuestring, params, id_copy, fds);
			}
		}
	}
	send_error(fds, JRPC_INVALID_REQUEST, strdup("The JSON sent is not a valid Request object."), NULL);
	return -1;
}

void jrpc_procedure_destroy(struct jrpc_procedure *procedure) {
	if (procedure->name) {
		free(procedure->name);
		procedure->name = NULL;
	}
	if (procedure->data) {
		free(procedure->data);
		procedure->data = NULL;
	}
}

int jrpc_register_procedure(jrpc_function function_pointer, char *name, void *data, char *p_names[], int p_count) {
	int i = ctx.jsonrpc.procedure_count++;
	if (!ctx.jsonrpc.procedures)
		ctx.jsonrpc.procedures = malloc(sizeof(struct jrpc_procedure));
	else {
		struct jrpc_procedure *ptr = realloc(ctx.jsonrpc.procedures,
						     sizeof(struct jrpc_procedure) * ctx.jsonrpc.procedure_count);
		if (!ptr)
			return -1;
		ctx.jsonrpc.procedures = ptr;

	}
	if ((ctx.jsonrpc.procedures[i].name = strdup(name)) == NULL)
		return -1;
	ctx.jsonrpc.procedures[i].function = function_pointer;
	ctx.jsonrpc.procedures[i].data = data;
	ctx.jsonrpc.procedures[i].p_names = p_names;
	ctx.jsonrpc.procedures[i].p_count = p_count;
	return 0;
}

int jrpc_deregister_procedure(char *name) {
	/* Search the procedure to deregister */
	int i;
	int found = 0;
	if (ctx.jsonrpc.procedures) {
		for (i = 0; i < ctx.jsonrpc.procedure_count; i++) {
			if (found)
				ctx.jsonrpc.procedures[i - 1] = ctx.jsonrpc.procedures[i];
			else if (!strcmp(name, ctx.jsonrpc.procedures[i].name)) {
				found = 1;
				jrpc_procedure_destroy(&(ctx.jsonrpc.procedures[i]));
			}
		}
		if (found) {
			ctx.jsonrpc.procedure_count--;
			if (ctx.jsonrpc.procedure_count) {
				struct jrpc_procedure *ptr = realloc(ctx.jsonrpc.procedures,
								     sizeof(struct jrpc_procedure) * ctx.jsonrpc.procedure_count);
				if (!ptr) {
					perror("realloc");
					return -1;
				}
				ctx.jsonrpc.procedures = ptr;
			} else {
				ctx.jsonrpc.procedures = NULL;
			}
		}
	} else {
		log_error("server : procedure '%s' not found", name);
		return -1;
	}
	return 0;
}

int parse_jsonrpc(char *buffer, size_t len, sock_data_t *fds) {
	cJSON *root;
	char *end_ptr = NULL;

	if ((root = cJSON_Parse_Stream(buffer, &end_ptr)) != NULL) {
		char *str_result = cJSON_PrintUnformatted(root);
		log_debug("Valid JSON Received: %s", str_result);
		free(str_result);

		if (root->type == cJSON_Object) {
			eval_request(root, fds);
		}
//		//shift processed request, discarding it
//		memmove(conn->buffer, end_ptr, strlen(end_ptr) + 2);
//
//		conn->pos = strlen(end_ptr);
//		memset(conn->buffer + conn->pos, 0,
//		       conn->buffer_size - conn->pos - 1);

		cJSON_Delete(root);
	} else {
		// did we parse the all buffer? If so, just wait for more.
		// else there was an error before the buffer's end
		if (end_ptr != (buffer + len)) {

			log_error("INVALID JSON Received:\n---\n%s\n---\n", buffer);

			send_error(fds, JRPC_PARSE_ERROR, strdup("Parse error. Invalid JSON was received by the server."), NULL);
			return -1;
		}
	}
	return 0;
}
