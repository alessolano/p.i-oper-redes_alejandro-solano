/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>			// getaddrinfo, freeaddrinfo
#include <unistd.h>			// close

/*
#include <cstddef>
#include <cstdio>



//#include <sys/types.h>
*/
#include "VSocket.h"


/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::BuildSocket( char t, bool IPv6 ) {

  const int domain = IPv6 ? AF_INET6 : AF_INET;

  int stype = 0;
  switch (t) {
    case 's': case 'S': stype = SOCK_STREAM; break; // TCP
    case 'd': case 'D': stype = SOCK_DGRAM;  break; // UDP
    default:
      throw std::runtime_error("VSocket::BuildSocket: tipo desconocido (use 's' o 'd')");
  }

  int fd = ::socket(domain, stype, 0);
  if (fd == -1) {
    throw std::runtime_error(std::string("VSocket::BuildSocket: socket() fallo: ")
                              + std::strerror(errno));
  }

  int yes = 1;
  (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  if (domain == AF_INET6) {
    int v6only = 1;
    (void)::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
  }

  this->idSocket = fd;
  this->IPv6     = IPv6;
  this->type     = (t == 'S') ? 's' : (t == 'D') ? 'd' : t;


}


/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {

   this->Close();

}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close() {

   if (this->idSocket >= 0) {
    ::shutdown(this->idSocket, SHUT_RDWR);

    int rc;
    do {
      rc = ::close(this->idSocket);
    } while (rc == -1 && errno == EINTR);

    this->idSocket = -1;
    this->port = 0;
  }

}


/**
  * EstablishConnection method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::EstablishConnection( const char * hostip, int port ) {

  if (this->idSocket < 0) {
    throw std::runtime_error("EstablishConnection(hostip,port): invalid socket descriptor");
  }
  if (!hostip) {
    throw std::runtime_error("EstablishConnection(hostip,port): null hostip");
  }
  if (port <= 0 || port > 65535) {
    throw std::runtime_error("EstablishConnection(hostip,port): invalid port");
  }

  char svc[16];
  std::snprintf(svc, sizeof(svc), "%d", port);

  addrinfo hints{};
  hints.ai_family = this->IPv6 ? AF_INET6 : AF_INET;
  hints.ai_socktype = (this->type == 'd' || this->type == 'D') ? SOCK_DGRAM : SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  addrinfo* res = nullptr;
  int rc = ::getaddrinfo(hostip, svc, &hints, &res);
  if (rc != 0) {
    if (rc == EAI_SYSTEM) {
      throw std::runtime_error(std::string("getaddrinfo: ") + std::strerror(errno));
    }
    throw std::runtime_error(std::string("getaddrinfo: ") + ::gai_strerror(rc));
  }

  int st = -1;
  for (addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
    do {
      st = ::connect(this->idSocket, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen));
    } while (st == -1 && errno == EINTR);

    if (st == 0) {
      // if (rp->ai_family == AF_INET)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in*>(rp->ai_addr)->sin_port);
      // else if (rp->ai_family == AF_INET6)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in6*>(rp->ai_addr)->sin6_port);
      break;
    }
  }

  ::freeaddrinfo(res);

  if (st == -1) {
    throw std::runtime_error(std::string("connect failed: ") + std::strerror(errno));
  }
  return 0;

}


/**
  * EstablishConnection method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::EstablishConnection( const char *host, const char *service ) {

  if (this->idSocket < 0) {
    throw std::runtime_error("EstablishConnection(host,service): invalid socket descriptor");
  }
  if (!host || !service) {
    throw std::runtime_error("EstablishConnection(host,service): null host/service");
  }

  addrinfo hints{};
  hints.ai_family = this->IPv6 ? AF_INET6 : AF_INET;
  // hints.ai_family = AF_UNSPEC;

  hints.ai_socktype = (this->type == 'd' || this->type == 'D') ? SOCK_DGRAM : SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG;

  addrinfo* res = nullptr;
  int rc = ::getaddrinfo(host, service, &hints, &res);
  if (rc != 0) {
    if (rc == EAI_SYSTEM) {
      throw std::runtime_error(std::string("getaddrinfo: ") + std::strerror(errno));
    }
    throw std::runtime_error(std::string("getaddrinfo: ") + ::gai_strerror(rc));
  }

  int st = -1;
  for (addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
    do {
      st = ::connect(this->idSocket, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen));
    } while (st == -1 && errno == EINTR);

    if (st == 0) {
      // if (rp->ai_family == AF_INET)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in*>(rp->ai_addr)->sin_port);
      // else if (rp->ai_family == AF_INET6)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in6*>(rp->ai_addr)->sin6_port);
      break;
    }
  }

  ::freeaddrinfo(res);

  if (st == -1) {
    throw std::runtime_error(std::string("connect failed: ") + std::strerror(errno));
  }
  return 0;

}

