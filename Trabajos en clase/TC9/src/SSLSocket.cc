/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *  Socket class implementation
  *
  * (Fedora version)
  *
 **/
 
// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <limits>
#include <stdexcept>
#include <cstring>   // strlen
#include <cstdio>    // printf
#include <cerrno>    // errno
#include <cstdint>   // uint8_t

#include "SSLSocket.h"
#include "Socket.h"

// small helper: format last OpenSSL error message
static const char* ssl_last_error_string() {
  static char buf[256];
  unsigned long e = ERR_get_error();
  if (e == 0) return "no SSL error";
  ERR_error_string_n(e, buf, sizeof(buf));
  return buf;
}

// small helper: throw with both errno and SSL error when applicable
static void throw_ssl_error(const char* where) {
  throw std::runtime_error(std::string(where) + ": " + ssl_last_error_string());
}

/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( bool IPv6 ) {

  this->BuildSocket( 's', IPv6 );

  this->SSLContext = nullptr;
  this->SSLStruct  = nullptr;

  this->Init();  // Initializes to client context
}


/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool IPv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( const char * certFileName, const char * keyFileName, bool IPv6 ) {

  this->BuildSocket( 's', IPv6 );

  this->SSLContext = nullptr;
  this->SSLStruct  = nullptr;

  this->Init( true );                          // server context
  this->LoadCertificates( certFileName, keyFileName );  // load cert/key
}


/**
  *  Class constructor
  *
  *  @param     int id: socket descriptor
  *
 **/
SSLSocket::SSLSocket( int id ) {

  this->BuildSocket( id );

  this->SSLContext = nullptr;
  this->SSLStruct  = nullptr;

  this->Init(true);  // server context for accepted sockets  // default to client context; caller may use server ctor instead

  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );
  int ok = SSL_set_fd( s, this->idSocket ); // bind fd now
  if (ok != 1) {
    throw_ssl_error("SSLSocket(int)::SSL_set_fd");
  }
}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

  // SSL destroy
  if ( this->SSLStruct != nullptr ) {
    SSL_free( reinterpret_cast<SSL*>( this->SSLStruct ) );
    this->SSLStruct = nullptr;
  }
  if ( this->SSLContext != nullptr ) {
    SSL_CTX_free( reinterpret_cast<SSL_CTX*>( this->SSLContext ) );
    this->SSLContext = nullptr;
  }

  this->Close();
}


/**
  *  SSLInit
  *     use SSL_new with a defined context
  *  Create a SSL object
  *
 **/
void SSLSocket::Init( bool serverContext ) {

  this->InitContext( serverContext );

  // Create the SSL* from context and attach the fd.
  SSL * s = SSL_new( reinterpret_cast<SSL_CTX*>( this->SSLContext ) );
  if (!s) {
    throw_ssl_error("SSLSocket::Init::SSL_new");
  }
  this->SSLStruct = s;

  int ok = SSL_set_fd( s, this->idSocket ); // bind current socket fd
  if (ok != 1) {
    throw_ssl_error("SSLSocket::Init::SSL_set_fd");
  }
}


/**
  *  InitContext
  *     use SSL_library_init, OpenSSL_add_all_algorithms, SSL_load_error_strings, TLS_server_method, SSL_CTX_new
  *
  *  Creates a new SSL server context to start encrypted comunications, this context is stored in class instance
  *
 **/
void SSLSocket::InitContext( bool serverContext ) {
  const SSL_METHOD * method = nullptr;
  SSL_CTX * context = nullptr;

  // OpenSSL library initialization (safe to call multiple times in 1.1+; kept for portability)
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  if ( serverContext ) {
    method = TLS_server_method();   // server side
  } else {
    method = TLS_client_method();   // client side
  }

  if ( method == nullptr ) {
    throw_ssl_error("SSLSocket::InitContext::method");
  }

  context = SSL_CTX_new( method );
  if ( context == nullptr ) {
    throw_ssl_error("SSLSocket::InitContext::SSL_CTX_new");
  }

  // Optional hardening example:
  // SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);

  this->SSLContext = context;
}


/**
 *  Load certificates
 *    verify and load certificates
 *
 *  @param	const char * certFileName, file containing certificate
 *  @param	const char * keyFileName, file containing keys
 *
 **/
void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName ) {

  SSL_CTX * ctx = reinterpret_cast<SSL_CTX*>( this->SSLContext );
  if (!ctx) {
    throw std::runtime_error("SSLSocket::LoadCertificates: context not initialized");
  }

  // Load server certificate
  if ( SSL_CTX_use_certificate_file( ctx, certFileName, SSL_FILETYPE_PEM ) <= 0 ) {
    throw_ssl_error("SSLSocket::LoadCertificates::use_certificate_file");
  }

  // Load server private key
  if ( SSL_CTX_use_PrivateKey_file( ctx, keyFileName, SSL_FILETYPE_PEM ) <= 0 ) {
    throw_ssl_error("SSLSocket::LoadCertificates::use_PrivateKey_file");
  }

  // Check that private key matches the certificate
  if ( !SSL_CTX_check_private_key( ctx ) ) {
    throw std::runtime_error("SSLSocket::LoadCertificates: private key does not match the certificate public key");
  }
}
 

/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	int port, service number
 *
 **/
int SSLSocket::MakeConnection( const char * hostName, int port ) {
  int st;

  // Establish a TCP connection first using base class.
  st = VSocket::EstablishConnection( hostName, port );
  if ( st != 0 ) {
    return st;
  }

  // Perform TLS handshake (client side).
  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );
  for (;;) {
    int rc = SSL_connect( s );
    if ( rc == 1 ) break; // success
    int err = SSL_get_error( s, rc );
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      continue; // retry
    }
    throw_ssl_error("SSLSocket::MakeConnection(host,port)::SSL_connect");
  }

  return 0;
}


/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	char * service, service name
 *
 **/
int SSLSocket::MakeConnection( const char * host, const char * service ) {
  int st;

  // Establish a TCP connection first using base class.
  st = VSocket::EstablishConnection( host, service );
  if ( st != 0 ) {
    return st;
  }

  // Perform TLS handshake (client side).
  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );
  for (;;) {
    int rc = SSL_connect( s );
    if ( rc == 1 ) break;
    int err = SSL_get_error( s, rc );
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      continue;
    }
    throw_ssl_error("SSLSocket::MakeConnection(host,service)::SSL_connect");
  }

  return 0;
}


/**
  *  Read
  *     use SSL_read to read data from an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity read
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Read( void * buffer, size_t size ) {
  if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
    throw std::overflow_error("Buffer size bigger than reading capacity");
  }

  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );
  for (;;) {
    int rc = SSL_read( s, buffer, static_cast<int>(size) );
    if (rc > 0) return static_cast<size_t>(rc);
    int err = SSL_get_error( s, rc );
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      continue; // try again
    }
    if (rc == 0) {
      // orderly shutdown from peer
      return 0;
    }
    throw_ssl_error("SSLSocket::Read");
  }
}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Writes data to a secure channel
  *
 **/
size_t SSLSocket::Write( const char * string ) {
  if (nullptr == string) return 0;
  return this->Write( static_cast<const void*>(string), std::strlen(string) );
}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Write( const void * buffer, size_t size ) {
  if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
    throw std::overflow_error("Buffer size bigger than writing capacity");
  }

  const uint8_t* p = static_cast<const uint8_t*>(buffer);
  size_t total = 0;
  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );

  while (total < size) {
    int rc = SSL_write( s, p + total, static_cast<int>(size - total) );
    if (rc > 0) {
      total += static_cast<size_t>(rc);
      continue;
    }
    int err = SSL_get_error( s, rc );
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      continue; // retry
    }
    throw_ssl_error("SSLSocket::Write");
  }

  return total;
}


/**
 *   Show SSL certificates
 *
 **/
void SSLSocket::ShowCerts() {
  X509 *cert;
  char *line;

  cert = SSL_get_peer_certificate( reinterpret_cast<SSL *>( this->SSLStruct ) ); // Get certificates (if available)
  if ( nullptr != cert ) {
    printf("Server certificates:\n");
    line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
    printf( "Subject: %s\n", line );
    free( line );
    line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
    printf( "Issuer: %s\n", line );
    free( line );
    X509_free( cert );
  } else {
    printf( "No certificates.\n" );
  }
}


/**
 *   Return the name of the currently used cipher
 *
 **/
const char * SSLSocket::GetCipher() {
  return SSL_get_cipher( reinterpret_cast<SSL *>( this->SSLStruct ) );
}

void SSLSocket::HandshakeAsServer() {
  SSL* s = reinterpret_cast<SSL*>( this->SSLStruct );
  if (!s) throw std::runtime_error("SSLSocket::HandshakeAsServer: SSL not initialized");

  for (;;) {
    int rc = SSL_accept(s);
    if (rc == 1) break; // success
    int err = SSL_get_error(s, rc);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
    throw_ssl_error("SSLSocket::HandshakeAsServer::SSL_accept");
  }
}

void SSLSocket::Copy(const SSLSocket* src) {
  if (!src || !src->SSLContext) throw std::runtime_error("Copy: invalid source context");
  // free any existing
  if (this->SSLStruct) { SSL_free(reinterpret_cast<SSL*>(this->SSLStruct)); this->SSLStruct = nullptr; }
  if (this->SSLContext) { SSL_CTX_free(reinterpret_cast<SSL_CTX*>(this->SSLContext)); this->SSLContext = nullptr; }
  // share ctx (increase refcount)
  SSL_CTX* ctx = reinterpret_cast<SSL_CTX*>(src->SSLContext);
  if (SSL_CTX_up_ref(ctx) != 1) throw_ssl_error("Copy::SSL_CTX_up_ref");
  this->SSLContext = ctx;
  // build SSL* for this fd
  SSL* s = SSL_new(ctx);
  if (!s) throw_ssl_error("Copy::SSL_new");
  this->SSLStruct = s;
  if (SSL_set_fd(s, this->idSocket) != 1) throw_ssl_error("Copy::SSL_set_fd");
}

