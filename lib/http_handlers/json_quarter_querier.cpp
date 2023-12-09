#include "json_quarter_querier.h"

JsonQuarterQuerier& GetJsonQuarterQuerier(AsyncWebServer& server, const char *user, const char *pwd) {
    static JsonQuarterQuerier jsonQuarterQuerier(server, GetJsonQuarterInfoCreator(), user, pwd);
    return jsonQuarterQuerier;
}