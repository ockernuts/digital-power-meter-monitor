#pragma once
#include <ArduinoJson.h>

/*
struct JsonFieldValidator {
    const char * field;
    const JsonFieldValidator** subFields;
    size_t numSubFields;
};

bool checkJsonChilds(const JsonObject& doc, const JsonFieldValidator& validatorInput, const char *parentFields, const char* topic) {
    for (size_t i = 0; i < validatorInput.numSubFields; i++) {
        const JsonFieldValidator * subFieldValidator = validatorInput.subFields[i]; 
        const char * subFieldName = subFieldValidator->field;
        const JsonObject& subField = doc[subFieldName];
        if (!subField) {
            Serial.printf("No '%s.%s' object in %s\n", parentFields, subFieldName);
            return false;
        }
        char newParent[80];
        snprintf(newParent, sizeof(newParent), "%s.%s", parentFields, subFieldName);
        if (!checkJsonChilds(subField, validatorInput, newParent, topic)) {
          return false; 
        }
    }
    return true;
}

bool validateJson(const JsonDocument& doc, const JsonFieldValidator* topFields[], int amountOfFields, const char *topic) {

  for (int i=0; i< amountOfFields; i++) {
    const JsonFieldValidator *top_field = topFields[i];
    const JsonObject &field_object = doc[top_field->field];
    if (!field_object) {
      Serial.printf("No '%s' object in %s\n", top_field->field, topic);
      return false; 
    }
    if (!checkJsonChilds(field_object, *top_field, top_field->field, topic)) {
      return false;
    }

  }
  return true; 
}

*/