// webserver.h
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "config.h"

#ifdef ESP8266
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebSrv;
#else
  #include <WebServer.h>
  typedef WebServer WebSrv;
#endif

// Inicializa todas as rotas HTTP no servidor
void initWebServer(WebSrv& server, Config& cfg);

#endif // WEBSERVER_H
