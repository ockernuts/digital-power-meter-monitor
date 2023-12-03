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

#define DOT_TIME 150
#define DASH_TIME 3*DOT_TIME
#define INTER_DASH_OR_DOT_SLEEP DOT_TIME
#define INTER_CHARACTER_SLEEP 3*DOT_TIME
#define INTER_WORD_SLEEP 7*DOT_TIME

constexpr const char *ONE= ".----";
constexpr const char *TWO = "..---";
constexpr const char *THREE = "...--";
constexpr const char *FOUR = "...._";
constexpr const char *FIVE = ".....";
constexpr const char *SIX = "-....";
constexpr const char *SEVEN = "--...";
constexpr const char *EIGHT = "---...";
constexpr const char *NINE = "----.";
constexpr const char *ZERO = "-----";

const char *morse_az[26] = { ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
                             "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", 
                             "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.." };

const char * morse_numbers[10] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE};

void morseDash(int pin, int strenght) {
    analogWrite(pin,strenght);
    delay(DASH_TIME);
    analogWrite(pin,0);
}

void morseDot(int pin, int strenght) {
    analogWrite(pin,strenght);
    delay(DOT_TIME);
    analogWrite(pin,0);
}

void morseCharacterCodeOut(int pin, const char * code, int strength) {
    int size= strlen(code);
    for (int i=0; i<size; i++) {
       Serial.print(code[i]);
       if (i>0) delay(INTER_DASH_OR_DOT_SLEEP);
       if (code[i] == '.') {
          morseDot(pin, strength);
       } else if (code[i] == '-') {
          morseDash(pin, strength);
       } else {
          sleep(14*DOT_TIME); // program error condition, unknown, just wait long. 
       }
    }
}

void morseChar(int pin, char token, int strength) {
    if (token >='a' && token <='z') {
        morseCharacterCodeOut(pin, morse_az[token - 'a'], strength);
    }  else if (token >= 'A' && token <= 'Z') {
        morseCharacterCodeOut(pin, morse_az[token - 'A'], strength);
    }
    else if (token >='0' && token <='9') {
        morseCharacterCodeOut(pin, morse_numbers[token-'0'], strength);
    } else if (token == ' ') {
        delay(INTER_WORD_SLEEP);
    }
}
void morseInterchar() {
    delay(INTER_CHARACTER_SLEEP);
}

void morseInterWord() {
    delay(INTER_WORD_SLEEP);
}

void morseOut(int pin, const char *text, int strenght) {
    Serial.print(text);
    Serial.print(" -> outputing on led in morse code:");
    int size = strlen(text);
    for(int i=0; i<size; i++) {
        if (i>0 && text[i]!=' ') morseInterchar();
        if (text[i] == ' ') {
            morseInterWord();
            Serial.print(" ");
        }
        morseChar(pin, text[i], strenght);
    }
    morseInterWord();
    Serial.println(" -- done !");
}



