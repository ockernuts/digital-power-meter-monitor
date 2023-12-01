/*
 * JsonQuarterQuerier.h
 *
 *  Created on: 21 feb. 2023
 *  Author: Wim
 */

#ifndef JSONQUARTERQUERIER_H_
#define JSONQUARTERQUERIER_H_

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "json_quarter_info_creator.h"

class JsonQuarterQuerier {
    AsyncWebServer& server;
    JsonQuarterInfoCreator& quarter_info_creator; 
    
public:
    JsonQuarterQuerier(AsyncWebServer& server, JsonQuarterInfoCreator& quarter_info_creator)  : 
      server(server), quarter_info_creator(quarter_info_creator) {}

    void Init() {
        server.on("/current/quarter", HTTP_GET, [this](AsyncWebServerRequest *request) {
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            DynamicJsonDocument doc(6144); // We could not use a StaticJsonDocument of this size, since it ruïnes the limited stack (4KB) on the ESP8266
            JsonObject root = doc.to<JsonObject>();
            quarter_info_creator.FillJsonObjectForLastQuarterQueryResult(root);
            serializeJson(root, *response);
            request->send(response);
        });   
    }
};

extern JsonQuarterQuerier& GetJsonQuarterQuerier(AsyncWebServer& server);

#endif 