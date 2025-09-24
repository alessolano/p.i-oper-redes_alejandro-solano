// src/SSLClient.c
/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** SSLSocket example - C client (OpenSSL)
  *
  * (Fedora/Ubuntu compatible)
  *
 **/

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

static int open_connection(const char *host, const char *port) {
  struct addrinfo hints, *res = NULL, *rp = NULL;
  int sfd = -1, rc;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;      // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;    // TCP
  hints.ai_flags    = AI_ADDRCONFIG;

  rc = getaddrinfo(host, port, &hints, &res);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo(%s,%s): %s\n", host, port, gai_strerror(rc));
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) continue;

    if (connect(sfd, rp->ai_addr, (socklen_t)rp->ai_addrlen) == 0) break; // success

    close(sfd);
    sfd = -1;
  }

  freeaddrinfo(res);

  if (sfd == -1) {
    fprintf(stderr, "connect: %s\n", strerror(errno));
    return -1;
  }
  return sfd;
}

static SSL_CTX* init_ctx(void) {
  const SSL_METHOD *method = NULL;
  SSL_CTX *ctx = NULL;

  // OpenSSL 1.1+ auto-inits, but keep for portability:
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  method = TLS_client_method();
  ctx = SSL_CTX_new(method);
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Optional: enforce min protocol
  // SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

  // Optional: load system CAs to verify server (demo omits verification)
  // if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
  //   ERR_print_errors_fp(stderr);
  // }

  return ctx;
}

static void show_peer_certs(SSL *ssl) {
  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) {
    printf("Info: No peer certificate.\n");
    return;
  }
  char *subj = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
  char *issu = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
  printf("Server certificates:\n");
  printf("Subject: %s\n", subj ? subj : "(null)");
  printf("Issuer : %s\n", issu ? issu : "(null)");
  OPENSSL_free(subj);
  OPENSSL_free(issu);
  X509_free(cert);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("usage: %s <hostname> <port>\n", argv[0]);
    return 0;
  }
  const char *host = argv[1];
  const char *port = argv[2];

  SSL_CTX *ctx = init_ctx();
  if (!ctx) return 1;

  int sfd = open_connection(host, port);
  if (sfd < 0) {
    SSL_CTX_free(ctx);
    return 1;
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }

  if (SSL_set_fd(ssl, sfd) != 1) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }

  // TLS handshake (client)
  for (;;) {
    int rc = SSL_connect(ssl);
    if (rc == 1) break;
    int err = SSL_get_error(ssl, rc);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }

  const char *cipher = SSL_get_cipher(ssl);
  printf("\nConnected with %s encryption\n", cipher ? cipher : "(unknown)");
  show_peer_certs(ssl);

  char user[16] = {0};
  char pass[16] = {0};
  printf("Enter the User Name : ");
  scanf("%15s", user);
  printf("\nEnter the Password : ");
  scanf("%15s", pass);

  const char *tmpl =
      "\n<Body>\n"
      "\t<UserName>%s</UserName>\n"
      "\t<Password>%s</Password>\n"
      "</Body>\n";

  char req[1024];
  int n = snprintf(req, sizeof(req), tmpl, user, pass);
  if (n < 0 || (size_t)n >= sizeof(req)) {
    fprintf(stderr, "request too long\n");
    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }

  // send request
  size_t sent = 0;
  while (sent < (size_t)n) {
    int rc = SSL_write(ssl, req + sent, n - (int)sent);
    if (rc > 0) { sent += (size_t)rc; continue; }
    int err = SSL_get_error(ssl, rc);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }

  // read reply
  char buf[1024];
  int r = SSL_read(ssl, buf, (int)sizeof(buf) - 1);
  if (r < 0) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);
    return 1;
  }
  buf[(r < 0) ? 0 : r] = '\0';
  printf("Received: \"%s\"\n", buf);

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(sfd);
  SSL_CTX_free(ctx);
  return 0;
}
