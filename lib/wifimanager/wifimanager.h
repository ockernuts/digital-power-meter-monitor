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

class MyWifiManager
{
private:
  static WifiConfigInfo wifiConfigInfo;
  
  bool restartNeeded; // Set after in AP mode configuration is set.
  bool configPresent; // Set when an Wifi Config was persisted. 
  unsigned long wifiConfigModeStart;

  AsyncWebServer& server;
  IDisplayer &displayer;
  IWifiConfigPersistency &configPersistency; 

  IWifiConfigPersistency *formerConfigPersistency; 

public:
  MyWifiManager(AsyncWebServer& server, IDisplayer& displayer, IWifiConfigPersistency& configPersistency,
                IWifiConfigPersistency *formerConfigPersistency = NULL);
  virtual ~MyWifiManager();

  bool Init();


  // Returns true if Wifi Reconfig is pending
  // Needs to be called from the main loop method to allow actions.
  bool LoopWifiReconfigPending();

  static const char* GetWifiStatusAsString(wl_status_t status);

  static String WifiSetupPageProcessor(const String& var);

protected:
  void SaveWifiConfig();
  bool GetPreviousConfigAndValidateOrSetDefaults();
  bool AttemptAutoConnect();
  void InitFS();

};

#endif /* MYWIFIMANAGER_H_ */
