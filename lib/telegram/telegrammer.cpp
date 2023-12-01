/*
 * telegrammer.cpp
 *
 *  Created on: 15 mar. 2023
 *      Author: Wim
 */

#include "telegrammer.h"
#include "prefs.h"
#include <LittleFS.h>

// Web page parameters
#define P_BOTTOKEN "TOKEN"
#define P_USERID1 "USERID1"
#define P_USERID2 "USERID2"
#define P_USERID3 "USERID3"
#define P_USERID4 "USERID4"
#define P_ALARMWATTS "ALARMWATTS"

// Initialization if static wifiConfigInfo member. (otherwise linker error).
TelegramInfoStruct Telegrammer::telegrammerInfo;


Telegrammer::Telegrammer(AsyncWebServer& server, IDisplayer& displayer) :
    server(server), displayer(displayer), my_bot(client)
{
}

Telegrammer::~Telegrammer()
{
}




void Telegrammer::WritePreferences() {
    prefs.begin("telegrammer", false);
    prefs.putString("bottoken", telegrammerInfo.bottoken);
    prefs.putString("userid1", telegrammerInfo.userid1);
    prefs.putString("userid2", telegrammerInfo.userid2);
    prefs.putString("userid3", telegrammerInfo.userid3);
    prefs.putString("userid4", telegrammerInfo.userid4);
    prefs.putInt("alarmwatts", telegrammerInfo.alarmwatts);
}

void Telegrammer::Init() {
    prefs.begin("telegrammer", true);
    int len_bottoken = prefs.getString("bottoken", telegrammerInfo.bottoken, sizeof(telegrammerInfo.bottoken));
    prefs.getString("userid1", telegrammerInfo.userid1, sizeof(telegrammerInfo.userid1));
    prefs.getString("userid2", telegrammerInfo.userid2, sizeof(telegrammerInfo.userid2));
    prefs.getString("userid3", telegrammerInfo.userid3, sizeof(telegrammerInfo.userid3));
    prefs.getString("userid4", telegrammerInfo.userid4, sizeof(telegrammerInfo.userid4));
    telegrammerInfo.alarmwatts = prefs.getInt("alarmwatts", 2500);
    prefs.end();
    

    if (len_bottoken > 0) RealInit();

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/telegram.html", "text/html", false, TelegramSetupPageProcessor );
    });

    server.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
    // One cannot use yield, delay and appearantly certain tft display actions here...
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == P_BOTTOKEN) {
          const char * token = p->value().c_str();
          strncpy(telegrammerInfo.bottoken, token, sizeof(telegrammerInfo.bottoken));
        }
        // HTTP POST pass value
        if (p->name() == P_USERID1) {
          const char * userid = p->value().c_str();
          strncpy(telegrammerInfo.userid1, userid, sizeof(telegrammerInfo.userid1));
        }
        // HTTP POST pass value
        if (p->name() == P_USERID2) {
          const char * userid = p->value().c_str();
          strncpy(telegrammerInfo.userid2, userid, sizeof(telegrammerInfo.userid2));
        }
        // HTTP POST pass value
        if (p->name() == P_USERID3) {
          const char * userid = p->value().c_str();
          strncpy(telegrammerInfo.userid3, userid, sizeof(telegrammerInfo.userid3));
        }
        // HTTP POST pass value
        if (p->name() == P_USERID4) {
          const char * userid = p->value().c_str();
          strncpy(telegrammerInfo.userid4, userid, sizeof(telegrammerInfo.userid4));
        }
        
        // HTTP POST subnetmask value
        if (p->name() == P_ALARMWATTS) {
          telegrammerInfo.alarmwatts = p->value().toInt();
        }
      }
    }

    WritePreferences(); 
    RealInit();
    request->send(200, "text/plain", "Telegram configurred");
  });
}


void Telegrammer::RealInit() {
    client.setCACert(telegram_cert);
    my_bot.setUpdateTime(5000);
    my_bot.setTelegramToken(telegrammerInfo.bottoken);
}

String Telegrammer::TelegramSetupPageProcessor(const String& var) {
  if (var == P_BOTTOKEN) {
    return String(telegrammerInfo.bottoken);
  }
  if (var == P_USERID1) {
    return String(telegrammerInfo.userid1);
  }
  if (var == P_USERID2) {
    return String(telegrammerInfo.userid2);
  }
  if (var == P_USERID3) {
    return String(telegrammerInfo.userid3);
  }
  if (var == P_USERID4) {
    return String(telegrammerInfo.userid4);
  }
  if (var == P_ALARMWATTS) {
    return String(telegrammerInfo.alarmwatts);
  }
  return String(); // @suppress("Ambiguous problem")
}
