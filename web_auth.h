#pragma once
#include <Arduino.h>
#include <functional>

// True when the request carries valid credentials, or when auth is disabled
// (WEB_AUTH_USER / WEB_AUTH_PASS empty in config.h).
bool webAuthorized();

// Register a route that is gated by webAuthorized(). Use instead of server.on()
// for every UI/API endpoint so authentication cannot be forgotten.
void webRoute(const char* path, std::function<void()> handler);
