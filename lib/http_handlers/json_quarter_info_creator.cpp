#include "json_quarter_info_creator.h"
#include "fixed_quarter_power_history_accumulator.h"

JsonQuarterInfoCreator& GetJsonQuarterInfoCreator() {
    static JsonQuarterInfoCreator jsonQuarterInfoCreator(GetFixedQuarterPowerHistoryAccumulator()); 
    return jsonQuarterInfoCreator;
}