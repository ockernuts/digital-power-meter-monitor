#include "http_handlers.h"
#include "LittleFS.h"
#include "json_quarter_querier.h"
#include "month_peak_querier.h"
#include "sd_card.h"

void InitHttpHandlers(AsyncWebServer &server) {
    //if you get here you have connected to the WiFi and you know the time !
  // Start WEBSERVER + Handler ?
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/index.html");
   });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/favicon.ico");
   });
  
  server.on("/dark-unica.js", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/dark-unica.js");
   });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/style.css");
   });
  server.on("/watt", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", String(GetFixedQuarterPowerHistoryAccumulator().GetCurrentWattsHistorySample().GetConsumedPowerAverage()).c_str());
    });
  // This is only possible because we wrapped the SdFat in an FS adapter...(and File adapter)
  server.serveStatic("/meter", SdFat32Fs, "/meter");
  server.serveStatic("/crashlog.txt", SdFat32Fs, "/crashlog.txt");
  server.onNotFound([](AsyncWebServerRequest *request){
      request->send(404, "text/plain", "Item not found");
    });

  server.on("/historic/years", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<768> doc;
      JsonObject years_holder = doc.to<JsonObject>();
      getYearsWithData(years_holder);
      serializeJson(years_holder, *response);
      request->send(response);
    });
  
  server.on("/historic/months", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebParameter* year_param = request->getParam("year");
      if (!year_param) {
        return request->send(400, "text/plain", F("ERROR: Need a year argument"));
      } 

      int year = year_param->value().toInt();
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<768> doc;
      JsonObject year_months_holder = doc.to<JsonObject>();
      getYearMonthsWithData(year_months_holder, year);
      serializeJson(year_months_holder, *response);
      request->send(response);
    });
  
  server.on("/historic/days", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebParameter* year_param = request->getParam("year");
      if (!year_param) {
        return request->send(400, "text/plain", F("ERROR: Need a year argument"));
      } 
      int year = year_param->value().toInt();

      AsyncWebParameter* month_param = request->getParam("month");
      if (!month_param) {
        return request->send(400, "text/plain", F("ERROR: Need a month argument"));
      } 
      int month = month_param->value().toInt();

      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<768> doc;
      JsonObject year_month_days_holder = doc.to<JsonObject>();
      getYearMonthDaysWithData(year_month_days_holder, year, month);
      serializeJson(year_month_days_holder, *response);
      request->send(response);
    });

  server.on("/historic/quarters", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncWebParameter* year_param = request->getParam("year");
      if (!year_param) {
        return request->send(400, "text/plain", F("ERROR: Need a year argument"));
      } 
      int year = year_param->value().toInt();

      AsyncWebParameter* month_param = request->getParam("month");
      if (!month_param) {
        return request->send(400, "text/plain", F("ERROR: Need a month argument"));
      } 
      int month = month_param->value().toInt();

      AsyncWebParameter* day_param = request->getParam("day");
      if (!day_param) {
        return request->send(400, "text/plain", F("ERROR: Need a day argument"));
      } 
      int day = day_param->value().toInt();

      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<2048> doc;
      JsonObject year_month_day_quarters_holder = doc.to<JsonObject>();
      getYearMonthDayQuartersWithData(year_month_day_quarters_holder, year, month, day);
      serializeJson(year_month_day_quarters_holder, *response);
      request->send(response);
    });
  

  GetJsonQuarterQuerier(server).Init();
  GetMonthPeakQuerier(server).Init();
}