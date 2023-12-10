
#pragma once
#include <ESPAsyncWebServer.h>
#include "fixed_quarter_power_history_accumulator.h"

extern const char *realm;
extern void InitHttpHandlers(AsyncWebServer &server);