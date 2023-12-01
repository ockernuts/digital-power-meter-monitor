/*
 * MyWifiManager.h
 *
 *  Created on: 12 feb. 2023
 *      Author: Wim
 */

#ifndef MYWIFIMANAGER_H_
#define MYWIFIMANAGER_H_

#include <WiFi.h>
#include "idisplayer.h"
#include <ESPAsyncWebServer.h>


// TODO:
// Use preferences instead of EEPROM on ESP32


// Using an OWN Wifi Manager to :
// - Have progress though a "displayer" adaptor, to a TFT screen or a serial port or any printer...
// - Have own HTML pages (in LittleFS).
// - Use Async Web Server
// - Use own parameter storage so more can be stored that the SDK does for Wifi.
//   Hence we don't use the SDK SmartConfig....


constexpr const char *DEFAULT_IP = "192.168.0.250";
constexpr const char *DEFAULT_GW = "192.168.0.1";
constexpr const char *DEFAULT_SN = "255.255.255.0";

struct WifiConfigInfoStruct {
    char ssid[36] = ""; // max 32 characters + zero + pad to next DWORD level.
    char pwd[64] = ""; // max 63 characters + zero

    unsigned char ip[4];       //Node static IP
    unsigned char gateway[4];
    unsigned char subnet[4];

    void ApplyIPConfig(const char *newIP = DEFAULT_IP, const char *newGateway = DEFAULT_GW, const char *newSubnet = DEFAULT_SN)
    {
      IPAddress IP;
      IP.fromString(newIP);
      IPAddress GW;
      GW.fromString(newGateway);
      IPAddress SN;
      SN.fromString(newSubnet);
      for (int i = 0; i < 4; i++)
      {
        ip[i] = IP[i];
        gateway[i] = GW[i];
        subnet[i] = SN[i];
      }
    }
};



typedef struct WifiConfigInfoStruct WifiConfigInfo;

class IWifiConfigPersistency {
  public: 
    virtual bool Load(WifiConfigInfo &) = 0;
    virtual void Save(const WifiConfigInfo &) = 0; 
};

class MyWifiManager
{
private:
  static WifiConfigInfo wifiConfigInfo;
  

  bool restartNeeded; // Set after in AP mode configuration is set.

  AsyncWebServer& server;
  IDisplayer &displayer;
  IWifiConfigPersistency &configPersistency; 

public:
  MyWifiManager(AsyncWebServer& server, IDisplayer& displayer, IWifiConfigPersistency& configPersistency);
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
