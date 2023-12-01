/*
 * FixedQuarterPowerHistoryAccumulator.h
 *
 *  Created on: 15 feb. 2023
 *      Author: Wim Ockerman
 */

#ifndef FIXEDQUARTERPOWERHISTORYACCUMULATOR_H_
#define FIXEDQUARTERPOWERHISTORYACCUMULATOR_H_
#include <stdint.h>
#include <time.h>
#include <Arduino.h>


typedef void (*Func_average_peak_warning)(unsigned short peakwatts);

struct WattLimits {
  int peak_watts;
  int min_watts;
};

// Compact storage of Quarter identifier using only 32 bits....
class QuarterIndicator {
private: 
  // All this storage is in local time. 
  uint8_t year; // like 23, starting from 2000 mentally...
  uint8_t month; // 1..12
  uint8_t day;   // 1..31, sometimes max 30 or 29
  // This is not just the quarter in the day !!! The 7th bit is the Daylight savings time aka summer time indicator !
  uint8_t quarter_in_day;  // 0..24*4-1  -> 0..95  + 7th bit ...
 public:  
  // GMT+1 based time, unless summer_time is set, then GMT+2 !!!
  QuarterIndicator(int year, int month, int day, int hour, int quarter, bool summer_time) :
    year(max(year - 2000, 0)), month(month), day(day), quarter_in_day(quarter + (hour * 4) + (summer_time ? 0x80 : 0) ) {
  }

  // Take in a local time. 
  QuarterIndicator(const tm& time) : 
    QuarterIndicator(time.tm_year + 1900, // 
	                 time.tm_mon +1, // tm_mon is zero based ! 
                     time.tm_mday,
					 time.tm_hour,
					 time.tm_min / 15,
					 time.tm_isdst == 1) {			
  }

  QuarterIndicator(int local_quarter_id ) : 
     year( local_quarter_id >> 24),
	 month( (local_quarter_id >> 16) & 0xFF ),
	 day( (local_quarter_id >> 8) & 0xFF),
	 quarter_in_day(local_quarter_id & 0xFF) {
  }

  QuarterIndicator() : QuarterIndicator(0) {}

  bool operator ==(const QuarterIndicator &rhs ) const {
    return year == rhs.year && month==rhs.month && day==rhs.day && quarter_in_day==rhs.quarter_in_day;
  }

  bool operator !=(const QuarterIndicator &rhs ) const {
    return !(*this == rhs);
  }

  bool IsUndefined() {
	return (GetLocalQuarterId() == 0);
  }

  int GetYear() const { return year + 2000;}
  int GetMonth() const { return month; }
  int GetDay() const { return day;}
  int GetQuarterOfTheHour() const { return quarter_in_day % 4; }
  // Get quarter in the day, without the summer / winter time different !!!! 
  // int GetQuarterOfTheDay() const { return quarter_in_day & 0x7F; }
  int GetHour() const { return (quarter_in_day & 0x7F) >> 2; }
  bool IsSummerTime() const { return quarter_in_day & 0x80 ? true : false; }
  // While this is a local quarter ID, the time with same winter and summer equivalent differ here in the 7th bit. 
  // So each local quarter is unique :-)
  int GetLocalQuarterId() const { return ((((( year <<8)  | month) <<8) | day) << 8) | quarter_in_day; }
};


// 4 bytes per WattHistorySample ! x 180 for a quarter = 720 bytes per quarter
// A watthistorysample contains average watt values for an interval of 5 seconds 
class WattHistorySample {
private:

    // In a house with 40A and 380V, the max wattage is 15200 Watt, so a signed 16-bitter is enough. 
	// This compactness is needed because we will need to store a value for every 5 seconds of a quarter. (so 180 samples)
    int16_t consumed_power_average; // a value of -1 here means there is no value ! 
	int16_t produced_power_average; 
public:
	WattHistorySample() : consumed_power_average(-1), produced_power_average(0) {}

	WattHistorySample(int16_t consumed_power_average, int16_t produced_power_average):
	  consumed_power_average(max(int16_t(0),consumed_power_average)), produced_power_average(min(produced_power_average,int16_t(0))) {
	}

	bool HasValue() const {
		return consumed_power_average != -1;
	}

	int16_t GetConsumedPowerAverage() const {
		if (!HasValue()) {
			// Should not happen, should be checked by caller; However blob querier uses this
			return -1;
		}
		if (consumed_power_average < 0) {
			// What ???? Who recorded this ??? (should not happen, is an internal mistake in the program)
			assert(false);
			return 0;
		}
		return consumed_power_average;
	}

	/// @brief returns the 0 or negative produced power 
	/// @return the produced power (negative or zero)
	int16_t GetProducedPowerAverage() const {
		if (!HasValue()) {
			// Should not happen, should be checked by caller; However blob querier uses this
			return 0; 
		}

		if (produced_power_average>0) {
			// What ??? Who recorded this ??? (should not happen, is an internal mistake in the program)
			assert(false);
			return 0; 
		}
		return produced_power_average; 
	}

};

#define WATT_HISTORY_SAMPLES 180

class WattHistorySamples {
	WattHistorySample array_watt_history_samples[WATT_HISTORY_SAMPLES];

public: 
	WattHistorySamples() {
		for (int i = 0; i< WATT_HISTORY_SAMPLES; i++) {
			array_watt_history_samples[i] = WattHistorySample();
		}
	}

	// returns highest index where power values are actually stored. 
	// returns -1 if no index is found where values are stored. 
	int GetHighestWattHistorySampleIndexWithValues() const {		
		for (int i= WATT_HISTORY_SAMPLES-1; i >=0; i--) {
			if (array_watt_history_samples[i].HasValue()) {
				return i;
			}
		}
		return -1; 
	}

	WattHistorySample& GetAtIndex(int index) {
		return array_watt_history_samples[index % WATT_HISTORY_SAMPLES];
	}

	const WattHistorySample& GetAtIndex(int index) const {
		return array_watt_history_samples[index % WATT_HISTORY_SAMPLES];
	}

	int GetAverageConsumedWatts() const {
		int valid_values = 0;
        int watt_sum = 0; 
		for (int i = 0; i< WATT_HISTORY_SAMPLES; i++) {
			const WattHistorySample& sample = array_watt_history_samples[i];
			if (!sample.HasValue()) continue;
			watt_sum += sample.GetConsumedPowerAverage();
			valid_values++;
		}
		if (valid_values == 0) return 0; 
		return watt_sum / valid_values;
	}

    int GetAverageProducedWatts() const {
		int valid_values = 0;
        int watt_sum = 0; 
		for (int i = 0; i< WATT_HISTORY_SAMPLES; i++) {
			const WattHistorySample& sample = array_watt_history_samples[i];
			if (!sample.HasValue()) continue;
			watt_sum += sample.GetProducedPowerAverage();
			valid_values++;
		}
		if (valid_values == 0) return 0; 
		return watt_sum / valid_values;
	}

};

// Minimally 4, otherwise quarters in hour can't be used anymore as index
#define QUARTERS_TO_KEEP_IN_RAM 4 
#define INTERVAL_SECONDS 5

class FixedQuarterPowerHistoryAccumulator {
private:
   int month_quarter_peak_power;
   int month_quarter_peak_id;

    
    // To keep a day of WattHistorySamples for each quarter, we would need 24x4x720 bytes = 69 120 bytes
	// ... which is a bit too much ...
    // So lets only keep the last 1 hours = 4x720 bytes in memory  + some quarter overhead... hope that works. 
    // The rest of the history can be written to SD card or LittleFS flash... 
	WattHistorySamples array_per_quarter_samples[QUARTERS_TO_KEEP_IN_RAM];
	// QuarterIndicator quarters[QUARTERS_TO_KEEP_IN_RAM];
	
	// Indicating "current" quarter
	QuarterIndicator storing_for_quarter; // storing_for_quarter.GetQuarterOfTheHour() -> index in WattHistorySample
	// an interval  is 5 seconds. This indicates in which interval in the quarter we are.
	// a quarter is 15 * 60 seconds = 900. So with an interval of 5 we have 900/5 intervals = 180
	// in this case the range would thus be 0..179, which is what can be used as first index in the array_watt_history_samples.
	int storing_for_interval_in_quarter_index;

	
    /////// SUB INTERVAL ACCUMULATION. 

	// Members for separate accumulation of watt samples in the same interval (so e.g. in 5 seconds -> 1 horizontal pixel).
	int consumed_watt_samples_in_accumulation_interval;
	int consumed_watt_sum_in_accumulation_interval;

	// Produced watts are stored as negative numbers !!!
	int produced_watt_samples_in_accumulation_interval;
	int produced_watt_sum_in_accumulation_interval;

	// Min and Max values updated along the way. 
	WattLimits watt_limits;

	Func_average_peak_warning average_peak_warning;
protected: 
     WattHistorySamples& GetCurrentWattHistorySamples();
	 
public:
	FixedQuarterPowerHistoryAccumulator();

	void setCallBackOnAveragePeakWarning(Func_average_peak_warning average_peak_warning) {
	  this->average_peak_warning = average_peak_warning;
	}

	virtual ~FixedQuarterPowerHistoryAccumulator();

	int GetMonthQuarterPeakPower() const;
	int GetMonthQuarterPeakId() const;
	
    // void InitForTest();

	void ResetAccumulationIntervalValues();

	void ResetQuarterValues(int quarter_of_the_hour);

	int GetIntervalInQuarter(const tm& tm_now) const;

	void ProcessNewSample(struct P1Data& data);

	int GetWattHistorySampleWidth() const { return WATT_HISTORY_SAMPLES; }

	WattLimits GetWattLimits() const;

	int GetAverageConsumedWatts() const;

	// Can be nullptr ! 
	const WattHistorySample * GetCurrentSampleFromIndex(int index) const;

	int GetStoringQuarterId() const { return storing_for_quarter.GetLocalQuarterId(); }

	const QuarterIndicator& GetStoringQuarter() const { return storing_for_quarter; }

	int GetStoringInterval() const { return storing_for_interval_in_quarter_index; }

	WattHistorySample GetCurrentWattsHistorySample() const;

	const WattHistorySamples* GetCurrentWattHistorySamples() const;
	

};

extern FixedQuarterPowerHistoryAccumulator& GetFixedQuarterPowerHistoryAccumulator();

#endif /* FIXEDQUARTERPOWERHISTORYACCUMULATOR_H_ */
