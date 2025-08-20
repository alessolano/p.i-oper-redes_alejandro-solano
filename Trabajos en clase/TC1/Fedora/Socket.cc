/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *  ******   Socket class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>         // sockaddr_in
#include <arpa/inet.h>          // ntohs
#include <unistd.h>		// write, read
#include <cstring>
#include <stdexcept>
#include <stdio.h>		// printf

#include "Socket.h"		// Derived class

/**
  *  Class constructor
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
Socket::Socket( char t, bool IPv6 ){

   this->BuildSocket( t, IPv6 );      // Call base class constructor

}


/**
  *  Class destructor
  *
  *  @param     int id: socket descriptor
  *
 **/
Socket::~Socket() {

}


/**
  * MakeConnection method
  *   use "EstablishConnection" in base class
  *
  * @param      char * host: host address in dot notation, example "10.1.166.62"
  * @param      int port: process address, example 80
  *
 **/
int Socket::MakeConnection( const char * hostip, int port ) {

   return this->EstablishConnection( hostip, port );

}


/**
  * MakeConnection method
  *   use "EstablishConnection" in base class
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int Socket::MakeConnection( const char *host, const char *service ) {

   return this->EstablishConnection( host, service );

}


/**
  * Read method
  *   use "read" Unix system call (man 3 read)
  *
  * @param      void * buffer: buffer to store data read from socket
  * @param      int size: buffer capacity, read will stop if buffer is full
  *
 **/
size_t Socket::Read( void * buffer, size_t size ) {
  if (this->idSocket < 0) {
      throw std::runtime_error("Read: invalid socket descriptor");
  }
  if (buffer == nullptr) {
      throw std::runtime_error("Read: null buffer");
  }
  if (size == 0) {
      return 0; // nada que leer
  }

  for (;;) {
      ssize_t n = ::read(this->idSocket, buffer, size);

      if (n > 0) {
          return static_cast<size_t>(n);  // bytes leídos
      }
      if (n == 0) {
          return 0; // EOF (el peer cerró ordenadamente)
      }

      // n < 0 → error
      if (errno == EINTR) {
          continue; // reintentar si fue interrumpido por una señal
      }

      // Si el socket está en modo no bloqueante, devolver 0 cuando "no hay datos aún"
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return 0;
      }

      // Cualquier otro error se reporta como excepción
      throw std::runtime_error(std::string("Read failed: ") + std::strerror(errno));
  }

}


/**
  * Write method
  *   use "write" Unix system call (man 3 write)
  *
  * @param      void * buffer: buffer to store data write to socket
  * @param      size_t size: buffer capacity, number of bytes to write
  *
 **/
size_t Socket::Write( const void * buffer, size_t size ) {

  if (this->idSocket < 0) {
    throw std::runtime_error("Write: invalid socket descriptor");
  }
  if (buffer == nullptr) {
    throw std::runtime_error("Write: null buffer");
  }
  if (size == 0) {
    return 0; // nada que escribir
  }

  const char* p = static_cast<const char*>(buffer);
  size_t total = 0;

  for (;;) {
    ssize_t n = ::write(this->idSocket, p + total, size - total);

    if (n > 0) {
      total += static_cast<size_t>(n);
      if (total == size) {
        return total; // listo
      }
          // si no bloqueante, puede seguir escribiendo más en siguientes iteraciones
        continue;
    }

    if (n == -1) {
      if (errno == EINTR) {
        continue; // reintentar si fue interrumpido por señal
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // socket no bloqueante: si ya escribimos algo, devolvemos el parcial
        return total;
      }
      throw std::runtime_error(std::string("Write failed: ") + std::strerror(errno));
    }

      // n == 0 es inusual en write; tratamos como “no progresó”
    return total;
  }

}


/**
  * Write method
  *   use "write" Unix system call (man 3 write)
  *
  * @param      char * text: text to write to socket
  *
 **/
size_t Socket::Write( const char * text ) {

    if (text == nullptr) {
      throw std::runtime_error("Write(text): null pointer");
    }

    return Write(text, std::strlen(text));

}

