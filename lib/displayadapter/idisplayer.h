#pragma once

/// @brief abstraction to resolve differences in output needed when 
///        emitting info on a tft screen vs on a serial port. 
///        For example on a tft screen, a new page will also need a clear, set cursor stuff...
///        Also when emitting more text than available on the screen on can in an implementation 
///        see how that is dealt with. 
class IDisplayer {
public:

    virtual ~IDisplayer() {};

    virtual void newPage(const char * title) = 0;

    virtual void print(const char * content) = 0;

    virtual void println(const char *content) = 0;

    virtual void errorln(const char *content) = 0;
};
