#pragma once
#include "idisplayer.h"
#include "Print.h"

class PrintDisplayer : public IDisplayer {

    Print& printCapable;

public:
    PrintDisplayer(Print& printCapable) : printCapable(printCapable) {}

    virtual void newPage(const char *title) {
      printCapable.println(title);
      printCapable.println("--------------------------------");
    }

    virtual void print(const char * text) {
      printCapable.print(text);
    }

    virtual void println(const char* text) {
      printCapable.println(text);
    }
    
    virtual void errorln(const char *content) {
        printCapable.print("!!! ");
        printCapable.print(content);
        printCapable.println(" !!!");
    }
};