#pragma once
#include "sd_fat32_fs_wrapper.h"
#include "idisplayer.h"
#include "fixed_quarter_power_history_accumulator.h"
#include <ArduinoJson.h>


// For SdFat library....
// using FAT32 for SDCards up to 16GB (but using 8GB standard)
#define SD_FAT_TYPE 1 
extern SdFat32 sd;

extern void sdcard_panic_print_char(const char c);
 
extern bool setupSDCard(IDisplayer& displayer);
extern void sdCardLogStartup(); 



extern void writeQuarterDataToFile(const QuarterIndicator& quarterId, const WattHistorySamples& samples );

extern void getYearsWithData(JsonObject& yearHolder);
extern void getYearMonthsWithData(JsonObject& year_months_holder, int year);
extern void getYearMonthDaysWithData(JsonObject& year_month_days_holder, int year, int month);
extern void getYearMonthDayQuartersWithData(JsonObject& year_month_day_quarters_holder, int year, int month, int day);
