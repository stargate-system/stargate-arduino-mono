#ifndef MessageHandler_h
#define MessageHandler_h

#include <Arduino.h>
#include <EEPROM.h>
#include "BaseDevice.h"
#include "OutputBuffer.h"
#include "values/GateValue.h"
#include "utils/GateValuesSet.h"

void handleManifestRequest(String* message, BaseDevice* device, String id);
void handleTypeRequest(String* message, BaseDevice* device);
void handleIdAssigned(String* message);
void handleServerStorageGetResponse(String* message, BaseDevice* device);
void handleValueMessage(String* message, GateValuesSet* valuesSet);
void setGateValue (int id, String* value, GateValuesSet* valuesSet);
String createManifest(BaseDevice* device, String id);
void handleSubscription(bool subscribed, String* message, BaseDevice* device);
void setValueSubscribed(int id, bool subscribed, BaseDevice* device);
String getIdFromEEPROM();

#endif