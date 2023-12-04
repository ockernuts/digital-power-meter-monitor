/*
 * MyWifiManager.cpp
 *
 *  Created on: 12 feb. 2023
 *      Author: Wim
 */

#include "wifimanager.h"
#include <LittleFS.h>
#include <DNSServer.h>
#include "idisplayer.h"
#include "hardware_and_pins.h"


// dnsServer is made when WifiReconfig needs to happen and the ESP becomes Access Point.
// This also indicates reconfig is pending.
// Reconfig will end with a nice reset.
std::unique_ptr<DNSServer> dnsServer;
const byte    DNS_PORT                = 53;

void setupDNSD(){
  dnsServer.reset(new DNSServer());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer->start(DNS_PORT, F("*"), WiFi.softAPIP());
}



// Initialization if static wifiConfigInfo member. (otherwise linker error).
WifiConfigInfo MyWifiManager::wifiConfigInfo;

MyWifiManager::MyWifiManager(AsyncWebServer& server, IDisplayer& displayer, IWifiConfigPersistency& configPersistency, 
                             IWifiConfigPersistency *formerConfigPersistency) :
    restartNeeded(false), server(server), displayer(displayer), configPersistency(configPersistency), 
    formerConfigPersistency(formerConfigPersistency)
{
}

MyWifiManager::~MyWifiManager()
{
}

const char* MyWifiManager::GetWifiStatusAsString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    // case WL_WRONG_PASSWORD: return "WL_WRONG_PASSWORD";  // NOT PRESENT IN ESP32 SDK (came from ESP8266)
  }
  return "UNKNOWN";
}

String MyWifiManager::WifiSetupPageProcessor(const String& var) {
  if (var == "SSID") {
    return String(wifiConfigInfo.ssid);
  }
  if (var == "PWD") {
    return String(wifiConfigInfo.pwd);
  }
  if (var == "IP") {
    IPAddress IP(wifiConfigInfo.ip);
    return IP.toString();
  }
  if (var == "GW") {
    IPAddress GW(wifiConfigInfo.gateway);
    return GW.toString();
  }
  if (var == "SUBNET") {
    IPAddress SN(wifiConfigInfo.subnet);
    return SN.toString();
  }
  return String(); // @suppress("Ambiguous problem")
}

// TODO move this out of the wifi manager (common)
void MyWifiManager::InitFS() {
  // Little FS is a file system in the flash, to store files that do not really change.
  // It is used as a data partition with html and other files.  
  if (!LittleFS.begin()) {
    displayer.errorln("LittleFS Init Error");
    delay(5000);
    ESP.restart();
  }
  else{
    displayer.println("LittleFS Init OK");
  }
}


// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
const char* PARAM_INPUT_5 = "subnet";

bool MyWifiManager::Init() {
  InitFS();

  if (AttemptAutoConnect()) return true;

  // Wifi needs to be reconfigurred. Setup an access poin to fetch the configuration.
  // Connect to Wi-Fi network with SSID and password
  displayer.newPage("     Setup Mode");
  WiFi.disconnect(true);
  
  // NULL sets an open Access Point
  WiFi.softAP("DigitaleMeterMonitor", NULL);
  setupDNSD();

  String SSID = WiFi.softAPSSID();
  IPAddress IP = WiFi.softAPIP();
 
  displayer.println("SSID:");
  displayer.println(SSID.c_str());
  displayer.println("Connect IP:");
  displayer.println(IP.toString().c_str());

  // Setup Wifi manager HTTP page and post handler to reconfigure the Wifi.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/wifimanager.html", "text/html", false, WifiSetupPageProcessor );
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
    // One cannot use yield, delay and appearantly certain tft display actions here...
    IPAddress IP;

    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          const char * lssid = p->value().c_str();
          strncpy(wifiConfigInfo.ssid, lssid, sizeof(wifiConfigInfo.ssid));
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          const char * lpass = p->value().c_str();
          strncpy(wifiConfigInfo.pwd, lpass, sizeof(wifiConfigInfo.pwd));
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          IP.fromString(p->value());
          for (int i=0; i<4; i++ ) { wifiConfigInfo.ip[i] = IP[i]; }
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          IPAddress GW;
          GW.fromString(p->value());
          for (int i=0; i<4; i++ ) { wifiConfigInfo.gateway[i] = GW[i]; }
        }
        // HTTP POST subnetmask value
        if (p->name() == PARAM_INPUT_5) {
          IPAddress SN;
          SN.fromString(p->value());
          for (int i=0; i<4; i++) { wifiConfigInfo.subnet[i] = SN[i]; }
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    SaveWifiConfig();
    request->send(200, "text/plain", "The device will restart. Decouple from this wifi network and connect to you normal network.");
    restartNeeded = true;
  });
  return false;
}


void MyWifiManager::SaveWifiConfig() {
  configPersistency.Save(wifiConfigInfo);
}

bool MyWifiManager::GetPreviousConfigAndValidateOrSetDefaults() {
  // First set defaults
  strncpy(wifiConfigInfo.ssid, "", sizeof(wifiConfigInfo.ssid));
  strncpy(wifiConfigInfo.pwd, "", sizeof(wifiConfigInfo.pwd));
  wifiConfigInfo.ApplyIPConfig(); 

  if (!configPersistency.Load(wifiConfigInfo)) {
    if (!formerConfigPersistency || !formerConfigPersistency->Load(wifiConfigInfo)) {
      displayer.println("No prior WIFI config");
      return false; 
    }
    // Upgrade case from formerConfigPersistency for beta users. 
    configPersistency.Save(wifiConfigInfo);
  }
  return true;
}

bool MyWifiManager::AttemptAutoConnect()
{
  // Try to read previous config values from EEPROM or SDCARD
  if (!GetPreviousConfigAndValidateOrSetDefaults()) return false;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  IPAddress IP(wifiConfigInfo.ip);
  IPAddress GW(wifiConfigInfo.gateway);
  IPAddress SN(wifiConfigInfo.subnet);
  IPAddress DNS;
  DNS.fromString("8.8.8.8");
  if (!WiFi.config(IP, GW, SN, DNS)){
    displayer.errorln("Wifi config failure");
    // resets the IP, gateway, subnet to defaults in memory only, for a new setup through HTML
    wifiConfigInfo.ApplyIPConfig();
    return false;
  }

  displayer.println("Previous SSID:");
  displayer.println(wifiConfigInfo.ssid);
  displayer.println("");
  wl_status_t status = WiFi.begin(wifiConfigInfo.ssid, wifiConfigInfo.pwd);

  displayer.println("Connecting ...");

  int i = 0;
  while (i<20) {
    delay(1000);
    status = WiFi.status();
    displayer.println(GetWifiStatusAsString(status));
    if (status == WL_CONNECTED) break;
    i++;
  }
  if( status != WL_CONNECTED) {
      displayer.errorln("Failed to connect.");
      delay(2000);
      return false;
  }

  displayer.println("Connected !");
  displayer.println(WiFi.localIP().toString().c_str());
  return true;
}

bool MyWifiManager::LoopWifiReconfigPending() {
  if (restartNeeded) {
    displayer.newPage("Wifi Resetup");
    displayer.print("SSID:");
    displayer.newPage("Wifi Resetup");
    displayer.println(wifiConfigInfo.ssid);
    displayer.println("");
    displayer.println("Restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  if (dnsServer) {
    dnsServer->processNextRequest();
    return true;
  }

  return false;
}
