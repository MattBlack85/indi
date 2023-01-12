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

#include "sunshine.h"
#include "indicom.h"

#include <cmath>
#include <memory>
#include <regex>
#include <termios.h>
#include <cstring>
#include <sys/ioctl.h>
#include <chrono>
#include <math.h>
#include <iomanip>

std::unique_ptr<Sunshine> rotatorSimulator(new Sunshine());

Sunshine::Sunshine()
{
    setVersion(1, 0);
}

bool Sunshine::initProperties()
{
    INDI::Rotator::initProperties();

    SetCapability(ROTATOR_CAN_ABORT |
                  ROTATOR_CAN_REVERSE |
                  ROTATOR_CAN_SYNC);

    addAuxControls();

    // Firmware
    IUFillText(&FirmwareT[0], "VERSION", "Version", "NA");
    IUFillTextVector(&FirmwareTP, FirmwareT, 1, getDefaultName(), "FIRMWARE_INFO", "Firmware", MAIN_CONTROL_TAB, IP_RO, 60,
                     IPS_IDLE);

    return true;
}

bool Sunshine::updateProperties()
{
    INDI::Rotator::updateProperties();

    if (isConnected())
    {
        // Main Control
        getFirmware();
        defineProperty(&FirmwareTP);
    }
    else
    {
        // Main Control
        deleteProperty(FirmwareTP.name);
    }

    return true;
}

const char * Sunshine::getDefaultName()
{
    return "Sunshine";
}

bool Sunshine::Handshake()
{
    return Ping();
}

IPState Sunshine::MoveRotator(double angle)
{
    if (ReverseRotatorS[INDI_ENABLED].s == ISS_ON)
        m_TargetAngle = range360(360 - angle);
    else
        m_TargetAngle = range360(angle);
    return IPS_BUSY;
}

bool Sunshine::SyncRotator(double angle)
{
    INDI_UNUSED(angle);
    return true;
}

bool Sunshine::AbortRotator()
{
    return true;
}

bool Sunshine::ReverseRotator(bool enabled)
{
    INDI_UNUSED(enabled);
    return true;
}

void Sunshine::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(getCurrentPollingPeriod());
        return;
    }

    if (GotoRotatorNP.s == IPS_BUSY)
    {
        if (std::fabs(m_TargetAngle - GotoRotatorN[0].value) <= ROTATION_RATE)
        {
            GotoRotatorN[0].value = m_TargetAngle;
            GotoRotatorNP.s = IPS_OK;
        }
        else
        {
            // Find shortest distance given target degree
            double a = m_TargetAngle;
            double b = GotoRotatorN[0].value;
            int sign = (a - b >= 0 && a - b <= 180) || (a - b <= -180 && a - b >= -360) ? 1 : -1;
            double diff = ROTATION_RATE * sign;
            GotoRotatorN[0].value += diff;
            GotoRotatorN[0].value = range360(GotoRotatorN[0].value);
        }

        IDSetNumber(&GotoRotatorNP, nullptr);
    }

    SetTimer(getCurrentPollingPeriod());
}

bool Sunshine::Ping()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand(":P#", res))
    {
        return true;
    }

    return false;
}

bool Sunshine::getFirmware()
{
  char res[DRIVER_LEN] = {0};
  if (sendCommand(":V#", res))
  {
    char fmt_fw[6] = {'\0', '.', '\0', '.', '\0', '\0'};
    fmt_fw[0] = res[2];
    fmt_fw[2] = res[3];
    fmt_fw[4] = res[4];
    LOGF_INFO("FW_VERSION <%s>", fmt_fw);
    IUSaveText(&FirmwareT[0], fmt_fw);
    return true;
  }

  return false;
}

bool Sunshine::sendCommand(const char * cmd, char * res)
{
    int nbytes_written = 0, nbytes_read = 0, tty_rc = 0;

    LOGF_INFO("CMD <%s>", cmd);
    
    tcflush(PortFD, TCIOFLUSH);
    
    if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(tty_rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial write error: %s.", errstr);
        return false;
    }

    tty_rc = tty_nread_section(PortFD, res, DRIVER_LEN, DRIVER_STOP_CHAR, DRIVER_TIMEOUT, &nbytes_read);

    if (tty_rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(tty_rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial read error: %s.", errstr);
        return false;
    }

    res[nbytes_read - 1] = '\0';
    LOGF_INFO("RES <%s>", res);

    return true;
}
