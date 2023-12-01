#include "hardware_and_pins.h"
#include <ezLED.h>



HardwareSerial& GetDsmrSerial() {
    static HardwareSerial DsmrSerial(DSMR_UART);
    return DsmrSerial; 
}

// See also being statement in setup to set this up. 
// We also invert the signal in the HardwareSerial.begin, so that we don't need a transistor after the digital meter's output. 
// This output is done using an opto-coupler with open collector output, hence and inverted signal.
// So we would only need to pull up this signal to 3.3V and use as RX1 input.
void InitDsmrSerial() {
    Serial.println("Serial pins used for communication to digital meter: ");
    Serial.printf("RX1: %d, TX1: %d, RTS %d\n", RX1, TX1, DSMR_RTS);
    GetDsmrSerial().begin(115200, SERIAL_8N1, RX1, TX1, true /* invert */);
}

ezLED* g_led; 

void initLed() {
    static ezLED led(LED);
    g_led = &led;
}

void loopLed() {
    g_led->loop();
}

void setLedOn(bool async, int animation_time, int strength) {
    g_led->fade(0, strength, animation_time);
    if (async) return;
    for (int i=0; i<animation_time; i+=10) {
        delay(10);
        g_led->loop();
    }
}

void setLedOff(bool async, int animation_time, int strength) {
    g_led->cancel();
    g_led->fade(strength, 0, animation_time);
    if (async) return;
    for (int i=0; i<animation_time; i+=10) {
        delay(10);
        g_led->loop();
    }
}


