#include "ssl.h"

extern ctx_t ctx;

void init_openssl() {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
//	CONF_modules_free();
//	ENGINE_cleanup();
//	CONF_modules_unload(1);
	FIPS_mode_set(0);
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);
	ERR_free_strings();
	CRYPTO_cleanup_all_ex_data();
	EVP_cleanup();
	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	SSL_CTX_free(ctx.ssl_ctx);
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



