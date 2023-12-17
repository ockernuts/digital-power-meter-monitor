// #pragma once
#include <dsmr.h>  

// cool feature of the dsmr library: define own fields that it doesn't cover.
namespace dsmr {
  namespace fields {
    DEFINE_FIELD(month_quarterly_peak, TimestampedFixedValue, ObisId(1, 0, 1, 6, 0), TimestampedFixedField,units::kW,units::W);
  }
}


class P1Data {
public:
  int watts_consumed = 0;
  int watts_produced = 0;
  int consumed_wh = 0;
  int produced_wh = 0;
  struct tm monthly_peak_timestamp = {};
  int monthly_peak_watts = 0;
  struct tm message_timestamp = {};

  int get_consumed_wh() const {
    return consumed_wh;
  }

  int get_produced_wh() const {
    return produced_wh;
  }
};

extern P1Reader& GetP1Reader();

extern bool ProcessP1ReaderResults(P1Data& data);