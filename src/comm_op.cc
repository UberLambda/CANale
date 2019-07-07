// CANale/src/comm_op.cc - Implementation of CANale/src/comm_op.hh
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "comm_op.hh"

#include "util.hh"
#include "comms.hh"

namespace ca
{

Operation::Operation(ProgressHandler onProgress, QObject *parent)
    : QObject(parent), m_onProgress(std::move(onProgress))
{
}

Operation::~Operation()
{
}

void Operation::start(QSharedPointer<Comms> comms, LogHandler *logger)
{
    m_comms = comms;
    m_logger = logger;
    started();
}

StartDevicesOp::StartDevicesOp(ProgressHandler onProgress,
                               QSet<CAdevId> devices, QObject *parent)
    : Operation(onProgress, parent),
      m_devices(devices), m_nDevices(devices.size())
{
}

void StartDevicesOp::started()
{
    if(m_nDevices == 0)
    {
        progress(QStringLiteral("No devices to unlock"), 100);
        return;
    }

    // Listen for when a device gets started
    connect(comms().get(), &ca::Comms::progStarted, this, &StartDevicesOp::started);

    // Send start command to all devices
    for(CAdevId devId : m_devices)
    {
        comms()->progStart(devId);
    }
}

void StartDevicesOp::onProgStarted(CAdevId devId)
{
    if(!m_devices.remove(devId))
    {
        // Not one of our devices
        return;
    }

    if(!m_devices.empty())
    {
        int progr = static_cast<int>(0.01f * (m_nDevices - m_devices.size()) / m_nDevices);
        progress(QStringLiteral("Unlocked device %1 (%1 of %2)")
                 .arg(hexStr(devId, sizeof(CAdevId) * 2)).arg(m_devices.size()).arg(m_nDevices),
                 progr);
    }
    else
    {
        // Done!
        progress(QStringLiteral("Unlocked %1 devices").arg(m_devices.size()).arg(m_nDevices),
                 100);
    }
}


StopDevicesOp::StopDevicesOp(ProgressHandler onProgress,
                             QSet<CAdevId> devices, QObject *parent)
    : Operation(onProgress, parent),
      m_devices(devices), m_nDevices(devices.size())
{
}

void StopDevicesOp::started()
{
    if(m_nDevices == 0)
    {
        progress(QStringLiteral("No devices to lock"), 100);
        return;
    }

    // Listen for when a device gets stopped
    connect(comms().get(), &ca::Comms::progEnd, this, &StopDevicesOp::onProgEnd);

    // Send stop command to all devices
    for(CAdevId devId : m_devices)
    {
        comms()->progEnd(devId);
    }
}

void StopDevicesOp::onProgEnd(CAdevId devId)
{
    if(!m_devices.remove(devId))
    {
        // Not one of our devices
        return;
    }

    if(!m_devices.empty())
    {
        int progr = static_cast<int>(0.01f * (m_nDevices - m_devices.size()) / m_nDevices);
        progress(QStringLiteral("Locked device %1 (%1 of %2)")
                 .arg(hexStr(devId, sizeof(CAdevId) * 2)).arg(m_devices.size()).arg(m_nDevices),
                 progr);
    }
    else
    {
        // Done!
        progress(QStringLiteral("Locked %1 devices").arg(m_devices.size()).arg(m_nDevices),
                 100);
    }
}


FlashElfOp::FlashElfOp(ProgressHandler onProgress,
                       CAdevId devId, QByteArray elfData, QObject *parent)
    : Operation(onProgress, parent),
      m_devId(devId), m_elfData(elfData)
{
}

void FlashElfOp::started()
{
    // FIXME IMPLEMENT!
    progress(QStringLiteral("TODO"), -1);
}

}
