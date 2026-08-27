/*
    ESP8266GateDevice.h - Library to connect ESP8266 module with Stargate server
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

#ifndef GateDevice_h
#define GateDevice_h

#include <Arduino.h>
#include "../GateCore/BaseDevice.h"
#include "../GateCore/OutputBuffer.h"
#include "../GateCore/MessageHandler.h"
#include "../GateCore/values/ValueFactory.h"

class GateDevice : public BaseDevice
{
    public:
        GateDevice();
        void start() override;
        void stop();
        void stop(bool stopNetwork);
        void loop() override;
        bool isReady() override;
        int connectionState;
        String discoveryKeyword;
        int discoveryPort;
        void setRemote(String id, String password);

    protected:
        virtual bool startUdp(int port) = 0;
        virtual void stopUdp() = 0;
        virtual String getUdpMessage() = 0;
        virtual String getServerIp() = 0;
        virtual void onDeviceStart() = 0;
        virtual void stopNetwork() = 0;
        virtual bool networkAvailable() = 0;
        virtual void startSocket(String serverIp, int port) = 0;
        virtual void stopSocket() = 0;
        virtual void loopSocket() = 0;

        void socketOpened();
        void socketClosed();
        void onMessage(char* message);
        bool networkStopped;

    private:
        void connectServer();
        void connectLocalServer();
        void connectRemoteServer();
        void handlePing();
        int pingInterval;
        unsigned long pingTimer;
        bool usePreviousAddress;
        String lastServerAddress;
        int lastServerPort;
        bool handleHandshakeMessage(String* remainingMessage);
        bool handleReadyStateMessage(String* remainingMessage);
        String id;
        String password;
        String connectionKey;
};

#endif