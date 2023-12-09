/*
 * MyWifiManager.h
 *
 *  Created on: 12 feb. 2023
 *      Author: Wim
 */

#ifndef MYWIFIMANAGER_H_
#define MYWIFIMANAGER_H_

#include "wifistructs.h"
#include "idisplayer.h"
#include <ESPAsyncWebServer.h>

// Using an OWN Wifi Manager to :
// - Have progress though a "displayer" adaptor, to a TFT screen or a serial port or any printer...
// - Have own HTML pages (in LittleFS).
// - Use Async Web Server
// - Use own parameter storage so more can be stored than what the SDK does for Wifi.
//   -> Storing Wifi config on SD Card is more flexibility: 
//      gives easier option to do debugging/reset/forces reconfig
//   Hence we don't use the SDK SmartConfig....

class MyWiFiManager
{
private:
  static WiFiConfigInfo wiFiConfigInfo;
  
  bool restartNeeded; // Set after in AP mode configuration is set.
  bool configPresent; // Set when an Wifi Config was persisted. 
  unsigned long wiFiConfigModeStart;

  AsyncWebServer& server;
  IDisplayer &displayer;
  IWiFiConfigPersistency &configPersistency; 

public:
  MyWiFiManager(AsyncWebServer& server, IDisplayer& displayer, IWiFiConfigPersistency& configPersistency);
  virtual ~MyWiFiManager();

  bool Init();


  // Returns true if Wifi Reconfig is pending
  // Needs to be called from the main loop method to allow actions.
  bool LoopWiFiReconfigPending();

  void PostWebServerStartSSDPInit();

  static const char* GetWiFiStatusAsString(wl_status_t status);

  static String WiFiSetupPageProcessor(const String& var);

protected:
  void SaveWiFiConfig();
  bool GetPreviousConfigAndValidateOrSetDefaults();
  bool AttemptAutoConnect();
  void InitFS();

};

#endif /* MYWIFIMANAGER_H_ */
