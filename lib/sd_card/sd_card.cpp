
#include <Arduino.h>
#include <SPI.h>
#include "sd_card.h"
#include <ArduinoJson.h>
#include "json_quarter_info_creator.h" 
#include "hardware_and_pins.h"


SdFat32 sd;

// Special : expose it also through an FS wrapper, so we can use is in libraries that are compatible with FS and File "interfaces"
fs::FS SdFat32Fs = fs::FS(fs::FSImplPtr(new SdFat32FSImpl(sd)));

void createPath(char * path) {
  if (!sd.exists(path)) {
    if (!sd.mkdir(path)) {
      Serial.print("Could not create:");
      Serial.println(path);
    }
  }
  yield();
}

File32 sdcardCrashLogFile; 
bool firstPanicPrint = true;


void sdcard_panic_print_char(const char c) {
  if (!sdcardCrashLogFile) return;
  if (firstPanicPrint) {
    firstPanicPrint = false;
    sdcardCrashLogFile.write("BOE!\n"); 
  }
  sdcardCrashLogFile.write(c);
  if (c == '\n') {
    sdcardCrashLogFile.flush();
  }
}

void createDirForDayIfNeeded(int year, int month, int day) {
  char path[32];
  snprintf(path, sizeof(path),"/meter/%04d", year);
  createPath(path);

  snprintf(path, sizeof(path),"/meter/%04d/%02d", year, month);
  createPath(path);
  
  snprintf(path, sizeof(path),"/meter/%04d/%02d/%02d", year, month, day);
  createPath(path);
}


bool setupSDCard(IDisplayer& displayer) {
 
  displayer.println("Init SD SLOT SPI...");
  SPI.begin(SD_CARD_SCK, SD_CARD_MISO, SD_CARD_MOSI, SD_CARD_SS); 
  
  displayer.println("SD SLOT SPI Pins set");


  if (!SD_CARD_INIT) {
    displayer.println("Init failed ...");
    displayer.println("Is a card inserted?");
    return false; 
  }

  Serial.printf("SD Card sectorsPerCluster  %d\n", sd.sectorsPerCluster());


  displayer.println("SD Card OK");
 
  if (!sd.exists("/meter")) {
    if (!sd.mkdir("/meter")) {
      Serial.println("!!!!! ERROR could not write to SD Card to make /meter folder - Needs formatting ?");
    }
  }
  
  Serial.println("Creating test dir /meter/2022/01/01");
  createDirForDayIfNeeded(2022,1,1);
  if (!sd.exists("/meter/2022/01/01")) {
    Serial.println("!!! ERROR Creation of meter YMD dir test failed. The directory does not exist");
  }

  Serial.println("Creating test file /meter/2022/01/01/hello.txt");
  File32 f = sd.open("/meter/2022/01/01/hello.txt", O_CREAT | O_WRITE);
  if (!f) {
    Serial.println("!!! ERROR Creating hello.txt file in meter YMD test dir");
  } else {
    int err = f.write("hello world!");
    if (err == -1) {
      Serial.println("!!! ERROR writing to hello.txt file in meter YMD test dir");
    }
    if (!f.close()) {
      Serial.println("!!! ERROR closing hello.txt file test"); 
    };
  }

  sdcardCrashLogFile = sd.open("crashlog.txt", O_APPEND | O_CREAT | O_WRONLY);
  return true; 
}

void sdCardLogStartup() {
  struct timeval tv;
  struct tm *tm;

  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);

  char timeString[20];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", tm);

  Serial.println(timeString);
  sdcardCrashLogFile.write("Started up @");
  sdcardCrashLogFile.write(timeString);
  sdcardCrashLogFile.write('\n');
  sdcardCrashLogFile.flush();
}


void writeQuarterDataToFile(const QuarterIndicator& quarterId, const WattHistorySamples& samples ) {
  createDirForDayIfNeeded(quarterId.GetYear(), quarterId.GetMonth(), quarterId.GetDay());

  char path[32];
  // Like 2023/01/03/1815W.json
  snprintf(path, sizeof(path), "/meter/%04d/%02d/%02d/%02d%02d%c.json", 
           quarterId.GetYear(), quarterId.GetMonth(), quarterId.GetDay(),
           quarterId.GetHour(), quarterId.GetQuarterOfTheHour() *15, quarterId.IsSummerTime() ? 'S' : 'W');
  File32 f = sd.open(path, O_CREAT | O_WRITE);
  if (!f) {
    // TODO how to say something about this.
    return; 
  }
  DynamicJsonDocument doc(6144); // We could not use a StaticJsonDocument of this size, since it ruïnes the limited stack (4KB) on the ESP8266
  JsonObject root = doc.to<JsonObject>();
  GetJsonQuarterInfoCreator().FillJsonObjectForQuarterWattHistorySamples(root, quarterId, samples);
  serializeJson(doc, f);
  f.close();
}


int getNumberFromFileName(File32 &f ) {
  char file_name[16];
  f.getName(file_name, sizeof(file_name));
  return atoi(file_name);
}

void getYearsWithData(JsonObject& year_holder) {
    File32 meter_dir = sd.open("/meter");
    if (!meter_dir) return;

    JsonArray years = year_holder.createNestedArray("years");

    while (true) {
      File32 year_entry =  meter_dir.openNextFile();
      if (! year_entry) {
        // no more files
        break;
      }
      if (year_entry.isDirectory())  {
        years.add(getNumberFromFileName(year_entry));
      }
      close(year_entry);
    }
    close(meter_dir);
}

void getYearMonthsWithData(JsonObject& year_months_holder, int year) {
    char path[32];
    snprintf(path, sizeof(path), "/meter/%04d", year);

    File32 year_dir = sd.open(path);
    if (!year_dir) return;

    year_months_holder["year"] = year;
    JsonArray months = year_months_holder.createNestedArray("months");

    while (true) {
      File32 month_entry =  year_dir.openNextFile();
      if (!month_entry) {
        // no more files
        break;
      }
      if (month_entry.isDirectory()) {
        months.add(getNumberFromFileName(month_entry));
      }
      close(month_entry);
    }
    close(year_dir);
}

void getYearMonthDaysWithData(JsonObject& year_month_days_holder, int year, int month) {
    char path[32];
    snprintf(path, sizeof(path), "/meter/%04d/%02d", year, month);

    File32 year_month_dir = sd.open(path);
    if (!year_month_dir) return;

    year_month_days_holder["year"] = year;
    year_month_days_holder["month"] = month;
    JsonArray days = year_month_days_holder.createNestedArray("days");

    while (true) {
      File32 day_entry =  year_month_dir.openNextFile();
      if (! day_entry) {
        // no more files
        break;
      }
      if (day_entry.isDirectory()) {
        days.add(getNumberFromFileName(day_entry));
      }
      close(day_entry);
    }
    close(year_month_dir);
}

void getYearMonthDayQuartersWithData(JsonObject& year_month_day_quarters_holder, int year, int month, int day) {
    char path[32];
    snprintf(path, sizeof(path), "/meter/%04d/%02d/%02d", year, month, day);

    File32 year_month_day_dir = sd.open(path);
    if (!year_month_day_dir) return;

    year_month_day_quarters_holder["year"] = year;
    year_month_day_quarters_holder["month"] = month;
    year_month_day_quarters_holder["day"] = day;
    JsonArray quarters = year_month_day_quarters_holder.createNestedArray("quarters");

    while (true) {
      File32 quarter_entry =  year_month_day_dir.openNextFile();
      if (! quarter_entry) {
        // no more files
        break;
      }
      if (!quarter_entry.isDirectory()) {
        int hour;
        int quarter_minutes; 
        char summer_or_winter; 
        char extension[5];
        char file_name[16];
        quarter_entry.getName(file_name, sizeof(file_name));
        if (sscanf(file_name, "%02d%02d%c.%4s", &hour, &quarter_minutes, &summer_or_winter, extension ) == 4) {
          if (strcmp("json", extension) == 0) {
            QuarterIndicator quarterIndicator(year, month, day, hour, quarter_minutes / 15, summer_or_winter == 'S');
            quarters.add(quarterIndicator.GetLocalQuarterId());
          }
        }
      }
      close(quarter_entry);
    }
    close(year_month_day_dir);
}


