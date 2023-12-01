/*
 * MonthPeakQuerier.h
 *
 *  Created on: 25 feb. 2023
 *  Author: Wim
 */

#ifndef MONTPEAKQ_H_
#define MONTPEAKQ_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "fixed_quarter_power_history_accumulator.h"


class MonthPeakQuerier {
    AsyncWebServer& server;
    const FixedQuarterPowerHistoryAccumulator& watt_history;
    
public:
    MonthPeakQuerier(AsyncWebServer& server, const FixedQuarterPowerHistoryAccumulator& watt_history)  : 
      server(server), watt_history(watt_history) {}

    void Init() {
        server.on("/current/month/peak", HTTP_GET, [this](AsyncWebServerRequest *request) {
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<256> doc;

            JsonObject memory = doc.createNestedObject("memory");
            memory["size_heap"] = ESP.getHeapSize();
            memory["free_heap"] = ESP.getFreeHeap();
            memory["free_sketch"] = ESP.getFreeSketchSpace();
            memory["size_sketch"] = ESP.getSketchSize();
            memory["size_psram"] = ESP.getPsramSize();
            memory["free_psram"] = ESP.getFreePsram();

            int quarter_id = watt_history.GetMonthQuarterPeakId();
            if (quarter_id == 0) {
                // Monthly info no known yet (typically this means no communication was received from the meter than)
                serializeJson(doc, *response);
                request->send(response);
                return;
            }

            JsonObject peak = doc.createNestedObject("peak");
            peak["quarter_id"] = quarter_id;
            peak["power"] = watt_history.GetMonthQuarterPeakPower();
            
            serializeJson(doc, *response);
            request->send(response);
        });   
    }
};


extern MonthPeakQuerier& GetMonthPeakQuerier(AsyncWebServer& server);
#endif 