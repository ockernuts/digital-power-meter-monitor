#pragma once
#include "idisplayer.h"
#include <Adafruit_GFX.h>


class AdafruitGfxDisplayer : public IDisplayer {

    Adafruit_GFX& tft;

    const char* title;

public:
    AdafruitGfxDisplayer(Adafruit_GFX& tft) : tft(tft), title(nullptr) {}

    virtual void newPage(const char *title) {
      this->title = title;
      tft.fillScreen(0x0000);
      tft.setCursor(0,0);
      tft.setTextSize(2);
      tft.setTextColor(0xFFC0); // yellow
      tft.println(title);
      tft.setTextColor(0xFFFF); // white
    }

    virtual void print(const char * text) {
      int len = strlen(text);
      for (int i=0; i< len; i++) {
        tft.write(text[i]);
        if (tft.getCursorY() > (tft.width() - 2 * 8)) {
          if (title) {  // It might be not safe to keep using this...
            newPage(title);
          } else {
            newPage("Continued..");
          }
        }
      }
    }

    virtual void println(const char* text) {
      print(text);
      print("\n");
    }

    virtual void errorln(const char *content) {
      tft.setTextColor(0xFC00);
      println(content);
      tft.setTextColor(0xFFFF);
    }
    
};