/*
    ESP8266GateDevice.cpp - Library to connect ESP8266 module with Stargate server
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

#include "GateDevice.h"

GateDevice::GateDevice() : BaseDevice() {
    this->connectionState = 0;
    this->pingInterval = 7000;
    this->pingTimer = 0;
    this->discoveryKeyword = "GateServer";
    this->discoveryPort = 10001;
    this->networkStopped = true;
    this->usePreviousAddress = false;
    this->lastServerAddress = "";
    this->lastServerPort = -1;
    this->id = "";
    this->password = "";
    this->connectionKey = "";
};

void GateDevice::start() {
    if (!this->deviceStarted) {
        this->deviceStarted = true;
        this->onDeviceStart();
        this->connectServer();
        this->networkStopped = false;
    }
};

void GateDevice::stop(bool stopNetwork) {
    if (this->deviceStarted) {
        this->usePreviousAddress = true;
        this->deviceStarted = false;
        this->networkStopped = stopNetwork;
        this->stopSocket();
    }
}

void GateDevice::stop() {
    this->stop(true);
}

void GateDevice::loop() {
    if (this->networkAvailable()) {
        if (this->connectionState == 4) {
            this->loopSocket();
            this->handlePing();
            this->outputBuffer.loop();
        } else {
            this->connectServer();
        }
    } else {
        if (this->connectionState == 4) {
            this->factory.getValues()->unsubscribeAll();
        }
        this->connectionState = 0;
    }
}

void GateDevice::setRemote(String id, String password) {

}

void GateDevice::connectServer() {
    if (this->id.length() > 0) {
        this->connectRemoteServer();
    } else {
        this->connectLocalServer();
    }
}

/*
    connectionState:
    0 - no connection
    1 - discovery
    2 - socket opening
    3 - socket opened
    4 - connection ready
*/

void GateDevice::connectLocalServer() {
    switch (this->connectionState) {
        case 0:
        {
            this->stopSocket();
            if (this->usePreviousAddress && this->lastServerAddress.length() > 0 && this->lastServerPort != -1) {
                this->startSocket(lastServerAddress, lastServerPort);
                this->pingTimer = millis() + this->pingInterval;
                this->connectionState = 2;
            } else {
                bool success = this->startUdp(this->discoveryPort);
                if (success) {
                    this->connectionState = 1;
                }
            }
            this->usePreviousAddress = false;
            break;
        }
        case 1:
        {
            String discoveryMessage = this->getUdpMessage();
            if (discoveryMessage.length() > this->discoveryKeyword.length()) {
                int separatorIndex = discoveryMessage.indexOf(':');
                if (separatorIndex > -1) {
                    String keyword = discoveryMessage.substring(0, separatorIndex);
                    if (keyword.equals(discoveryKeyword)) {
                        String serverIp = this->getServerIp();
                        this->stopUdp();
                        int connectionPort = discoveryMessage.substring(separatorIndex + 1).toInt();
                        this->lastServerAddress = serverIp;
                        this->lastServerPort = connectionPort;
                        this->startSocket(serverIp, connectionPort);
                        this->pingTimer = millis() + this->pingInterval;
                        this->connectionState = 2;
                    }
                }
            }
            break;
        }
        case 2:
        case 3:
        {
            if (this->pingTimer < millis()) {
                this->stopSocket();
                this->connectionState = 0;
            } else {
                this->loopSocket();
            }
            break;
        }
    }
}

void GateDevice::connectRemoteServer() {
    switch (this->connectionState) {
        case 0:
        {
            this->stopSocket();
            if (this->usePreviousAddress && this->lastServerAddress.length() > 0 && this->lastServerPort != -1) {

            } else {
                this->connectionState = 1;
            }
            break;
        }
        case 1:
        {
            
            break;
        }
    }
}

void GateDevice::handlePing() {
    unsigned long now = millis();
    if (this->pingTimer < now) {
        this->stopSocket();
        this->connectionState = 0;
    }
}

void GateDevice::socketOpened() {
    this->connectionState = 3;
}

void GateDevice::socketClosed() {
    this->connectionState = 0;
    this->factory.getValues()->unsubscribeAll();
    this->loopSocket();
    if (this->networkStopped) {
        delay(100);
        this->stopNetwork();
    }
}

void GateDevice::onMessage(char* message) {
    this->pingTimer = millis() + this->pingInterval;
    String* remainingMessage = new String(message);
    bool isAcknowledge = false;
    if (this->connectionState == 3) {
        isAcknowledge = this->handleHandshakeMessage(remainingMessage);
    } else {
        isAcknowledge = this->handleReadyStateMessage(remainingMessage);
    }
    if (isAcknowledge) {
        this->outputBuffer.acknowledgeReceived();
    } else {
        this->outputBuffer.sendAcknowledge();
    }
    delete remainingMessage;
}

bool GateDevice::handleHandshakeMessage(String* remainingMessage) {
    bool isAcknowledge = false;
    while (remainingMessage->length() > 0) {
        if (remainingMessage->charAt(0) == '*') {
            switch (remainingMessage->charAt(1)) {
                case '?':
                    if (remainingMessage->charAt(2) == 'm') {
                        handleManifestRequest(remainingMessage, this);
                    } else if (remainingMessage->charAt(2) == 't') {
                        handleTypeRequest(remainingMessage, this);
                    } else {
                        remainingMessage->remove(0);
                    }
                    break;
                case '!':
                    if (remainingMessage->charAt(2) == 'a') {
                        handleIdAssigned(remainingMessage);
                    } else if (remainingMessage->charAt(2) == 'r') {
                        this->outputBuffer.reset();
                        this->connectionState = 4;
                        remainingMessage->remove(0, 9);
                    } else {
                        remainingMessage->remove(0);
                    }
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
            remainingMessage->remove(0);
        }
    }
    return isAcknowledge;
}

bool GateDevice::handleReadyStateMessage(String* remainingMessage) {
    bool isAcknowledge = false;
    while (remainingMessage->length() > 0) {
        if (remainingMessage->charAt(0) == '*') {
            switch (remainingMessage->charAt(1)) {
                case '>':
                    handleServerStorageGetResponse(remainingMessage, this);
                    break;
                case '!':
                    if(remainingMessage->charAt(2) == 's') {
                        handleSubscription(true, remainingMessage, this);
                    } else if (remainingMessage->charAt(2) == 'u') {
                        handleSubscription(false, remainingMessage, this);
                    } else {
                        remainingMessage->remove(0);
                    }
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
    return isAcknowledge;
}

bool GateDevice::isReady() {
    return this->connectionState == 4;
}
