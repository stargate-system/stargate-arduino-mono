/*
    SerialGateDevice.cpp - Library to connect with Stargate server through serial adapter
    Created by Michał Marchel, December 2, 2023.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "SerialGateDevice.h"

SerialGateDevice::SerialGateDevice() : BaseDevice() {
    this->ready = false;
};

void SerialGateDevice::send(String* message) {
    if (Serial) {
        Serial.println(*message);
    }
}

void SerialGateDevice::start() {
    if (!this->deviceStarted) {
        this->deviceStarted = true;
        Serial.setTimeout(100);
        Serial.begin(115200);
    }
}

void SerialGateDevice::loop() {
    if (Serial) {
        this->outputBuffer.loop();
        if (Serial.available() > 0) {
            String message = Serial.readStringUntil('\n');
            this->onMessage(&message);
        }
    }
}

bool SerialGateDevice::isReady() {
    return this->ready;
}

void SerialGateDevice::onMessage(String* remainingMessage) {
    bool isAcknowledge = false;
    while (remainingMessage->length() > 0) {
        if (remainingMessage->charAt(0) == '*') {
            switch (remainingMessage->charAt(1)) {
                case '!':
                    switch (remainingMessage->charAt(2)) {
                        case 's':
                            handleSubscription(true, remainingMessage, this);
                            break;
                        case 'u':
                            handleSubscription(false, remainingMessage, this);
                            break;
                        case 'a':
                            handleIdAssigned(remainingMessage);
                            break;
                        case 'r':
                            this->outputBuffer.reset();
                            this->ready = true;
                            remainingMessage->remove(0, 9);
                            break;
                        case 'n':
                            this->ready = false;
                            this->factory.getValues()->unsubscribeAll();
                            remainingMessage->remove(0, 10);
                            break;
                        default:
                            remainingMessage->remove(0);
                    }
                    break;
                case '?':
                    if (remainingMessage->charAt(2) == 'm') {
                        handleManifestRequest(remainingMessage, this, "");
                    } else if (remainingMessage->charAt(2) == 't') {
                        handleTypeRequest(remainingMessage, this);
                    } else {
                        remainingMessage->remove(0);
                    }
                    break;
                case '>':
                    handleServerStorageGetResponse(remainingMessage, this);
                    break;
                case '+':
                    isAcknowledge = true;
                    remainingMessage->remove(0, 2);
                    break;
                case '.':
                    remainingMessage->remove(0, 2);
                    break;
                default:
                    remainingMessage->remove(0);
            }
        } else {
            handleValueMessage(remainingMessage, this->factory.getValues());
            remainingMessage->remove(0);
        }
    }
    if (isAcknowledge) {
        this->outputBuffer.acknowledgeReceived();
    } else {
        this->outputBuffer.sendAcknowledge();
    }
}