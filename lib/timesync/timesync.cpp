#include "timesync.h"

bool first_time_update = true;


uint32_t sntp_update_delay_MS_rfc_not_less_than_15000()
{
    // //info_sntp_update_delay_MS_rfc_not_less_than_15000_has_been_called = true;
  if (first_time_update) {
    return 15000; // min = 15000 = 15s
  } else {
    return 3600000; // 1 hour
  }
}


// Some NTP pre-config. 
#define MY_NTP_SERVER "be.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"  

void setupTimeSync(IDisplayer &displayer) {
    // Wait for time sync ...
  // Activate sync to NPT.
  delay(200); // Give Wifi More time to get up ....
  displayer.newPage("Syncronizeer Tijd");
  uint32_t startTime = millis();

  // Central european time defined here !!!
  configTzTime(MY_TZ, MY_NTP_SERVER); 

  while (time(nullptr) < 100000ul) {
    displayer.print(".");
    delay(100);
  }
  first_time_update = true;
  uint32_t runtimeMillis = millis() - startTime;
  displayer.print("Synced in ");
  displayer.print(String(runtimeMillis).c_str());
  displayer.println(" ms.");
  delay(1000);
}