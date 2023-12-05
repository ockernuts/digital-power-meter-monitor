#pragma once
#include <Arduino.h>


#define OUTPUT_TYPE_TFT 1
#define OUTPUT_TYPE_SERIAL 2
#define OUTPUT_TYPE_SERIAL1 3



#define SOLUTION_ESP32_S2_MINI_NOTFT 1


#define SOLUTION SOLUTION_ESP32_S2_MINI_NOTFT

#if SOLUTION == SOLUTION_ESP32_S2_MINI_NOTFT
// Choose from above constants here how the feedback/ ouput will be given (tft screen or serial...)
// For ESP_S2 this is the normal serial that goes over USB. 
#define PROGRAM_MAIN_OUTPUT OUTPUT_TYPE_SERIAL
#endif

// For readout of the digital smart meter (DSMR), a different serial port must be used. 
// ESP32s can use any pin for the UARTs. THe used pins are defined below per board. 
// The value below is used to define another HardwareSerial instance 
#define DSMR_UART 1


#ifdef LOLIN_S2_MINI
#define RX1 38
#define TX1 40
#define DSMR_RTS 36
#define LED LED_BUILTIN

// For example with LOLIN S2 MINI
// Standard SPI (2?) using pins: 
// SCK 7
// MISO 9
// MOSI 11
// CS/SS 12
const int SD_CARD_MISO = MISO;
const int SD_CARD_SCK = SCK;
const int SD_CARD_MOSI = MOSI;
const int SD_CARD_SS = SS;

#define SD_CARD_INIT sd.begin(SdSpiConfig(SD_CARD_SS, SHARED_SPI, SD_SCK_MHZ(18), &SPI))

// TFT (Optional pins to be adapted, still from ESP8266 )
#define TFT_DC    D8
#define TFT_RST   D6
#define TFT_MOSI  D7
#define TFT_SCLK  D5

#define BOARD_NAME "Wemos ESP32 S2 Mini"

#endif

#ifdef TTGO_T8_ESP32_S2
#define RX1 18 // Has pull up resistor on schema
#define TX1 17
#define DSMR_RTS 19
#define LED 36

// derived from : https://github.com/Xinyuan-LilyGO/LilyGo-esp32s2-base
const int SD_CARD_MISO = 13;
const int SD_CARD_SCK = 12;
const int SD_CARD_MOSI = 11;
const int SD_CARD_SS = 10;
// not sure if the above works... see: https://www.reddit.com/r/esp32/comments/wy880v/anyone_got_sd_card_running_on_ttgo_t8/
// only needed when battery powerred ? 
const int POWER_LED_AND_SD_CARD = 14;

#define SD_CARD_INIT sd.begin(SD_CARD_SS)

#define BOARD_NAME "LilyGo TTGO T8 ESP32 S2 V1.1"

// The boot button can be used to remove/wipe the Wifi config and restart.
#define ENABLE_BOOT_BUTTON_WIFI_RESET
#define BOOT_BUTTON 0 

#endif

#ifdef TTGO_T8_ESP32_C3
#define LED 3
#define RX1 2
#define TX1 0
// do not use pin IO09, because the DSRM pulls it low and puts the ESP in download mode...
#define DSMR_RTS 8

const int SD_CARD_MISO = 5;
const int SD_CARD_SCK = 4;
const int SD_CARD_MOSI = 7;
const int SD_CARD_SS = 6;

#define SD_CARD_INIT sd.begin(SD_CARD_SS)

#define BOARD_NAME "LilyGo TTGO T8 ESP32 C3 V1.1"

// The boot button can be used to remove/wipe the Wifi config and restart.
#define ENABLE_BOOT_BUTTON_WIFI_RESET
#define BOOT_BUTTON 9 

#endif


// Methods that will return and initialize the proper serial to read out the digital meter. 
extern HardwareSerial& GetDsmrSerial();
extern void InitDsmrSerial();


// Led animation related methods
#define LED_STRENGTH 4
#define LED_STRENGHT_STARTUP 16
#define LED_STRENGHT_ERROR 32
#define LED_ANIMATION_TIME_IN_MS 100
extern void initLed();
extern void loopLed();
extern void setLedOn(bool async=true, int animation_time=LED_ANIMATION_TIME_IN_MS, int strength=LED_STRENGTH);
extern void setLedOff(bool async=true, int animation_time=LED_ANIMATION_TIME_IN_MS, int strength=LED_STRENGTH);

extern void morseOut(int pin, const char * text, int strenght=LED_STRENGHT_STARTUP);