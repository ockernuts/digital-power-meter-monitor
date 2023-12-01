#include "json_quarter_querier.h"

JsonQuarterQuerier& GetJsonQuarterQuerier(AsyncWebServer& server) {
    static JsonQuarterQuerier jsonQuarterQuerier(server, GetJsonQuarterInfoCreator());
    return jsonQuarterQuerier;
}