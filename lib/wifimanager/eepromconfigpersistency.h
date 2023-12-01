#pragma once
#include "wifimanager.h"
#include <EEPROM.h>

#define WIFI_CONFIG_ADDRESS 0
#define EEPROM_CHECK_VALUE  "NUT1WIM"

struct EEPROM_CHECK {
    char eeprom_check[8] = EEPROM_CHECK_VALUE;
};

class SDCardConfigPersistency : public IWifiConfigPersistency {
    public: 
        virtual bool Load(WifiConfigInfo& info) {
            struct EEPROM_CHECK check; 
            EEPROM.begin(sizeof(EEPROM_CHECK) + sizeof(WifiConfigInfo) + WIFI_CONFIG_ADDRESS);
            EEPROM.get(WIFI_CONFIG_ADDRESS, check);
            if (strncmp(check.eeprom_check, EEPROM_CHECK_VALUE, sizeof(check.eeprom_check)) != 0) {
                EEPROM.end();
                return false; 
            }
            EEPROM.get(WIFI_CONFIG_ADDRESS + sizeof(EEPROM_CHECK), info);
            EEPROM.end();
            return true;
        }

        virtual void Save(const WifiConfigInfo& info) {
            struct EEPROM_CHECK check; 
            EEPROM.begin(sizeof(EEPROM_CHECK) + sizeof(WifiConfigInfo) + WIFI_CONFIG_ADDRESS);
            EEPROM.put(WIFI_CONFIG_ADDRESS, check);
            EEPROM.put(WIFI_CONFIG_ADDRESS + sizeof(EEPROM_CHECK), info);
            EEPROM.end();
        }
};