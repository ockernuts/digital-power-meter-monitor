 
#include "belgium_dsmr.h"
#include "hardware_and_pins.h"

// Needs some instanciation as well or you get a linker error...
constexpr ObisId month_quarterly_peak::id;
constexpr char month_quarterly_peak::name_progmem[];

// Then other cool feature is that you can make a struct (view) of the data you need.
using MyData = ParsedData<
  /* String */     timestamp,
  /* String */     identification,
  /* FixedValue */ energy_delivered_tariff1,
  /* FixedValue */ energy_delivered_tariff2,
  /* FixedValue */ energy_returned_tariff1,
  /* FixedValue */ energy_returned_tariff2,
  /* FixedValue */ power_delivered,
  /* FixedValue */ power_returned,
  /* String */     month_quarterly_peak
>;

P1Reader& GetP1Reader() {
  static P1Reader digital_meter_reader(&GetDsmrSerial(), DSMR_RTS );
  return digital_meter_reader;
}


bool dsmr_parse_datetime(const char * datetime_dsmr_format, struct tm* tm) {
  // Parse the DSMR string format. For example 230103103000W refers to 2023, Month 01, Day 03, 10:30:00 in local wintertime. 
  // 230401103000S, refers to 2023/04/01 10:30:00 in summertime. 
  char summer_or_winter; 
  int parse_count = sscanf(datetime_dsmr_format,"%02d%02d%02d%02d%02d%02d%c",
                           &tm->tm_year, &tm->tm_mon, &tm->tm_mday,
                           &tm->tm_hour, &tm->tm_min, &tm->tm_sec, &summer_or_winter);
  if (parse_count != 7) {
     return false;
  }

  // The year is 2 digit, so misses 2000 years of civilization, but the time structure only counts from 1900
  tm->tm_year += 100;
  tm->tm_mon -=1; // month index is from 0..11 ...
  tm->tm_isdst = summer_or_winter == 'S' ? 1 : 0; 
  // the side effect of "conversion to time_t" is that the weekday info gets filled in ...
  mktime(tm);
  return true;
}

const char *getWeekday(const struct tm* tm_time) {
  static const char * dutch_weekdays[7] = {"zo", "ma", "di", "wo", "do", "vr", "za"};
  return dutch_weekdays[tm_time->tm_wday];
}

bool ProcessP1ReaderResults(P1Data& returned_data) {
  P1Reader& p1_reader  = GetP1Reader();
  if (!p1_reader.loop()) {
    return false; 
  }

   
  // Internal data structure of DSMR library to get readings 
  // This will be translated to "returned_data" format. 
  MyData data;
  if (!p1_reader.parse(&data, nullptr)) {
    // TODO Keep statistics ? 
    p1_reader.enable(true);
    return false; 
  }

  if (!data.all_present()) {
    p1_reader.enable(true);
    return false; 
  }

  returned_data.watts_consumed = data.power_delivered.int_val(); // @suppress("Method cannot be resolved")
  returned_data.watts_produced = -data.power_returned.int_val();  // @suppress("Method cannot be resolved")
  returned_data.monthly_peak_watts = data.month_quarterly_peak.int_val();
  returned_data.consumed_wh = data.energy_delivered_tariff1.int_val() + data.energy_delivered_tariff2.int_val();
  returned_data.produced_wh = data.energy_returned_tariff1.int_val() + data.energy_returned_tariff2.int_val();

  if (!dsmr_parse_datetime( data.month_quarterly_peak.timestamp.c_str() , &returned_data.monthly_peak_timestamp)) {
    p1_reader.enable(true);
    return false; 
  }  
  // Special, the month average is only reported after the quarter, so to get the proper quarter_id later
  // we must substract 15 minutes from the reading...
  returned_data.monthly_peak_timestamp.tm_min -=15;
  // the mktime fuction normalizes the time, so also adjusts to the previous hour, day, year if needed. 
  mktime(&returned_data.monthly_peak_timestamp);


  if (!dsmr_parse_datetime( data.timestamp.c_str() , &returned_data.message_timestamp)) {
    p1_reader.enable(true);
    return false; 
  }

  return true; 
}