[![nl](https://img.shields.io/badge/lang-nl-yellow.svg)](README.md)

# Digital power meter monitor
The digital power meter monitor is for power meters installed in Belgium, Flanders. It can measure and show the current or past power measurements in real-time on your phone or PC/laptop.
Screenshots are in Dutch: 

![](doc/app/current_measurement_viewing.png)

The digital power meter has a "P1 port" through which the measurements can be received. 
> Using the "S1 port" will likely destroy the monitor. 

![](doc/ports_digital_meter.png)

The P1 port is disabled by default and can be opened through an action on the website of your network operator (like Fluvius) typically. However, it can take days for it to get activated. 

The monitoring is done using an ESP32 board for which the software is here. 
In the board sections below the needed hardware connections to an RJ12 cable that one can plug into the power meter P1 port are described per supported ESP32 board. You need an RJ12 cable with 6 wires, called: 6P6C, not 6P4C
> **WARNING**: Never connect the board with the digital meter and with a USB cable.

Detailed measurements per quarter are stored on a micro SD card in JSON format. Hence we focus on boards that have the SD card reader integrated, but leaving other options open. Use an 8GB to 32GB micro SD card (only tested up to 16GB). It is best to reformat the SD card first in FAT32 and choose a small cluster size (8K for example) because a lot of small files are stored. 

The board is connected to WiFi and exposes a web interface, meant for in-home access only (HTTP), on which one can see the current measurements almost in real-time (updated every 5 seconds by default) or navigate to historical measurements.

The measurements are shown per fixed quarter in the hour because the average watt consumption per quarter can be penalized by higher electricity costs when they exceed 2500 Watts. One incurs an additional cost of approximately 4 euros (as of the 2023 reference) for each additional 1000 Watts on the highest quarter-average consumption per month. This pertains to the capacity tariff, known as 'capaciteits tarief' in Dutch."

One can easily jump back to the details of the quarter having the highest the month peak by pressing the "month peak" button if the monitor measured that quarter. 

![](doc/app/month_peak_viewing.png)

The source code can deal with multiple boards in theory. In practice, the LilyGo TTGO T8 ESP32-C3 board is mostly used and thus certainly working. It is also the most power-efficient and the cheapest.


# Using the source code
Make sure you have "Visual Studio Code" with "Platform IO" installed as an ESP coding environment. 
Platform IO has an alien-like icon on the left side of VStudio Code to activate it. 
Clicking on it, you should be able to see the "PIO Home" tab. 

Clicking on the project should show a tab per target hardware device. 

For example: "TTGO_T8_ESP32_C3"  

See specific board sections below for their hardware-specific setup.

To get a working board, you need to build and upload the filesystem image and the C/C++ code. (Do this with Platform IO Project Tasks). There might be board-specific actions needed to get it in a state where the items can be uploaded over the USB cable. 

Make sure there is a micro SD card plugged into the board. 


# Using the board with the installed code
## First startup or startup with WiFi connection problems
Remove a possible USB cable from the board. 
Click the RJ12 connector in the P1 port of the digital meter. 
You should see the LED tuning on and off in a faded way. 
If you don't see the LED light up, check whether the P1 port of the digital meter is turned on/open on the website of your electrical network operator (Fluvius).  It can take a couple of days after turning it on before the P1 port really works. 

> **WARNING**: Never connect the board with the digital meter AND with a USB cable. This because the board is getting tension from the digital meter. 
> **ALTERNATIVE**: One can also disconnect the board from the digital meter and connect it to a USB cable and even connect it to a laptop/PC. The board is then powered this way and this also allows WiFi config.

Upon first startup, the board will try to locate WiFi connection parameters from the config.json file on the micro SD card. 
If this file doesn't exist or the board cannot connect to the WiFi network, it will start in configuration mode and act as an access point for a WiFi network "DigitaleMeterMonitor". This uses an IP Subnet 192.168.4.0/24 on which the board has the IP 192.168.4.1
We now also support mDNS which makes the monitor reachable via the name digimon.local
As such one can connect to that WiFi network and browse to http://digimon.local or http://192.168.4.1 to configure the WiFi settings.
The latter might even give a popup in Windows to easily click-open the WiFi config page. 

Sometimes it is difficult to stay on the board's network since it has no path to the internet. In the configuration, the board will require a :
- WiFi network / SSID to connect to
> The WiFi network name of the monitor becomes later the username to use to login to the monitor

> Most probably there is a IP network modem with integrated WiFi capabilities in the same technical area of the digital meter. For provider Telenet you can look on "My Telenet" how the wifi is configured, turn is on, determine SSID and the password and so forth. The WiFi capabilities typically cover a 2.4 GHz and a 5.0 GHz. Best use this network, unless there is a closer network.

> Always use a WiFi network that stays on for the monitor ! 

- WiFi network password
> This WiFi password will later also be used as the password to login to the monitor
Device name (mDNS name / host name/ name on the network)
- Normally the IP configuration is via DHCP. 
- One can switch this to manual and then once needs to configure:
  - IPAddress: The board needs a fixed IPAddress for in-home use (No DHCP). Choose a free address in your home network, typically ending in a high value. Our default is 192.168.0.250. If you have Telenet as an ISP Provided you can have a look at the currently used IP addresses in your home on their website ("Mijn Telenet"). Typically addresses from .0 up to .249 are handed out by DHCP, which is why our default is 250.  
  - Subnet: This is typically 192.168.0.0/24. This depends on your in-house setup / Telecom provider
  - Gateway IP: This is typically 192.168.0.1 or the .1 address in another chosen subnet. 


Once this config is submitted it will be saved to the SD card in the config.json file. 

The board will then reset and attempt to connect to the WiFi. 

Upon startup, the onboard LED will fade in and out. 
Then the LED will blink "hi" in morse code: '.... ..'.  
Eventually the LED will fade in and if connected to the meter fade out as well and repeat this a couple of times per second. 

This means it started up ok and is awaiting data from the digital power meter and if it pulsates even receives data. 
In case WiFi is not connectable, the LED will blink "WiFi" in morse code: '.-- .. ..-. ..' when starting the WiFi configuration mode and then turn off until WiFi configuration was done. 

If there was a problem with the SD card during the startup, the LED will blink "sos" in morse code '... --- ...' with a higher intensity.

If the board is connected then to the P1 port of the digital meter, the LED should pulsate. This indicates measurements are being received. 

## Viewing the measurements
If the board is running you should be able to contact it on its IP address over HTTP. By default browse to http://digimon.local .
> Log in with:
  - Username -> Wifi netwerkname/SSID you configured for the monitor
  - Password -> Wifi Password you configured for the monitor 
You should then see the measurements. 

> To see measurements your phone, laptop or PC must be connected to the home network. This can be with WiFi or with a cable. 

## Alternative Wifi config
An alternative way to configure the board to proper Wifi is to power it off, unmount the SD card from it and put it in a laptop/PC from where you can edit the config.json. 

The config.json looks like:
````
{
  "wifi": {
    "ssid": "yourSSID",
    "password": "yourPassword"
  },
  "network": {
    "dhcp": false,
    "device_name": "digimon", 
    "ip" : "192.168.0.250",
    "gateway": "192.168.0.1",
    "subnet": "255.255.255.0"
  }
}
````

Change what you want, save, properly unmount the card from the laptop/PC and remount it on the board. 
Connect the board again to the digital meter of with an USB cable. 

## Erase the WiFi config using the BOOT button
If the monitor has a WiFi configuration and you want to erase it, you need to press 20 seconds on the BOOT button on the board. If you press long enough, the LED starts blinking again. It blinks "bye" in morse code: '-... -.-- .'. If you see this, directly depress the BOOT button. 
The monitor will erase the WiFi config and restart after a while. 
The monitor then doesn't see a WiFi config and will come up in WiFi configuration mode. 

> The BOOT button only works if the WiFi configuration mode is not already active. 

> If one presses the BOOT button too long, then the board will startup in program mode. Interruptthe P1 or USB cable shortly to let the board restart in normal mode.

## Debug the startup with the serial monitor
Upon trouble with startup, one can also use the Platform IO serial monitor to see what the board is doing upon startup. 

If monitored serially it emits messages like:

  ````
  Digital meter monitor using board: LilyGo TTGO T8 ESP32 C3 V1.1
  LED pin: 3
  Ports used for SD Card communication:
  MOSI: 7
  MISO: 5
  SCK: 4
  SS: 6
  SD Card sectorsPerCluster  64      
  SD Card OK
  Creating test dir /meter/2022/01/01
  Creating test file /meter/2022/01/01/hello.txt
  Serial pins used for communication to digital meter:
  RX1: 2, TX1: 0, RTS 8
  LittleFS Init OK
  No prior WiFi config
      Setup Mode
  --------------------------------
  SSID:
  DigitaleMeterMonitor
  Connect IP:
  192.168.4.1
  ````
The pin values will vary per board type. 

In this example, the device starts up as an access point with an SSID "DigitaleMeterMonitor". 

When the WiFi is connecting fine you will see on the debug terminal:
  ````
  Digital meter monitor using board: LilyGo TTGO T8 ESP32 C3 V1.1
  LED pin: 3
  Ports used for SD Card communication:
  MOSI: 7
  MISO: 5
  SCK: 4
  SS: 6
  Init SD SLOT SPI...
  SD SLOT SPI Pins set
  SD Card sectorsPerCluster  64      
  SD Card OK
  Creating test dir /meter/2022/01/01
  Creating test file /meter/2022/01/01/hello.txt
  Serial pins used for communication to digital meter:
  RX1: 2, TX1: 0, RTS 8
  LittleFS Init OK
  Opened /config.json file, size 138
  Previous SSID:
  ghost

  Connecting ...
  WL_CONNECTED
  Connected !
  192.168.0.248
  Syncronizeer Tijd
  --------------------------------
  ..............................Synced in 3000 ms.
  2023-12-02 13:37:27
  Verbruiksmonitor
  --------------------------------
````

After the board has software and its LittleFS data and can connect to WiFi, it can be upgraded by browsing to:
````
http://digimon.local/update
````

This gives you a user interface where build output files can be uploaded for code or data(file system), as needed. 

As a progammer you can make your own builds of the code or the LittleFS filesystem and upload them to the device. 

# Web application development
The data folder contains the files for the web application. 
These files are put in the flash filesystem on the ESP board. This file system is also known as little FS. 
Your main interest will be the index.html file. 
This gets current and historic data using HTTP GET calls like: 
- GET http://&lt;ip&gt;/current/quarter 
- GET http://&lt;ip&gt;>/meter/2023/12/01/1615W.json
- GET http://&lt;ip&gt;>/current/month/peak

The index.html page detects when it is served from localhost. In this case, it does the above GET calls to 192.168.0.250 so a functioning board plugged in your home digital power meter can serve the requests. This facilitates debugging because you can change the index.html file faster on the laptop/PC, save it and reload it in your browser to see the change effect. 
It does mean that the index.html file needs to be served from the laptop/PC. 
To do this, there is a python httpserver.py file in the data folder which can be run. 
To start servicing any file in the data directory like index.html on the local port 8000:
````
  cd data
  python httpserver.py
````
You need a version of Python installed. Tested with Python 3.11.1 on a Windows PC. 

After this you can browse to: 
http://localhost:8000
and you can start debugging/changing the html and javascript code as you wish. 

# Board specifics
## TTGO T8 ESP32-C3
See: [LilyGo ESP32-C3 board with SD card interface](https://www.lilygo.cc/products/t8-c3)   
See: [NL Supplier link](https://www.tinytronics.nl/shop/en/development-boards/microcontroller-boards/with-wi-fi/lilygo-ttgo-t8-c3-esp32-c3-4mb-flash)

This is a LilyGO TTGO T8 ESP32-C3 module, which has an integrated SD card interface. 

- One needed to install special Serial USB drivers and reboot the PC or laptop first before the connection was even recognized as a serial port...
  See: [Espressif page](https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32c3/get-started/establish-serial-connection.html)
  Sometimes the serial port only comes up after using some other board or USB device as well ...
- To get the first Serial to work for debugging, we needed to set additional build flags so the first hardware serial goes out to the USB connector.
  ```` 
  build_flags = 
    -D TTGO_T8_ESP32_C3
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
  ````  
- To get the first upload working containing the above, one must manually enter program mode by:
  - Pressing the boot button (do not release)
  - Press the reset button and release
  - Release the boot button
  - Then upload...
  
- After investigation on the web and measuring, it seems the HardwareSerial.begin pulls up the RX pin as needed. 
  Measured 3.3V on RX1 -> PIN 2
  This would mean we don't need to foresee an external pull-up resistor !!!    
  We also discovered later that there is even an external-to-the-chip pull-up resistor.  

- Found out the hard way that pin 9 can not be used for RTS, because the connection to the DSRM pulls it down. This influences the ESP32 startup and it goes into program mode...

- The pinout of DSRM and colors for typical cables and connection to the ESP board are as follows: 

   |RJ12 PIN | P1 function | Wire color| ESP32 PIN | ESP Function | Comment |
   |---------|-------------|:---------:|--------|-----|---------|
   |P1 |  5V          | white  | 5V | 5V | 5V is near the SD Card interface on the board.<br> P1 is the leftmost wire in the RJ12 connector when the clip is on top | 
   |P2 | RTS Input    | black  | IO08 | RTS out | Active high input to let the meter send data (Active high due to optocoupler) |
   |P3 | GND for DATA | red    | | | Not used |   
   |P4 | No function  | green  | | | Not used
   |P5 |  TX          | yellow | IO02 | RX1 | Inverted due to opto coupler in meter. <br> Inverted in ESP/Software by HardwareSerial initialization | 
   |P6 |  GND for POWER | blue | GND | GND |

- Wiring overview: ![](doc/TTGO_T8_ESP32_C3/pinout%20V1.1.jpg)
- Soldered and assembled example:  ![](doc/TTGO_T8_ESP32_C3/T8-C3-soldered-assembled.jpg)


## TTGO T8 ESP32-S2
See: [LilyGo supplier](https://www.lilygo.cc/products/esp32-s2)   
See: [Tinytronics supplier in NL for the ESP32-S2 board with SD card interface](https://www.tinytronics.nl/shop/nl/development-boards/microcontroller-boards/met-wi-fi/lilygo-ttgo-t8-esp32-s2-met-sd-kaart-slot)   
See: [Github examples ](https://github.com/Xinyuan-LilyGO/ESP32_S2)


This is a "LILYGO ESP32-S2" module (V1.1)
This is a board that holds an interface to put in a micro SD card. 

- There are 2 DIP switch blocks on the board that we normally do not touch. 
  The first DIP switch block is close to the USB connector. 
  When the Connector is to the left of the board, the switch positions are: UP, DOWN, UP, DOWN
  This corresponds to "USB" mode (slave mode -> your PC is the master)
  In another position one gets OTG mode (master mode, the board is an USB master and it can support other devices) 
  We never use the OTG mode. 
  From what we can deduce OTG mode would interfere with the use of PIN 19 as an output signal for RTS. 

- Very special on this board is to get the SDCard working, one needs to set PIN 14 to HIGH. 
  This is not so visible on the schema, but otherwise, the V3V pin has no proper voltage and this also powers the SDCard...We spent some days finding this out...
 
After programming the board with the filesystem and code, it should start normally. 

- The pinout and colors are as follows: 
   |RJ12 PIN | P1 Function | Wire color | ESP32 PIN | ESP Function | Comment |
   |---------|-------------|:----------:|:----------|--------------|---------|
   | P1 | 5V         |  white  | 5V        | 5V | Left most wire in RJ12 connector with clip on top at digital meter |
   | P2 | RTS Input  |  black  | IO19      | RTS Out | Active high input to let the meter send data (Active high due to optocoupler) |
   | P3 | GND for DATA| red    |           | | Not used |
   | P4 | No function |green   |           | | Not used |
   | P5 | TX         | yellow  | IO18      | RX1 | Open collector output, hence inverted.<br> Inverted at reception by ESP Hardware serial initialization.<br> Pulled up on ESP board. | 
   | P6 | GND for POWER|blue   | GND       | GND ||

- Wiring overview: ![](doc/TTGO_T8_ESP32_S2/pinout%20V1.1.jpg)