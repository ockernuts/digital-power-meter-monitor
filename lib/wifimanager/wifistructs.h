#pragma once
#include <WiFi.h>
constexpr const char *DEFAULT_IP = "192.168.0.250";
constexpr const char *DEFAULT_GW = "192.168.0.1";
constexpr const char *DEFAULT_SN = "255.255.255.0";
constexpr const char *DEFAULT_DEVICE_NAME = "digimon";

struct WifiConfigInfoStruct {
    char ssid[36] = ""; // max 32 characters + zero + pad to next DWORD level.
    char pwd[64] = ""; // max 63 characters + zero

    unsigned char ip[4];       //Node static IP
    unsigned char gateway[4];
    unsigned char subnet[4];

    bool dhcp;
    char device_name[16];

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
    virtual void Erase();
};