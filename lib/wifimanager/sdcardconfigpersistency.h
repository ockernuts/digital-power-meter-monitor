#pragma once
#include "wifimanager.h"
#include <ArduinoJson.h>
#include "sd_card.h"
#include "jsonvalidator.h"


constexpr const char *CONFIG_FILE = "/config.json";
/*
READ+VALIDATE / WRITE a json like:
{
  "wifi": {
    "ssid": "yourSSID",
    "password": "yourPassword"
  },
  "network": {
    "ip" : "192.168.0.250",
    "gateway": "192.168.0.1",
    "subnet": "255.255.255.0"
  }
}

*/

// FIELDS:
constexpr const char *WIFI = "wifi";
constexpr const char *SSID = "ssid";
constexpr const char *PASSWORD = "password";
constexpr const char *NETWORK = "network";
constexpr const char *IP = "ip";
constexpr const char *GATEWAY = "gateway";
constexpr const char *SUBNET = "subnet";


class SDCardConfigPersistency : public IWifiConfigPersistency {
    public: 
        virtual bool Load(WifiConfigInfo& info) {
            File32 f = sd.open(CONFIG_FILE);
            if (!f.isOpen()) {
                Serial.printf("No %s file found\n", CONFIG_FILE);
                return false; 
            }

            Serial.printf("Opened %s file, size %d\n", CONFIG_FILE, f.size());

            StaticJsonDocument<500> doc;
            auto error = deserializeJson(doc, f);
            f.close();
            if (error) {
                Serial.printf("Cannot parse %s file as json: %s\n", CONFIG_FILE, error.c_str());
                return false; 
            }

            /*
            if (!validateConfigJson(doc, CONFIG_FILE)) {
                Serial.printf("Problem validating %s file\n", CONFIG_FILE);
                return false; 
            }
            */

            const JsonObject& wifi = doc[WIFI];
            const char *ssid = wifi[SSID];
            const char *password = wifi[PASSWORD];
            const JsonObject& network = doc[NETWORK];
            const char *ip = network[IP];
            const char *subnet = network[SUBNET];
            const char *gateway = network[GATEWAY];

            strncpy(info.ssid, ssid, sizeof(info.ssid));
            strncpy(info.pwd, password, sizeof(info.pwd));

            info.ApplyIPConfig(ip, gateway, subnet);
            return true;
            
        }

        virtual void Save(const WifiConfigInfo& info) {
            IPAddress ip(info.ip);
            IPAddress gateway(info.gateway);
            IPAddress subnet(info.subnet);
            StaticJsonDocument<500> doc; 

            JsonObject wifi = doc.createNestedObject(WIFI);
            wifi[SSID] = info.ssid;
            wifi[PASSWORD] = info.pwd;
            JsonObject network = doc.createNestedObject(NETWORK);
            network[IP] = ip;
            network[GATEWAY] = gateway;
            network[SUBNET] = subnet;

            File32 f = sd.open(CONFIG_FILE, O_CREAT | O_WRONLY);
            if (!f) {
                Serial.printf("Problem starting to write %s\n", CONFIG_FILE);
                return;
            }
            serializeJson(doc, f);
            f.close();
            return; 
        }
    
    private:
    /*
        bool validateConfigJson(const JsonDocument& doc, const char * config_file) {
            static const JsonFieldValidator wifi_ssid = {SSID, NULL, 0};
            static const JsonFieldValidator wifi_password = {PASSWORD, NULL, 0};
            static const JsonFieldValidator* wifi_fields[] = {&wifi_ssid, &wifi_password};
            static const JsonFieldValidator wifi = {WIFI, wifi_fields, 2};
            static const JsonFieldValidator network_ip = {IP, NULL, 0};
            static const JsonFieldValidator network_subnet = {SUBNET, NULL, 0};
            static const JsonFieldValidator network_gateway = {GATEWAY, NULL, 0};
            static const JsonFieldValidator *network_fields[] = {&network_ip, &network_subnet, &network_gateway};
            static const JsonFieldValidator network = {NETWORK, network_fields, 3};
            static const JsonFieldValidator *fields[] = {&wifi, &network};

            return validateJson(doc, fields, 2, config_file);
        };
    */

};