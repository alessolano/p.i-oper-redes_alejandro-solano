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


#ifdef __linux__
#include <net/if.h>   // if_nametoindex para scope-id (IPv6 link-local con %iface)
#endif

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

  // Elegir tipo de socket
  int stype = 0;
  switch (t) {
    case 's': case 'S': stype = SOCK_STREAM; break; // TCP
    case 'd': case 'D': stype = SOCK_DGRAM;  break; // UDP
    default:
      throw std::runtime_error("VSocket::BuildSocket: tipo desconocido (use 's' o 'd')");
  }

  // Crear el socket
  int fd = ::socket(domain, stype, 0);
  if (fd == -1) {
    throw std::runtime_error(std::string("VSocket::BuildSocket: socket() fallo: ")
                              + std::strerror(errno));
  }

  // Opcional: reutilizar dirección rápidamente al reejecutar
  int yes = 1;
  (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  // Opcional: si es IPv6, forzar solo-IPv6 (v6only=1) o permitir dual-stack (v6only=0)
  if (domain == AF_INET6) {
    int v6only = 1; // pon 0 si quieres aceptar IPv4-mapeado también
    (void)::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
  }

  // Guardar en el objeto
  this->idSocket = fd;
  this->IPv6     = IPv6;
  this->type     = (t == 'S') ? 's' : (t == 'D') ? 'd' : t; // normaliza a minúscula
  // Si tu clase tiene 'port', puedes inicializarlo a 0 aquí:
  // this->port = 0;

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
    // Intento de cierre ordenado (no falla si es UDP o no está conectado)
    ::shutdown(this->idSocket, SHUT_RDWR);

    int rc;
    do {
      rc = ::close(this->idSocket);
    } while (rc == -1 && errno == EINTR);

    // Marcar como cerrado para evitar doble close
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

  int st = -1;

  if (this->IPv6) {
    std::string ip(hostip);
    unsigned int scope = 0;

#ifdef __linux__
    const auto pos = ip.find('%');
    if (pos != std::string::npos) {
      std::string ifname = ip.substr(pos + 1);
      ip.erase(pos);
      scope = if_nametoindex(ifname.c_str());
    }
#endif

    sockaddr_in6 addr6{};
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET6, ip.c_str(), &addr6.sin6_addr) != 1) {
      throw std::runtime_error("EstablishConnection(hostip,port): invalid IPv6 literal");
    }
#ifdef __linux__
    if (scope != 0) {
      addr6.sin6_scope_id = scope;
    }
#endif

    do {
      st = ::connect(this->idSocket,
                     reinterpret_cast<const sockaddr*>(&addr6),
                     static_cast<socklen_t>(sizeof(addr6)));
    } while (st == -1 && errno == EINTR);
  } else {
    sockaddr_in addr4{};
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, hostip, &addr4.sin_addr) != 1) {
      throw std::runtime_error("EstablishConnection(hostip,port): invalid IPv4 literal");
    }

    do {
      st = ::connect(this->idSocket,
                     reinterpret_cast<const sockaddr*>(&addr4),
                     static_cast<socklen_t>(sizeof(addr4)));
    } while (st == -1 && errno == EINTR);
  }

  if (st == -1) {
    throw std::runtime_error(std::string("connect failed: ") + std::strerror(errno));
  }

  // Si tu clase tiene 'port', puedes actualizarlo aquí:
  // this->port = port;

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
  int stype = (this->type == 'd' || this->type == 'D') ? SOCK_DGRAM : SOCK_STREAM;
  hints.ai_socktype = stype;
  hints.ai_flags = AI_NUMERICSERV;

  addrinfo* res = nullptr;
  const int rc = ::getaddrinfo(host, service, &hints, &res);
  if (rc != 0) {
    throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));
  }

  int st = -1;
  for (addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
    do {
      st = ::connect(this->idSocket, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen));
    } while (st == -1 && errno == EINTR);

    if (st == 0) {
      // Opcional: guardar el puerto si tu clase lo expone
      // if (rp->ai_family == AF_INET)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in*>(rp->ai_addr)->sin_port);
      // else if (rp->ai_family == AF_INET6)
      //   this->port = ntohs(reinterpret_cast<sockaddr_in6*>(rp->ai_addr)->sin6_port);
      break;
    }
  }

  ::freeaddrinfo(res);

  if (st == -1) {
    throw std::runtime_error(std::string("connect(getaddrinfo) failed: ") + std::strerror(errno));
  }

  return 0;

}

