#include "month_peak_querier.h"

MonthPeakQuerier& GetMonthPeakQuerier(AsyncWebServer &server, const char *user, const char*pwd) {
    static MonthPeakQuerier monthPeakQuerier(server, GetFixedQuarterPowerHistoryAccumulator(), user, pwd);
    return monthPeakQuerier;
}