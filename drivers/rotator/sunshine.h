/*
    INDI Rotator Simulator
    Copyright (C) 2020 Jasem Mutlaq (mutlaqja@ikarustech.com)

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#include "indirotator.h"

class Sunshine : public INDI::Rotator
{
    public:

        Sunshine();
        virtual ~Sunshine() = default;
        virtual bool initProperties() override;
        virtual bool updateProperties() override;
        const char * getDefaultName() override;

    protected:
        // Rotator Overrides
        virtual IPState MoveRotator(double angle) override;
        virtual bool SyncRotator(double angle) override;
        virtual bool AbortRotator() override;

        // Misc.
        virtual void TimerHit() override;
        virtual bool ReverseRotator(bool enabled) override;
        

    private:
        bool Handshake() override;
        bool getFirmware();
        bool Ping();
        void hexDump(char * buf, const char * data, uint32_t size);
        bool sendCommand(const char * cmd, char * res);
        double m_TargetAngle {-1};
        // 10 degrees per second
        static const uint8_t ROTATION_RATE {10};
        /// Firmware
        ITextVectorProperty FirmwareTP;
        IText FirmwareT[1] {};
        static constexpr const uint8_t DRIVER_LEN {20};
        static constexpr const uint8_t DRIVER_STOP_CHAR {0x23};
        static constexpr const uint8_t DRIVER_TIMEOUT {3};
};
