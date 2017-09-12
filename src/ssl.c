#include "ssl.h"

int create_socket(int port) {
	int s;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("Unable to create socket");
		exit(EXIT_FAILURE);
	}

	if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("Unable to bind");
		exit(EXIT_FAILURE);
	}

	if (listen(s, 1) < 0) {
		perror("Unable to listen");
		exit(EXIT_FAILURE);
	}

	return s;
}

void init_openssl() {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
	EVP_cleanup();
}

SSL_CTX *create_context() {
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = SSLv23_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	return ctx;
}

void configure_context(SSL_CTX *ctx) {
//    SSL_CTX_set_ecdh_auto(ctx, 1);

	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
//	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
//	SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1);
//	SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1);


	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, "/home/dk/devel/netexec/cert.crt", SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "/home/dk/devel/netexec/cert.key", SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

}



