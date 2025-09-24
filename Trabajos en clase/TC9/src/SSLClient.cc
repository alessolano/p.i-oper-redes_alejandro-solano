/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** SSLSocket example 
  *
  * (Fedora version)
  *
 **/

#include <cstdlib>   // atoi, exit
#include <cstring>   // strlen
#include <cstdio>    // printf, scanf

#include "SSLSocket.h"   // use SSLSocket directly

/**
 *
 **/
int main(int cuantos, char * argumentos[] ) {
  SSLSocket * client;
  char userName[16]  = { 0 };
  char password[16]  = { 0 };
  const char * requestMessage =
    "\n<Body>\n"
    "\t<UserName>%s</UserName>\n"
    "\t<Password>%s</Password>\n"
    "</Body>\n";

  char buf[1024];
  char clientRequest[1024] = { 0 };
  int bytes;
  const char *hostname;
  const char *portnum;

  if ( cuantos != 3 ) {
    printf("usage: %s <hostname> <portnum>\n", argumentos[0] );
    return 0;
  }

  hostname = argumentos[1];
  portnum  = argumentos[2];

  // IPv6 = false by default; pass true if you need it
  client = new SSLSocket(/*IPv6=*/false);

  // TCP connect + TLS handshake
  client->MakeConnection( hostname, std::atoi(portnum) );

  printf( "Enter the User Name : " );
  std::scanf( "%15s", userName );   // width limit to avoid overflow
  printf( "\nEnter the Password : " );
  std::scanf( "%15s", password );   // width limit

  std::sprintf( clientRequest, requestMessage, userName, password ); // construct request

  const char* cipher = client->GetCipher();
  printf( "\n\nConnected with %s encryption\n", cipher ? cipher : "(unknown)" );
  client->ShowCerts();    // display any certs

  client->Write( clientRequest );     // encrypt & send message

  bytes = static_cast<int>( client->Read( buf, sizeof(buf) - 1 ) ); // leave space for '\0'
  if (bytes < 0) bytes = 0;
  buf[ bytes ] = '\0';

  printf("Received: \"%s\"\n", buf);

  client->Close();
  delete client;

  return 0;
}
