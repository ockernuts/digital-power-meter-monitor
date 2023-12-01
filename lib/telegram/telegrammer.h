#pragma once
/*
 * telegrammer.h
 *
 *  Created on: 15 mar. 2023
 *      Author: Wim
 */

#include <WiFi.h>
#include "idisplayer.h"
#include <ESPAsyncWebServer.h>
//#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>


struct TelegramInfoStruct {
    char bottoken[48] = "";
    char userid1[12] = "";
    char userid2[12] = ""; 
    char userid3[12] = ""; 
    char userid4[12] = ""; 
    int  alarmwatts = 2500; // Average watts per quarter to avoid. 
};

typedef struct TelegramInfoStruct TelegramInfoStruct;

class Telegrammer
{
private:
  static TelegramInfoStruct telegrammerInfo;

  AsyncWebServer& server;
  IDisplayer &displayer;

  WiFiClientSecure client;
  AsyncTelegram2 my_bot;

public:
  Telegrammer(AsyncWebServer& server, IDisplayer& displayer);
  virtual ~Telegrammer();

  void Init();

  static String TelegramSetupPageProcessor(const String& var);

protected:
  void RealInit();
  void WritePreferences(); 
  
};