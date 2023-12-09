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

const unsigned long millisTimeoutForWifiReconfigUponConnectFailure = 1000 * 60 * 5; // 5 minutes

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

MyWifiManager::MyWifiManager(AsyncWebServer& server, IDisplayer& displayer, IWifiConfigPersistency& configPersistency) :
    restartNeeded(false), configPresent(false), wifiConfigModeStart(0), server(server), displayer(displayer),
    configPersistency(configPersistency)
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
  if (var == "SELECTED_DHCP") {
    if (wifiConfigInfo.dhcp) {
      return String("Selected");
    } else {
      return String();
    }
  }
  if (var == "SELECTED_NO_DHCP") {
    if (wifiConfigInfo.dhcp) {
      return String();
    } else {
      return String("Selected");
    }
  }
  if (var == "DEVICE_NAME") {
    return String(wifiConfigInfo.device_name);
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
const char* PARAM_DEVICE_NAME = "device_name";
const char* PARAM_IP_CONFIG = "ip_config";

bool MyWifiManager::Init() {
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

  // Setup Wifi manager HTTP page and post handler to reconfigure the Wifi.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Sending wifi config page");
    request->send(LittleFS, "/wifimanager.html", "text/html", false, WifiSetupPageProcessor );
  });

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "");
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println("Sending wifi config page on not found");
    request->send(LittleFS, "/wifimanager.html", "text/html", false, WifiSetupPageProcessor );
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
        if (p->name() == PARAM_IP_CONFIG) {
          wifiConfigInfo.dhcp =  p->value() =="dhcp";
        }
        if (p->name() == PARAM_DEVICE_NAME) {
          strncpy(wifiConfigInfo.device_name, p->value().c_str(), sizeof(wifiConfigInfo.device_name));
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    SaveWifiConfig();
    char message[250];
    snprintf(message, sizeof(message), "<head></head><body>De monitor herstart nu. Verbind je weer met een thuis netwerk en probeer na een tijdje deze link: "
                                       "<A HREF='http://%s.local'>http://%s.local</A> om de monitor te bereiken</body>",
                                       wifiConfigInfo.device_name,wifiConfigInfo.device_name);
    request->send(200, "text/html", message);
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
  strncpy(wifiConfigInfo.device_name, DEFAULT_DEVICE_NAME, sizeof(wifiConfigInfo.device_name));
  wifiConfigInfo.dhcp = true;
  wifiConfigInfo.ApplyIPConfig(); 

  if (!configPersistency.Load(wifiConfigInfo)) {
      displayer.println("No prior WIFI config");
      return false; 
  }
  return true;
}

bool MyWifiManager::AttemptAutoConnect()
{
  // Try to read previous config values from EEPROM or SDCARD
  if (!GetPreviousConfigAndValidateOrSetDefaults()) return false;

  

  WiFi.setHostname(wifiConfigInfo.device_name);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  if (!wifiConfigInfo.dhcp) {
    IPAddress IP(wifiConfigInfo.ip);
    IPAddress GW(wifiConfigInfo.gateway);
    IPAddress SN(wifiConfigInfo.subnet);
    IPAddress DNS;
    DNS.fromString("8.8.8.8");
    displayer.print("Previous IP:");
    displayer.println(IP.toString().c_str());
    
    if (!WiFi.config(IP, GW, SN, DNS)){
      displayer.errorln("Wifi config failure");
      // resets the IP, gateway, subnet to defaults in memory only, for a new setup through HTML
      wifiConfigInfo.ApplyIPConfig();
      return false;
    }      
  }


  displayer.println("Previous SSID:");
  displayer.println(wifiConfigInfo.ssid);
  if (wifiConfigInfo.dhcp) displayer.println("DHCP mode");
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
      // Mark there was a Wifi Config. This can be an important difference for how long the Wifi Config mode 
      // is usable. This is important because a bad Wifi connect below might be on a powerloss in the home. 
      // The telecom equipment might need some time to establish the network again...
      // Hence not being able to connect with a saved wifi config deserves a restart 
      // after a while to try to reconnect...
      displayer.println("Limiting Wifi Reconfig in time to retry Wifi connect with stored config");
      configPresent = true; 
      wifiConfigModeStart = millis();
      delay(2000);
      return false;
  }

  displayer.println("Connected !");
  displayer.println(WiFi.localIP().toString().c_str());
  displayer.println(WiFi.getHostname());

  if (!MDNS.begin(wifiConfigInfo.device_name)) { 
    displayer.println("Error setting up mDNS");
  }
  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "board", BOARD_NAME);
  MDNS.enableWorkstation();


  // SSDP 
  server.on("/description.xml", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/xml", SSDP.schema(false));
  });
  server.on("/electic-meter.png", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(LittleFS, "/electic-meter.png");
  });
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

  if (configPresent && (millis()-wifiConfigModeStart)>millisTimeoutForWifiReconfigUponConnectFailure) {
    displayer.println("Timeout reached on Wifi Reconfig after saved Wifi could not connect");
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
      displayer.println("Erasing Wifi Config");
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


void MyWifiManager::PostWebServerStartSSDPInit() {
  //set schema xml url, nees to match http handler
  //"ssdp/schema.xml" if not set
  SSDP.setSchemaURL("description.xml");
  //set port
  //80 if not set
  SSDP.setHTTPPort(80);
  //set device name
  //Null string if not set
  SSDP.setName(wifiConfigInfo.device_name);
  //set Serial Number
  //Null string if not set
  //SSDP.setSerialNumber("001788102201");
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
  //SSDP.setModelURL("http://www.meethue.com");
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
  SSDP.setServerName(wifiConfigInfo.device_name);
  //set UUID, you can use https://www.uuidgenerator.net/
  //use 38323636-4558-4dda-9188-cda0e6 + 4 last bytes of mac address if not set
  //use SSDP.setUUID("daa26fa3-d2d4-4072-bc7a-a1b88ab4234a", false); for full UUID
  //SSDP.setUUID("daa26fa3-d2d4-4072-bc7a");
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
  displayer.print("Starting SSDP...\n");
  SSDP.begin();
}
