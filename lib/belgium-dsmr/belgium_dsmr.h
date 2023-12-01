// #pragma once
#include <dsmr.h>  

// cool feature of the dsmr library: define own fields that it doesn't cover.
namespace dsmr {
  namespace fields {
    DEFINE_FIELD(month_quarterly_peak, TimestampedFixedValue, ObisId(1, 0, 1, 6, 0), TimestampedFixedField,units::kW,units::W);
  }
}


struct P1Data {
  int watts_consumed = 0;
  int watts_produced = 0;
  struct tm monthly_peak_timestamp = {};
  int monthly_peak_watts = 0;
  struct tm message_timestamp = {};
};

extern P1Reader& GetP1Reader();

extern bool ProcessP1ReaderResults(struct P1Data& data);