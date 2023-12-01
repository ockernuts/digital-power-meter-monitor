/*
 * FixedQuarterPowerHistoryAccumulator.cpp
 *
 *  Created on: 15 feb. 2023
 *      Author: Wim
 */

#include "fixed_quarter_power_history_accumulator.h"
#include "sd_card.h"
#include "belgium_dsmr.h"


FixedQuarterPowerHistoryAccumulator& GetFixedQuarterPowerHistoryAccumulator() {
    static FixedQuarterPowerHistoryAccumulator watt_history; 
    return watt_history;
}

FixedQuarterPowerHistoryAccumulator::FixedQuarterPowerHistoryAccumulator():
  month_quarter_peak_power(0),
  month_quarter_peak_id(0),
  storing_for_quarter(),
  storing_for_interval_in_quarter_index(-1),
	watt_limits{0, 0},
	average_peak_warning(nullptr)
{
  for (int quarter=0; quarter < QUARTERS_TO_KEEP_IN_RAM; quarter++) {
    array_per_quarter_samples[quarter] = WattHistorySamples();
  }
  ResetAccumulationIntervalValues();
}

FixedQuarterPowerHistoryAccumulator::~FixedQuarterPowerHistoryAccumulator() {
}

/*
void FixedQuarterPowerHistoryAccumulator::InitForTest() {
  for (int i=-10; i < watt_history_samples-10; i++) {
    ProcessNewSample(i*20, 0);
  }
}
*/

void FixedQuarterPowerHistoryAccumulator::ResetAccumulationIntervalValues() {
  consumed_watt_samples_in_accumulation_interval = 0;
  consumed_watt_sum_in_accumulation_interval = 0;

  produced_watt_samples_in_accumulation_interval = 0;
  produced_watt_sum_in_accumulation_interval = 0;
}

void FixedQuarterPowerHistoryAccumulator::ResetQuarterValues(int quarter_of_the_hour) {
  array_per_quarter_samples[quarter_of_the_hour] = WattHistorySamples();
  watt_limits.peak_watts = 0;
  watt_limits.min_watts = 0;
}

int FixedQuarterPowerHistoryAccumulator::GetIntervalInQuarter(const tm& tm_now) const {
  int minutes_in_quarter = tm_now.tm_min % 15; // 0..14
  int seconds_in_quarter = minutes_in_quarter * 60 + tm_now.tm_sec;  // 0..14 * 60 == 0..840 + 0..59 = 0..899
  return seconds_in_quarter / INTERVAL_SECONDS;  // if INTERVAL_SECONDS 5 -> 0 .. 179
}

int FixedQuarterPowerHistoryAccumulator::GetMonthQuarterPeakPower() const {
  return month_quarter_peak_power;
}

int FixedQuarterPowerHistoryAccumulator::GetMonthQuarterPeakId() const {
  return month_quarter_peak_id;
}
	
WattHistorySamples& FixedQuarterPowerHistoryAccumulator::GetCurrentWattHistorySamples() {  
  return array_per_quarter_samples[storing_for_quarter.GetQuarterOfTheHour()];
}

const WattHistorySamples* FixedQuarterPowerHistoryAccumulator::GetCurrentWattHistorySamples() const {
  if (storing_for_interval_in_quarter_index <0) return nullptr; 
  return &array_per_quarter_samples[storing_for_quarter.GetQuarterOfTheHour()];
}


void FixedQuarterPowerHistoryAccumulator::ProcessNewSample(struct P1Data& data) {
  // Store month readings first
  month_quarter_peak_power = data.monthly_peak_watts;
  month_quarter_peak_id = QuarterIndicator(data.monthly_peak_timestamp).GetLocalQuarterId();

  // Determine if we are still storing values for the same quarter.
  QuarterIndicator current_quarter_indicator(data.message_timestamp);
  int current_quarter_in_the_hour = current_quarter_indicator.GetQuarterOfTheHour();
  int current_interval_in_quarter = GetIntervalInQuarter(data.message_timestamp); // 5 seconds interval, so in quarter ! 
  if (storing_for_quarter != current_quarter_indicator ) {
    // Maybe we need to save the last quarter ....
    if (!storing_for_quarter.IsUndefined()) {
      writeQuarterDataToFile(storing_for_quarter, GetCurrentWattHistorySamples());
    }
    // initialization clearly needed
    storing_for_quarter = current_quarter_indicator;
    ResetQuarterValues(current_quarter_in_the_hour);
    ResetAccumulationIntervalValues();
  }

  // Different interval, value already stored, just need to reset accumulation.
  if (storing_for_interval_in_quarter_index != current_interval_in_quarter) {
    ResetAccumulationIntervalValues();
    storing_for_interval_in_quarter_index = current_interval_in_quarter;
  }

  // Accumulate
  consumed_watt_sum_in_accumulation_interval += data.watts_consumed;
  consumed_watt_samples_in_accumulation_interval++;
  produced_watt_sum_in_accumulation_interval += data.watts_produced;
  produced_watt_samples_in_accumulation_interval++;
 
  int average_consumed_wattage_interval = consumed_watt_sum_in_accumulation_interval / consumed_watt_samples_in_accumulation_interval;
  int average_produced_wattage_interval = produced_watt_sum_in_accumulation_interval / produced_watt_samples_in_accumulation_interval;
  
  // Update the accumulated value so far
  GetCurrentWattHistorySamples().GetAtIndex(storing_for_interval_in_quarter_index) = WattHistorySample(average_consumed_wattage_interval, average_produced_wattage_interval);
  if (average_consumed_wattage_interval > watt_limits.peak_watts)  watt_limits.peak_watts = average_consumed_wattage_interval;
  if (average_produced_wattage_interval < watt_limits.min_watts)  watt_limits.min_watts = average_produced_wattage_interval;

}

WattLimits FixedQuarterPowerHistoryAccumulator::GetWattLimits() const {
  return watt_limits;
}

WattHistorySample FixedQuarterPowerHistoryAccumulator::GetCurrentWattsHistorySample() const {
  if (storing_for_interval_in_quarter_index < 0) return WattHistorySample();
  return GetCurrentWattHistorySamples()->GetAtIndex(storing_for_interval_in_quarter_index);
}

int FixedQuarterPowerHistoryAccumulator::GetAverageConsumedWatts() const {
  if (storing_for_interval_in_quarter_index <0) return 0; 
  return GetCurrentWattHistorySamples()->GetAverageConsumedWatts();
}

const WattHistorySample * FixedQuarterPowerHistoryAccumulator::GetCurrentSampleFromIndex(int index) const {
  if (storing_for_interval_in_quarter_index <0) return nullptr; 
  return &GetCurrentWattHistorySamples()->GetAtIndex(index);
}


