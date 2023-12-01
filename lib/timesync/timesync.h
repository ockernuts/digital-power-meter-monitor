#pragma once

#include "idisplayer.h"
#include <Arduino.h>
#include "time.h"


extern uint32_t sntp_update_delay_MS_rfc_not_less_than_15000();

extern void setupTimeSync(IDisplayer &displayer);