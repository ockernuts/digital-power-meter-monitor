# Digital power meter monitor
The digital power meter monitor is for power meters installed in Belgium. It can measure and show the current or past power measurements in realtime on your phone or pc/laptop.
Screenshots are in Dutch: 

![](doc/app/current_measurement_viewing.png)

The digital power meter a "P1 port" through wich the measurements can be received. 
> Using the "S1 port" will likely destroy the monitor. 

![](doc/ports_digital_meter.png)
The P1 port is disabled by default and can be opened through an action on the website of your network operator (like Fluvius) typically. Though, it can take days for it to get activated. 

The monitorring is done using an ESP32 board for which the software is here. 
In sections below the needed hardware connections to an RJ12 cable that one can plug into the power meter P1 port are described per supported ESP32 board. You need an RJ12 cable with 6 wires, called: 6P6C, not 6P4C

Detailed measurements per quarter are stored on an SDCard in json format. Hence we focus on boards that have the SDCard reader integrated, leaving other options open. Use an 8GB micro SD card or up to 64GB (only tested up to 16GB). It is best to reformat the SD Card first and to choose a small cluster size, because the stored files are all small. 

The board is connected to Wifi and exposes a web interface, meant for in home access only (http), on which one can see the current measurements almost in realtime (updated every 5 seconds by default) or navigate to historical measurements.

The measurements are show per fixed quarters in the hour because the average watt consumption per quarter can be penalized by higher electricity costs when they exceed 2500 Watts. One incurs an additional cost of approximately 4 euros (as of the 2023 reference) per each exceeded 1000 Watt on the highest quarter-average consumption per month. This pertains to the capacity tariff, known as 'capaciteits tarief' in Dutch."

One can easily jump back to the details of the quarter causing the month peak by pressing the "month peak" button if the monitor measured that quarter. 

![](doc/app/month_peak_viewing.png)


The source code can deal with multiple boards in theory. 
In practice the LilyGo TTGO T8 ESP32-C3 board is mostly used and thus certainly working. It is also the most power efficient.


# Using the source code
Make sure you have Visual Studio Code with "Platform IO" installed as IDE/Tooling environment. 
Platform IO has an alien alike icon on the left side of Visual Studio Code to activate it. 
Clicking on it, you should be able to see the "PIO Home" tab. 

Clicking on the project should show a tab per target hardware device. 

For example: "TTGO_T8_ESP32_C3"  

See specific board sections below for their hardware specific setup.

To get a working board, you need to build and upload the filesystem image and the C/C++ code. (Do this with Platform IO Project Tasks). Their might be board specific actions needed to get it in a state where the items can be uploaded over the USB cable. 

Make sure there is an SD card in the board. 


# Using the board with the installed code
## First startup or statup with Wifi connection problems
Upon first startup the board will try to locate Wifi connection parameters from the config.json file on the SDCard. 
If this file doesn't exist or the board cannot connect to the Wifi network, it will startup in configuration mode and act as an access point for a Wifi network "DigitaleMeterMonitor". This uses an IP Subnet 192.168.4.0/24 on which the board has the IP 192.168.4.1.
As such one can connect to that Wifi network and browse to http://192.168.4.1 to configure the Wifi settings.
Sometimes it is difficult to stay on the board's network since it has no path to the internet.  

In the configuration the board will require a :
- Wifi network / SSID to connect to
- Wifi network password
- IPAddress: The board needs a fixed IPAddress for in home use (No DHCP). Choose a free address, typically ending in a high value. Our default is 192.168.0.250.  
- Subnet: This is typically 192.168.0.0/24 . This depends on your in house setup / Telecom provider. 

Once this config is submitted it will be saved to the SDCard in the config.json file. 

The board will then reset and attempt to connect to the Wifi. 

Upon startup the onboard LED will light up bright in several sequences. 
Eventually the onboard LED should dim. This means it started up ok and is awaiting data from the digital power meter. 
If the board is connected to the digital power meter using the RJ12 cable (see pinout and connection schemas for each board in sections below), the dimmed LED should even pulsate a couple of times per second. This indicates the measurements are comming in. 

## Viewing the measurements
If the board is running you should be able to contact it on it's ip address over http. By default this is http://192.168.0.250
You should then see the measurements. 

## Alternative Wifi config
An alternative way to configure the board to proper Wifi is to power it off, unmount the sdcard from it and put it in a laptop/pc from where you can edit the config.json. 

The config.json looks like:
````
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
````

Change what you want, save, properly unmount the card from the laptop/pc and remount it on the board. 
Connect the board. 

## Debug the startup with the serial monitor
Upon trouble with startup, one can also use the Platform IO serial monitor to see what the board is doing upon startup. 
If monitorred serially it emits messages like:

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
  No prior WIFI config
      Setup Mode
  --------------------------------
  SSID:
  DigitaleMeterMonitor
  Connect IP:
  192.168.4.1
  ````

The pin values will vary per board type. 

In this example, the dhe device starts up as an access point with an SSID "DigitaleMeterMonitor". 

When the Wifi is connecting fine you will see on the debug terminal:
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

After the board has software and can connect to Wifi, it can be upgraded by browsing to:
````
http://<ipaddress>/update
````

This gives you a user interface where build output files can be uploaded for code or data(file system), as needed. 

# Develop on the web application served
The data folder contains the files for the web application. 
Your main intrest will be the index.html file. 
This gets current and historic data using HTTP GET calls like: 
- GET http://&lt;ip&gt;/current/quarter 
- GET http://&lt;ip&gt;>/meter/2023/12/01/1615W.json
- GET http://&lt;ip&gt;>/current/month/peak

The index.html page detects when it is served from localhost. In this case it does the above GET calls to 192.168.0.250 hard coded, to facilitate in debugging. 

There is a python httpserver.py file in the data folder which can be run to start service index.html on the local port 8000:
````
  cd data
  python httpserver.py
````
Obviously you need a version of python installed. Tested with python 3.11.1 on a windows pc. 

After this you can browse to: 
http://localhost:8000
and you can start debugging / changing the html and javascript code as you wish. 

You can do development on your computer with 

# Board specifices
## TTGO T8 ESP32-C3
See: [LilyGo ESP32-C3 board with SDCard interface](https://www.lilygo.cc/products/t8-c3)   
See: [NL Supplier link](https://www.tinytronics.nl/shop/en/development-boards/microcontroller-boards/with-wi-fi/lilygo-ttgo-t8-c3-esp32-c3-4mb-flash)

This is a LilyGO TTGO T8 ESP32-C3 module, which has an integrated SDCard interface. 

- One needed to install special Serial USB drivers and reboot the pc or laptop first before the connection was even recognised as a serial port...
  See: [Espressif page](https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32c3/get-started/establish-serial-connection.html)   
  Somethimes the serial port only comes up after using some other board and then switch back...
- To get the first Serial to work for debugging, we needed to set additional build flags so the first hardware serial goes out to the USB connector.
  ```` 
  build_flags = 
    -D TTGO_T8_ESP32_C3
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
  ````  
- To get the first upload working containing the above, one must manually enter program mode by:
  - pressing the boot button (do not release)
  - press the reset button and release
  - release the boot button

  Then upload...
  
- After investigation on the web and measurring, it seems the HardwareSerial.begin pulls up the RX pin as needed. 
  Measured 3.3V on RX1 -> PIN 2
  This would mean we don't need to foresee an external pull-up resistor - yeah ! 
  Also discoverred later that there is even a resistor externally to the ESP chip to pull this up.  

- Found out the hard way that pin 9 can not be used for RTS, as the connection to the DSRM pulls it down. 
  Appearantly this influences the ESP32 startup and is goes in program mode...

- The pinout of DSRM and colors for typical cables and connectio to the ESP board are as follows: 

   |RJ12 PIN         | COLORS | ALT COL | ESP32 PIN | Comment |
   |----------------:|:------:|:-------:|:--------|---------|
   |P1   5V          | white  | white   | 5V (near SD Card) | Left most wire in RJ11 connector with clip on top at digital meter / most right wire at board | 
   |P2  RTS Input    | black  | brown   | IO08 - RTS out | Active high INPUT to let the meter send data (Active high due to optocoupler) |
   |P3  GND for DATA | red    | green   | | Not used |   
   |P4  No function  | green  | yellow  | | Not used
   |P5   TX          | yellow | grey    | IO02 RX1 | Inverted due to opto coupler in meter| 
   |P6   GND for POW | blue   | orange  | GND | |

- Wiring overview: ![](doc/TTGO_T8_ESP32_C3/pinout%20V1.1.jpg)
- Soldered and assembled example:  ![](doc/TTGO_T8_ESP32_C3/T8-C3-soldered-assembled.jpg)


## TTGO T8 ESP32-S2
> **Warning**
> There is still a problem with this board to access the SDCard properly

See: [Tinytronics supplier in NL for the ESP32-S2 board with SDCard interface](https://www.tinytronics.nl/shop/nl/development-boards/microcontroller-boards/met-wi-fi/lilygo-ttgo-t8-esp32-s2-met-sd-kaart-slot)   
See: [Github use examples](https://github.com/Xinyuan-LilyGO/ESP32_S2)


This is a "LILYGO ESP32-S2" module (V1.1)
This is a board that holds an interface to put in a micro SD card. 

- There are 2 DIP switch blocks on the board that we normally do not touch. 
  The first DIP sqwitch block is close to the USB connector. 
  When the Connector is to the left of the board, the switch positions are: UP, DOWN, UP, DOWN
  This corresponds to "USB" mode (slave mode -> your PC is the master)
  In another position one gets OTG mode (master mode, the board is an USB master and it can suppor other devices) 
  We never use the OTG mode. 
 
After programming the board with the filesystem and code, it should startup normally. 



New board hardware wiring needed...

  If you buy a cable and cut it in half to solder on end to the board, you have one end with "normal" colors and one end with "reversed" colors. 
- The pinout and colors are as follows (from LEFT to right when viewing with plug up with the lip on top): 
   PIN               COLORS  ALT COL
   P1   5V           white - left most wire in RJ11 connector with clip on top at digital meter / most right wire at board 
   P2   RTS Input    bl?ack - Active high INPUT to let the meter send data (Active hight due to optocoupler)
   P3   GND for DATA re?d
   P4   No function  gr?een
   P5   TX           yel?low - open collector output. 
   P6   GND for POW  whi?te   blue 

- The connections of the RJ12 pins to the board are as follows
  - P1 to 5V on the board (near the on/off switch). 
  - P2 to pin 19 on the board, 2 holes above the 5V pin. 
  - P3 + P6 GND to a GND on the board (other side than 5V)
  - P5 to pin 18 on the board (RX). pin 18 is two holes up from pin 19 ! 
    This pin 18 is pulled up to 3.3V internally automatically on this board, matching with the open collector output