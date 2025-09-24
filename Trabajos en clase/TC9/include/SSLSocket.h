/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *   SSL Socket class interface
  *
  * (Fedora version)
  *
 **/

#ifndef SSLSocket_h
#define SSLSocket_h

#include <openssl/ssl.h>   // SSL types
#include <openssl/err.h>   // error strings

#include "VSocket.h"

class SSLSocket : public VSocket {

   public:
      SSLSocket( bool IPv6 = false );                    // TCP client constructor (no UDP)
      SSLSocket( const char * , const char * , bool = false ); // Server: cert, key, IPv6
      SSLSocket( int );                                  // Wrap accepted fd (server-side)
      ~SSLSocket();

      int MakeConnection( const char * , int );          // connect + TLS handshake (client)
      int MakeConnection( const char * , const char * ); // connect by service + TLS handshake

      size_t Write( const char * );                      // SSL_write (null-terminated text)
      size_t Write( const void * , size_t );             // SSL_write (raw buffer)
      size_t Read( void * , size_t );                    // SSL_read

      void ShowCerts();                                   // print peer certificate info
      const char * GetCipher();                           // return negotiated cipher name

      void HandshakeAsServer();
      void Copy(const SSLSocket* src);

   private:
      void Init( bool = false );                          // false: client ctx, true: server ctx
      void InitContext( bool );                           // create SSL_CTX with TLS_(client|server)_method
      void LoadCertificates( const char * , const char * ); // load server cert/key into context

      // Instance variables
      SSL_CTX * SSLContext = nullptr;                    // TLS context
      SSL     * SSLStruct  = nullptr;                    // TLS session bound to socket fd
};

#endif
