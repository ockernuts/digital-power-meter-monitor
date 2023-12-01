#include "month_peak_querier.h"

MonthPeakQuerier& GetMonthPeakQuerier(AsyncWebServer &server) {
    static MonthPeakQuerier monthPeakQuerier(server, GetFixedQuarterPowerHistoryAccumulator());
    return monthPeakQuerier;
}