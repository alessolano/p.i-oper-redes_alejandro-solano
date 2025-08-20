// net_test.cc
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include "VSocket.h"
#include "Socket.h"

// Devuelve host formateado para el header Host: (brackets en IPv6, sin %iface)
static std::string host_for_header(const std::string& host, bool ipv6) {
  std::string h = host;
  if (ipv6) {
    // Elimina el scope-id para el header (e.g., "%enp0s31f6")
    auto pos = h.find('%');
    if (pos != std::string::npos) h = h.substr(0, pos);
    // Coloca brackets si hay ':'
    if (!h.empty() && h.front() != '[' && h.find(':') != std::string::npos) {
      h = "[" + h + "]";
    }
  }
  return h;
}

static std::string build_request(const std::string& host, const std::string& service,
                                 const std::string& path, bool ipv6) {
  std::string h = host_for_header(host, ipv6);
  std::string req;
  req += "GET " + (path.empty() ? std::string("/") : path) + " HTTP/1.1\r\n";
  // Si el puerto no es 80, agrega ":puerto" para ser explícitos
  req += "Host: " + h;
  if (service != "80") req += ":" + service;
  req += "\r\n";
  req += "User-Agent: CI0123-Test/1.0\r\n";
  req += "Connection: close\r\n";
  req += "\r\n";
  return req;
}

static void usage(const char* prog) {
  std::cerr
    << "Uso:\n"
    << "  " << prog << " v4 <host> <port> [path]\n"
    << "  " << prog << " v6 <host> <service> [path]\n\n"
    << "Ejemplos:\n"
    << "  " << prog << " v4 163.178.104.62 80 /\n"
    << "  " << prog << " v6 ::1 8080 /ip\n"
    << "  " << prog << " v6 fe80::4161:e292:8c1d:e3c0%enp0s31f6 8080 /\n";
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
    return 1;
  }

  const std::string fam = argv[1];
  const bool useIPv6 = (fam == "v6" || fam == "V6" || fam == "ipv6");
  if (!(useIPv6 || fam == "v4" || fam == "V4" || fam == "ipv4")) {
    usage(argv[0]);
    return 1;
  }

  const std::string host = argv[2];
  const std::string service = argv[3];
  const std::string path = (argc >= 5) ? argv[4] : "/";

  try {
    // Crea socket TCP ('s') en la familia indicada
    Socket s('s', useIPv6);

    // Conecta (acepta servicio por nombre: "http" o por número: "8080")
    s.MakeConnection(host.c_str(), service.c_str());

    // Construye y envía request
    const std::string req = build_request(host, service, path, useIPv6);
    s.Write(req.c_str(), req.size());

    // Lee y muestra toda la respuesta
    char buf[2048];
    for (;;) {
      size_t n = s.Read(buf, sizeof(buf) - 1);
      if (n == 0) break;       // EOF (o no hay más datos)
      buf[n] = '\0';
      std::fputs(buf, stdout);
    }
    std::puts("");

  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
