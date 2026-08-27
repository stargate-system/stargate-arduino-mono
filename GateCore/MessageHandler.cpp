#include "MessageHandler.h"

void handleManifestRequest(String* message, BaseDevice* device, String id) {
    String* response = new String("*>manifest|");
    {
        String manifest = createManifest(device, id);
        response->concat(manifest.length());
        response->concat("|");
        response->concat(manifest);
    }
    device->outputBuffer.sendFunctionalMessage(response);
    delete response;
    message->remove(0, 12);
}

void handleTypeRequest(String* message, BaseDevice* device) {
    String response = "*>type|6|device";
    device->outputBuffer.sendFunctionalMessage(&response);
    message->remove(0, 8);
}

String createManifest(BaseDevice* device, String id) {
    String manifest("{");
    if (id.length() > 0) {
        manifest += "\"id\":\"" + id + "\",";
    } else {
        String storedId = getIdFromEEPROM();
        if (storedId.length() > 0) {
            manifest += "\"id\":\"" + storedId + "\",";
        }
    }
    manifest += "\"deviceName\":\"" + device->deviceName + "\"";
    if (device->groupName.length() > 0) {
        manifest += ",\"group\":\"" + device->groupName + "\"";
    }
    if (device->info.length() > 0) {
        manifest += ",\"info\":\"" + device->info + "\"";
    }
    manifest += ",\"values\":[";
    GateValuesSet* valuesSet = device->factory.getValues();
    if (valuesSet->size() > 0) {
        for (int i = 0; i < valuesSet->size(); i++) {
            manifest += valuesSet->get(i)->toManifest() + ",";
        }
        manifest.remove(manifest.length() - 1);
    }
    manifest += "]}";
    return manifest;
}

String getIdFromEEPROM() {
    #ifdef __AVR__
        EEPROM.begin();
    #else
        EEPROM.begin(40);
        delay(10);
    #endif
    String id("");
    bool success = true;
    if (EEPROM.read(0) == 0xFF) {
        byte currentByte;
        int currentIndex = 1;
        while(true) {
            currentByte = EEPROM.read(currentIndex);
            if (currentByte == 0) {
                break;
            } else {
                currentIndex++;
                if (isAscii((char) currentByte)) {
                    id += (char) currentByte;
                } else {
                    success = false;
                    break;
                }
            }
        }
    } else {
        EEPROM.end();
        return "";
    }
    if (success) {
        EEPROM.end();
        return id;
    } else {
        EEPROM.write(0, 0);
        EEPROM.end();
        return "";
    }
    
}

void handleIdAssigned(String* message) {
    message->remove(0, 13);
    int separatorIndex = message->indexOf('|');
    int idLength = message->substring(0, separatorIndex).toInt();
    message->remove(0, separatorIndex + 1);
    String id = message->substring(0, idLength);
    message->remove(0, idLength);

    int bufferSize = id.length() + 1;
    byte *buffer = new byte[bufferSize];
    id.getBytes(buffer, bufferSize);
    #ifdef __AVR__
        EEPROM.begin();
    #else
        EEPROM.begin(40);
        delay(10);
    #endif
    EEPROM.write(0, 0xFF);
    for(int i = 0; i < bufferSize; i++) {
        EEPROM.write(i + 1, buffer[i]);
    }
    EEPROM.end();
}

void handleServerStorageGetResponse(String* message, BaseDevice* device) {
    message->remove(0, 13);
    int separatorIndex = message->indexOf('|');
    int responseLength = message->substring(0, separatorIndex).toInt();
    message->remove(0, separatorIndex + 1);
    device->serverStorage.handleGetResponse(message->substring(0, responseLength));
    message->remove(0, responseLength);
}

void setValueSubscribed(int id, bool subscribed, BaseDevice* device) {
    GateValuesSet* valuesSet = device->factory.getValues();
    int valueIndex = valuesSet->find(id);
    if (valueIndex > -1) {
        valuesSet->get(valueIndex)->subscribed = subscribed;
        if (subscribed) {
            device->outputBuffer.sendValue(valuesSet->get(valueIndex));
        }
    }
}

void handleSubscription(bool subscribed, String* message, BaseDevice* device) {
    int separatorIndex = message->indexOf('|');
    if (separatorIndex != -1) {
        message->remove(0, separatorIndex + 1);
        separatorIndex = message->indexOf('|');
        if (separatorIndex != -1) {
            String lengths = message->substring(0, separatorIndex);
            message->remove(0, separatorIndex + 1);

            int lengthsFirstSeparator = -1;
            int lengthsSecondSeparator = -1;
            do {
                lengthsSecondSeparator = lengths.indexOf(',', lengthsFirstSeparator + 1);
                int length = -1;
                if (lengthsSecondSeparator == -1) {
                    length = lengths.substring(lengthsFirstSeparator + 1).toInt();
                } else {
                    length = lengths.substring(lengthsFirstSeparator + 1, lengthsSecondSeparator).toInt();
                }
                lengthsFirstSeparator = lengthsSecondSeparator;

                if (length > message->length()) {
                    message->remove(0);
                    break;
                }
                int id = message->substring(0, length).toInt();
                message->remove(0, length);
                setValueSubscribed(id, subscribed, device);
            } while (lengthsFirstSeparator > -1);

            return;
        }
    }
    message->remove(0);
}

void setGateValue (int id, String* value, GateValuesSet* valuesSet) {
    int valueIndex = valuesSet->find(id);
    if (valueIndex > -1) {
        if (valuesSet->get(valueIndex)->direction == 1) {
            valuesSet->get(valueIndex)->fromRemote(*value);
        }
    }
}

void handleValueMessage(String* message, GateValuesSet* valuesSet) {
    message->trim();
    if (message->length() > 0) {
        int firstSeparatorIndex = message->indexOf('|');
        if (firstSeparatorIndex != -1) {
            int secondSeparatorIndex = message->indexOf('|', firstSeparatorIndex + 1);
            if (secondSeparatorIndex != -1) {
                String ids = message->substring(0, firstSeparatorIndex);
                String lengths = message->substring(firstSeparatorIndex + 1, secondSeparatorIndex);
                String values = message->substring(secondSeparatorIndex + 1);

                firstSeparatorIndex = -1;
                secondSeparatorIndex = -1;
                int lengthsFirstSeparator = -1;
                int lengthsSecondSeparator = -1;
                int lengthProcessed = 0;
                do {
                    secondSeparatorIndex = ids.indexOf(',', firstSeparatorIndex + 1);
                    int id = -1;
                    if (secondSeparatorIndex == -1) {
                        id = ids.substring(firstSeparatorIndex + 1).toInt();
                    } else {
                        id = ids.substring(firstSeparatorIndex + 1, secondSeparatorIndex).toInt();
                    }
                    firstSeparatorIndex = secondSeparatorIndex;

                    lengthsSecondSeparator = lengths.indexOf(',', lengthsFirstSeparator + 1);
                    int length = -1;
                    if (lengthsSecondSeparator == -1) {
                        length = lengths.substring(lengthsFirstSeparator + 1).toInt();
                    } else {
                        length = lengths.substring(lengthsFirstSeparator + 1, lengthsSecondSeparator).toInt();
                    }
                    lengthsFirstSeparator = lengthsSecondSeparator;

                    int currentLength = lengthProcessed + length;
                    if (currentLength > values.length()) {
                        break;
                    }
                    String value = values.substring(lengthProcessed, currentLength);
                    setGateValue(id, &value, valuesSet);
                    lengthProcessed = currentLength;
                } while (firstSeparatorIndex > -1 || lengthsFirstSeparator > -1);
            }
        }
    }
}
