/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** VSocket base class interface
  *
  * (Fedora version)
  *
 **/

#include <cstddef>
#ifndef VSocket_h
#define VSocket_h
 
class VSocket {
 public:
  void BuildSocket( char, bool = false );
  void BuildSocket( int );                    // wrap an accepted fd
  virtual ~VSocket();                         // virtual: delete via base pointer

  void Close();
  int Bind( int );                            // server: bind local port
  int MarkPassive( int );                     // server: listen(backlog)
  int AcceptConnection();                     // server: accept(), returns fd

  int EstablishConnection( const char *, int );
  int EstablishConnection( const char *, const char * );

  virtual int MakeConnection( const char *, int ) = 0;
  virtual int MakeConnection( const char *, const char * ) = 0;

  virtual size_t Read( void *, size_t ) = 0;
  virtual size_t Write( const void *, size_t ) = 0;
  virtual size_t Write( const char * ) = 0;

 protected:
  int  idSocket = -1;   // Socket identifier
  bool IPv6     = false; // Is IPv6 socket?
  int  port     = 0;     // Socket associated port
  char type     = 0;     // Socket type (datagram, stream, etc.)
};

#endif // VSocket_h
