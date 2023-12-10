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
#include <ESPmDNS.h>
#include "ESP32SSDP.h"

const unsigned long millisTimeoutForWiFiReconfigUponConnectFailure = 1000 * 60 * 5; // 5 minutes

// dnsServer is made when WiFiReconfig needs to happen and the ESP becomes Access Point.
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
WiFiConfigInfo MyWiFiManager::wiFiConfigInfo;

MyWiFiManager::MyWiFiManager(AsyncWebServer& server, IDisplayer& displayer, IWiFiConfigPersistency& configPersistency) :
    restartNeeded(false), configPresent(false), wiFiConfigModeStart(0), server(server), displayer(displayer),
    configPersistency(configPersistency)
{
}

MyWiFiManager::~MyWiFiManager()
{
}

const char *MyWiFiManager::GetSSID() {
  return wiFiConfigInfo.ssid;
}

const char *MyWiFiManager::GetPassword() {
  return wiFiConfigInfo.pwd;
}

const char* MyWiFiManager::GetWiFiStatusAsString(wl_status_t status) {
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

String MyWiFiManager::WiFiSetupPageProcessor(const String& var) {
  if (var == "SSID") {
    return String(wiFiConfigInfo.ssid);
  }
  if (var == "PWD") {
    return String(wiFiConfigInfo.pwd);
  }
  if (var == "IP") {
    IPAddress IP(wiFiConfigInfo.ip);
    return IP.toString();
  }
  if (var == "GW") {
    IPAddress GW(wiFiConfigInfo.gateway);
    return GW.toString();
  }
  if (var == "SUBNET") {
    IPAddress SN(wiFiConfigInfo.subnet);
    return SN.toString();
  }
  if (var == "SELECTED_DHCP") {
    if (wiFiConfigInfo.dhcp) {
      return String("Selected");
    } else {
      return String();
    }
  }
  if (var == "SELECTED_NO_DHCP") {
    if (wiFiConfigInfo.dhcp) {
      return String();
    } else {
      return String("Selected");
    }
  }
  if (var == "DEVICE_NAME") {
    return String(wiFiConfigInfo.device_name);
  }
  
  return String(); // @suppress("Ambiguous problem")
}

// TODO move this out of the wifi manager (common)
void MyWiFiManager::InitFS() {
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
const char* PARAM_DEVICE_NAME = "device_name";
const char* PARAM_IP_CONFIG = "ip_config";

bool MyWiFiManager::Init() {
  InitFS();

#ifdef ENABLE_BOOT_BUTTON_WIFI_RESET
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
#endif

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

  // Setup WiFi manager HTTP page and post handler to reconfigure the WiFi.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/wifimanager.html", "text/html", false, WiFiSetupPageProcessor );
  });

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "");
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(LittleFS, "/wifimanager.html", "text/html", false, WiFiSetupPageProcessor );
  });

  server.serveStatic("/style.css", LittleFS, "/style.css");

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
          strncpy(wiFiConfigInfo.ssid, lssid, sizeof(wiFiConfigInfo.ssid));
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          const char * lpass = p->value().c_str();
          strncpy(wiFiConfigInfo.pwd, lpass, sizeof(wiFiConfigInfo.pwd));
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          IP.fromString(p->value());
          for (int i=0; i<4; i++ ) { wiFiConfigInfo.ip[i] = IP[i]; }
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          IPAddress GW;
          GW.fromString(p->value());
          for (int i=0; i<4; i++ ) { wiFiConfigInfo.gateway[i] = GW[i]; }
        }
        // HTTP POST subnetmask value
        if (p->name() == PARAM_INPUT_5) {
          IPAddress SN;
          SN.fromString(p->value());
          for (int i=0; i<4; i++) { wiFiConfigInfo.subnet[i] = SN[i]; }
        }
        if (p->name() == PARAM_IP_CONFIG) {
          wiFiConfigInfo.dhcp =  p->value() =="dhcp";
        }
        if (p->name() == PARAM_DEVICE_NAME) {
          strncpy(wiFiConfigInfo.device_name, p->value().c_str(), sizeof(wiFiConfigInfo.device_name));
        }
      }
    }
    SaveWiFiConfig();
    char message[250];
    snprintf(message, sizeof(message), "<head></head><body>De monitor herstart nu. Verbind je weer met een thuis netwerk en probeer na een tijdje deze link: "
                                       "<A HREF='http://%s.local'>http://%s.local</A> om de monitor te bereiken</body>",
                                       wiFiConfigInfo.device_name,wiFiConfigInfo.device_name);
    request->send(200, "text/html", message);
    restartNeeded = true;
  });
  return false;
}


void MyWiFiManager::SaveWiFiConfig() {
  configPersistency.Save(wiFiConfigInfo);
}

bool MyWiFiManager::GetPreviousConfigAndValidateOrSetDefaults() {
  // First set defaults
  strncpy(wiFiConfigInfo.ssid, "", sizeof(wiFiConfigInfo.ssid));
  strncpy(wiFiConfigInfo.pwd, "", sizeof(wiFiConfigInfo.pwd));
  strncpy(wiFiConfigInfo.device_name, DEFAULT_DEVICE_NAME, sizeof(wiFiConfigInfo.device_name));
  wiFiConfigInfo.dhcp = true;
  wiFiConfigInfo.ApplyIPConfig(); 

  if (!configPersistency.Load(wiFiConfigInfo)) {
      displayer.println("No prior WIFI config");
      return false; 
  }
  return true;
}

bool MyWiFiManager::AttemptAutoConnect()
{
  // Try to read previous config values from EEPROM or SDCARD
  if (!GetPreviousConfigAndValidateOrSetDefaults()) return false;

  

  WiFi.setHostname(wiFiConfigInfo.device_name);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  if (!wiFiConfigInfo.dhcp) {
    IPAddress IP(wiFiConfigInfo.ip);
    IPAddress GW(wiFiConfigInfo.gateway);
    IPAddress SN(wiFiConfigInfo.subnet);
    IPAddress DNS;
    DNS.fromString("8.8.8.8");
    displayer.print("Previous IP:");
    displayer.println(IP.toString().c_str());
    
    if (!WiFi.config(IP, GW, SN, DNS)){
      displayer.errorln("Wifi config failure");
      // resets the IP, gateway, subnet to defaults in memory only, for a new setup through HTML
      wiFiConfigInfo.ApplyIPConfig();
      return false;
    }      
  }


  displayer.println("Previous SSID:");
  displayer.println(wiFiConfigInfo.ssid);
  if (wiFiConfigInfo.dhcp) displayer.println("DHCP mode");
  displayer.println("");
  wl_status_t status = WiFi.begin(wiFiConfigInfo.ssid, wiFiConfigInfo.pwd);

  displayer.println("Connecting ...");

  int i = 0;
  while (i<20) {
    delay(1000);
    status = WiFi.status();
    displayer.println(GetWiFiStatusAsString(status));
    if (status == WL_CONNECTED) break;
    i++;
  }
  if( status != WL_CONNECTED) {
      displayer.errorln("Failed to connect.");
      // Mark there was a Wifi Config. This can be an important difference for how long the Wifi Config mode 
      // is usable. This is important because a bad Wifi connect below might be on a powerloss in the home. 
      // The telecom equipment might need some time to establish the network again...
      // Hence not being able to connect with a saved wifi config deserves a restart 
      // after a while to try to reconnect...
      displayer.println("Limiting Wifi Reconfig in time to retry Wifi connect with stored config");
      configPresent = true; 
      wiFiConfigModeStart = millis();
      delay(2000);
      return false;
  }

  displayer.println("Connected !");
  displayer.println(WiFi.localIP().toString().c_str());
  displayer.println(WiFi.getHostname());

  if (!MDNS.begin(wiFiConfigInfo.device_name)) { 
    displayer.println("Error setting up mDNS");
  }
  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "board", BOARD_NAME);
  MDNS.enableWorkstation();


  // SSDP 
  server.on("/description.xml", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/xml", SSDP.getSchema());
  });
  server.on("/electic-meter.png", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/electic-meter.png");
  });
  return true;
}

bool MyWiFiManager::LoopWiFiReconfigPending() {
  if (restartNeeded) {
    displayer.newPage("WiFi Resetup");
    displayer.print("SSID:");
    displayer.newPage("WiFi Resetup");
    displayer.println(wiFiConfigInfo.ssid);
    displayer.println("");
    displayer.println("Restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  if (configPresent && (millis()-wiFiConfigModeStart)>millisTimeoutForWiFiReconfigUponConnectFailure) {
    displayer.println("Timeout reached on WiFi Reconfig after saved WiFi could not connect");
    displayer.println("Restarting in 5 seconds");
    delay(5000);
    ESP.restart(); 
  }

  if (dnsServer) {
    dnsServer->processNextRequest();
    return true;
  }

#ifdef ENABLE_BOOT_BUTTON_WIFI_RESET
  static unsigned long start_boot_button_down_ms = 0; 
  if (!digitalRead(BOOT_BUTTON)) {
    if (start_boot_button_down_ms == 0) {
      start_boot_button_down_ms = millis();
      displayer.println("Detected factory reset button pressed, hold for 20 seconds to engage");
      return false; 
    }
    if ( (millis()-start_boot_button_down_ms) > 20000) {
      displayer.println("Erasing WiFi Config");
      configPersistency.Erase();
      morseOut(LED, "bye");
      ESP.restart();
    }
  } else {
    if (start_boot_button_down_ms) {
      displayer.println("Release factory reset button");
    }
    start_boot_button_down_ms = 0; 
  }  
#endif

  return false;
}


void MyWiFiManager::PostWebServerStartSSDPInit() {
  //set schema xml url, nees to match http handler
  //"ssdp/schema.xml" if not set
  SSDP.setSchemaURL("description.xml");
  //set port
  //80 if not set
  SSDP.setHTTPPort(80);
  //set device name
  //Null string if not set
  SSDP.setName(wiFiConfigInfo.device_name);
  //set Serial Number
  //Null string if not set
  SSDP.setSerialNumber(getSerialNo());
  //set device url
  //Null string if not set
  SSDP.setURL("index.html");
  //set model name
  //Null string if not set
  SSDP.setModelName("Digitale Meter Monitor");
  //set model description
  //Null string if not set
  SSDP.setModelDescription("This device can be controled by WiFi");
  //set model number
  //Null string if not set
  SSDP.setModelNumber(BOARD_NAME);
  //set model url
  //Null string if not set
  SSDP.setModelURL("https://github.com/ockernuts/digital-power-meter-monitor");
  //set model manufacturer name
  //Null string if not set
  SSDP.setManufacturer("Ockernuts");
  //set model manufacturer url
  //Null string if not set
  SSDP.setManufacturerURL("https://github.com/ockernuts/digital-power-meter-monitor");
  //set device type
  //"urn:schemas-upnp-org:device:Basic:1" if not set
  SSDP.setDeviceType("rootdevice"); //to appear as root device, other examples: MediaRenderer, MediaServer ...
  //set server name
  //"Arduino/1.0" if not set
  SSDP.setServerName(wiFiConfigInfo.device_name);
  //set UUID, you can use https://www.uuidgenerator.net/
  //use 38323636-4558-4dda-9188-cda0e6 + 4 last bytes of mac address if not set
  //use SSDP.setUUID("daa26fa3-d2d4-4072-bc7a-a1b88ab4234a", false); for full UUID
  SSDP.setUUID("38323636-4558-4dda-9188-cda0e6");
  //Set icons list, NB: optional, this is ignored under windows
  SSDP.setIcons(  "<icon>"
                  "<mimetype>image/png</mimetype>"
                  "<height>32</height>"
                  "<width>32</width>"
                  "<depth>24</depth>"
                  "<url>electric-meter.png</url>"
                  "</icon>");
  //Set service list, NB: optional for simple device
  /*
  SSDP.setServices(  "<service>"
                      "<serviceType>urn:schemas-upnp-org:service:SwitchPower:1</serviceType>"
                      "<serviceId>urn:upnp-org:serviceId:SwitchPower:1</serviceId>"
                      "<SCPDURL>/SwitchPower1.xml</SCPDURL>"
                      "<controlURL>/SwitchPower/Control</controlURL>"
                      "<eventSubURL>/SwitchPower/Event</eventSubURL>"
                      "</service>");
  */
  displayer.print("Starting Simple Service Discover Protocol ...\n");
  SSDP.begin();
}
