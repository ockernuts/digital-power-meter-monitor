# What is this
This project is about monitoring a digital power meter installed in homes in Belgium, following the DSRM standard. 
The monitorring is done using an ESP32 board for which the software is here. 
We also describe then needed hardware connection to an RJ12 cable that one can plug into the power meter. 
The source code can deal with multiple boards in theory. 
In practice the LilyGo TTGO T8 ESP32-C3 board is mostly used and thus certainly working. 


# Using the source code
Make sure you have Visual Studio code with "Platform IO" installed. 
Platform IO has an Icon on the left side of Visual Studio code to activate it. 
Clicking on it, you should be able to see the "PIO Home" tab. 

Clicking on the project should show a tab per target hardware device. 

For example: "TTGO_T8_ESP32_C3"  

See below for hardware specific setup. 



# TTGO T8 ESP32-C3
- One needed to install special Serial USB drivers and reboot the pc or laptop first before the connection was even recognised as a serial port...
  Somethimes the serial port only comes up after using some other board and then switch back...
- To get the first Serial to work for debugging, we needed to set additional build flags so the hardware serial is used. 
  build_flags = 
    -D TTGO_T8_ESP32_C3
	-DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
- To get the first upload working containing the above, one must manually enter program mode by:
  - pressing the boot button (do not release)
  - press the reset button and release
  - release the boot button

  Then upload...
  
- After investigation on the web and measurring, it seams the HardwareSerial.begin pulls up the RX pin as needed. 
  Measured 3.3V on RX1 -> PIN 2
  This would mean we don't need to foresee an external pull-up resistor - yeah ! 
  Also discoverred later that there is even a resistor externally to the ESP chip to pull this up.  

- Found out the hard way that pin 9 can not be used for RTS, as the connection to the DSRM pulls it down. 
  Appearantly this influences the ESP32 startup and is goes in program mode...

A black RJ12 cable from "Inline Computer Accessories" ( bought on Amazon), has a strange color layout. The ALT COLOR below is the more normal color. 

- The pinout of DSRM and colors for the black cable are as follows (from LEFT to right when viewing with plug up with the lip on top): 
   PIN               COLORS  ALT COL
   P1   5V           white   white    - left most wire in RJ11 connector with clip on top at digital meter / most right wire at board 
   P2   RTS Input    brown   black    - Active high INPUT to let the meter send data (Active hight due to optocoupler)
   P3   GND for DATA green   red
   P4   No function  yellow  green
   P5   TX           grey    yellow   - open collector output. 
   P6   GND for POW  orange  blue

- The connections of the RJ12 pins to the board are as follows
               COLOR  ALT COLOR
  - P1 -  5V - white  white  -    5V -  5V on the board (near the sdcard connector). 
  - P2 - RTS - brown  black  -  IO08 - RTS on the other side of the board 3rd hole from the bottom. 
  - P3 - GND - green  red      (NC)
  - P4 -     - yellow green    (NC)
  - P5 - TX  - grey   yellow  -  IO02 - RX1 on the board , 4 holes above the 5V pin. Pulled up without soldering.
  - P6 - GND - orange blue    -  GND  - GND on the board, 3 holes above the 5V pin. 

# TTGO T8 ESP32-S2
See: https://github.com/Xinyuan-LilyGO/ESP32_S2

This is a "LILYGO ESP32-S2" module (V1.1)
This is a board that holds a socket to put in a micro SD card. 
The SD Card is used to put the per quarter measurred date on it. 
The board also has a USB module. 
There are 2 DIP switch blocks on the board that we normally do not touch. 
The first DIP sqwitch block is close to the USB connector. 
When the Connector is to the left of the board, the switch positions are: UP, DOWN, UP, DOWN
This corresponds to "USB" mode (slave mode -> your PC is the master)
In another position one gets OTG mode (master mode, the board is an USB master and it can suppor other devices) 
We never use the OTG mode. 

To initialize a board, we need to put on it two things:
- The CODE
- The "data" partition/little fs file system. 

First make sure the code can be build withouth troubles. 
Make sure there is an SD card in the board. 

Then plugin the USB cable in the board and connect to the PC. 
On the bottom of the VSCode window you can see a serial connector with either "Auto" or "COM<n>" next to it. 
Select here the port you think corresponds to the plugged in board. 

In the Project tasks select "Upload and Monitor". 
This will upload the code and then listen to serial output send to the pc by the embedded board program. 
This should normally lead to an output where you see the board starting to emit messages. 
If the "data" partition is not uploaded yet, the software will likely fail with the "Failed to Init LittleFS" or something like that. 

To upload the data partition/ little FS, leave the USB connected and use the "Build FIlesystem Image" option. 
The use the "Upload Filesystem Image" options. This is only needed once, unless files changes, but there is a better method...upload over wifi. (see below).  

Hereafter do an Upload and Monitor again and see that the system starts up better. 
Normally (on a blank board), this will emit messages like: 

  Ports used for SD Card communication:
  MOSI: 35
  MISO: 37
  SCK: 36
  SS: 34
  Serial pins used for communication to digital meter:
  RX1: 38, TX1: 40, RTS: 36
  LittleFS Init OK
  No prior WIFI config
      Setup Mode
  --------------------------------
  SSID:
  DigitaleMeterMonitor
  Connect IP:
  192.168.4.1

The device starts up as an access point with an SSID "DigitaleMeterMonitor". 
-> Let your PC Connect to this Wifi. 
In the serial monitor you will see a lot of messages like: 
[2124569][E][vfs_api.cpp:105] open(): /littlefs/204 does not exist, no permits for creation   
[2124571][E][vfs_api.cpp:105] open(): /littlefs/204.gz does not exist, no permits for creation
[2124576][E][vfs_api.cpp:105] open(): /littlefs/204/index.htm does not exist, no permits for creation   
[2124585][E][vfs_api.cpp:105] open(): /littlefs/204/index.htm.gz does not exist, no permits for creation
[2124850][E][vfs_api.cpp:105] open(): /littlefs/connecttest.txt does not exist, no permits for creation   
[2124852][E][vfs_api.cpp:105] open(): /littlefs/connecttest.txt.gz does not exist, no permits for creation
[2124860][E][vfs_api.cpp:105] open(): /littlefs/connecttest.txt/index.htm does not exist, no permits for creation
[2124869][E][vfs_api.cpp:105] open(): /littlefs/connecttest.txt/index.htm.gz does not exist, no permits for creation
...

Not a problem. 

On the PC open a browser an go to http://192.168.4.1
This should give you a page where you can setup the board to connect to your home Wifi. 
Make sure that you use a not used ipaddress. 

If you miss here, the device will after applying the settings restart and attempt to connect to the wifi, fail and 
become an access point again so that you can repeat the above steps.

When the Wifi is connecting fine you will see on the debug terminal:
  Connecting ...
  WL_DISCONNECTED
  WL_CONNECTED
  Connected !
  192.168.0.248
  Syncronizeer Tijd
  --------------------------------
  ........................Synced in 2400 ms.
  Init SD card - SPI...
  SD Card SPI Pins set
  SD Card OK
  Verbruiksmonitor
  --------------------------------
  
Here 192.168.0.248 is the wifi. Connect to it with your browser. 

You should see a black page with title "Digitale Meter Monitor" and some ? marks on meassured values. 
This is the case if the device is not connected to the digital meter....

Unplug the board from the pc and plug its RJ45 connector into the digital meter of your home. 

-----------------------------

Upgrading a board.
On can connect to the board with a browser:
http://<ipaddress>/update

This gives you a user interface where build output files can be uploaded for code or data, as needed. 

-----------------------------

New board hardware wiring needed...
- The digital meter P1 port can fit an RJ12 connector with 6 wires (6P6C needed not 6P4C). 
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