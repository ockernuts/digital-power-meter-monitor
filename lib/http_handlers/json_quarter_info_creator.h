/*
 * JsonQuarterInfoCreator.h
 *
 *  Created on: 19 feb. 2023
 *  Author: Wim
 */

#ifndef JSONQUARTERINFOCREATOR_H_
#define JSONQUARTERINFOCREATOR_H_

#include "fixed_quarter_power_history_accumulator.h"
#include <ArduinoJson.h>
#include <Print.h>


#define QUARTER_SAMPLES_JSON_SIZE 6300


class JsonQuarterInfoCreator {
private: 
    const FixedQuarterPowerHistoryAccumulator& watt_history;

public: 
    JsonQuarterInfoCreator(const FixedQuarterPowerHistoryAccumulator&  watt_history): watt_history(watt_history) {}

    void FillJsonObjectForLastQuarterQueryResult(JsonObject & root ) {        
        const QuarterIndicator & storing_quarter = watt_history.GetStoringQuarter();
        int quarter_id = storing_quarter.GetLocalQuarterId();
        if (quarter_id == 0) {
            return;
        }

        const WattHistorySamples* samples= watt_history.GetCurrentWattHistorySamples();
        if (samples == nullptr) return;

        FillJsonObjectForQuarterWattHistorySamples(root, storing_quarter, *samples);
    }
    


    void FillJsonObjectForQuarterWattHistorySamples(JsonObject& quarterJson, const QuarterIndicator& quarterId, const WattHistorySamples& samples) {
        quarterJson["quarter_id"] = quarterId.GetLocalQuarterId();
        quarterJson["consumed_power_avg"] = samples.GetAverageConsumedWatts();
        quarterJson["begin_consumed_wh"] = samples.GetBeginConsumedWattHours();
        quarterJson["begin_produced_wh"] = samples.GetBeginProducedWattHours();
        quarterJson["interval_secs"] = 5;
    
        JsonArray quarter_consumed_power_interval_avgs = quarterJson.createNestedArray("consumed_power_interval_avgs");
        JsonArray quarter_produced_power_interval_avgs = quarterJson.createNestedArray("produced_power_interval_avgs");

        int end_interval_id = samples.GetHighestWattHistorySampleIndexWithValues();
        if (end_interval_id == -1) return;

        for (int interval_id=0; interval_id<= end_interval_id; interval_id++) {
            const WattHistorySample& sample = samples.GetAtIndex(interval_id);            
            if (sample.HasValue()) {
                quarter_consumed_power_interval_avgs.add(sample.GetConsumedPowerAverage());
                quarter_produced_power_interval_avgs.add(sample.GetProducedPowerAverage());
            } else {
                quarter_consumed_power_interval_avgs.add();
                quarter_produced_power_interval_avgs.add();
            }
        }
    }
};

extern JsonQuarterInfoCreator& GetJsonQuarterInfoCreator();

#endif