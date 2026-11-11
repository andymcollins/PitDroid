#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "web_auth.h"

extern WebServer server;

// sizeof("") == 1, so an empty define leaves auth disabled.
static constexpr bool AUTH_ENABLED = sizeof(WEB_AUTH_USER) > 1;

bool webAuthorized() {
  if (!AUTH_ENABLED) return true;
  return server.authenticate(WEB_AUTH_USER, WEB_AUTH_PASS);
}

void webRoute(const char* path, std::function<void()> handler) {
  server.on(path, [handler]() {
    if (!webAuthorized()) {
      server.requestAuthentication();
      return;
    }
    handler();
  });
}
