/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** SSLSocket example, server code
  *
  * (Fedora version)
  *
 **/

#include <thread>
#include <cstdlib>  // atoi
#include <cstdio>   // printf
#include <cstring>  // strlen, strcmp
#include <iostream>

#include "SSLSocket.h"

#define PORT 4321

static void Service( SSLSocket * client ) {
  try {
    char buf[1024] = {0};
    const char* ServerResponse =
      "\n<Body>\n"
      "\t<Server>os.ecci.ucr.ac.cr</Server>\n"
      "\t<dir>ci0123</dir>\n"
      "\t<Name>Proyecto Integrador Redes y sistemas Operativos</Name>\n"
      "\t<NickName>PIRO</NickName>\n"
      "\t<Description>Consolidar e integrar los conocimientos de redes y sistemas operativos</Description>\n"
      "\t<Author>profesores PIRO</Author>\n"
      "</Body>\n";

    const char* validMessage =
      "\n<Body>\n"
      "\t<UserName>piro</UserName>\n"
      "\t<Password>ci0123</Password>\n"
      "</Body>\n";

    client->HandshakeAsServer();     // TLS server-side handshake
    client->ShowCerts();

    size_t bytes = client->Read(buf, sizeof(buf) - 1);
    buf[bytes] = '\0';
    printf("Client msg: \"%s\"\n", buf);

    if (std::strcmp(validMessage, buf) == 0) {
      client->Write(ServerResponse, std::strlen(ServerResponse));
    } else {
      const char* msg = "Invalid Message";
      client->Write(msg, std::strlen(msg));
    }
  } catch (const std::exception& e) {
    std::cerr << "[Service] error: " << e.what() << "\n";
  }

  client->Close();
  delete client; // end of connection
}

int main( int cuantos, char ** argumentos ) {
  int port = PORT;
  if (cuantos > 1) port = std::atoi(argumentos[1]);

  try {
    // listener: loads server cert/key and prepares TLS server context
    SSLSocket* server = new SSLSocket((const char*)"ci0123.pem", (const char*)"key0123.pem", false);
    server->Bind(port);
    server->MarkPassive(10);

    for (;;) {
      // accept TCP connection (fd only)
      int fd = server->AcceptConnection();

      // wrap fd in SSLSocket and share TLS context from listener
      SSLSocket* client = new SSLSocket(fd);
      client->Copy(server); // share SSL_CTX and set SSL* on client's fd

      // serve connection in a detached thread
      std::thread(Service, client).detach();
    }

    // unreachable in this simple loop:
    // server->Close();
    // delete server;

  } catch (const std::exception& e) {
    std::cerr << "[main] fatal: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
