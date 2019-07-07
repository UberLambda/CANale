// CANale/src/comm_op.cc - Implementation of CANale/src/comm_op.hh
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "comm_op.hh"

#include "comms.hh"
#include "util.hh"
#include "elf.hh"
#include "moc_comm_op.cpp"

namespace ca
{

/// Returns a hex-formatted device id.
inline QString devIdStr(CAdevId devId)
{
    return hexStr(devId, sizeof(CAdevId) * 2);
}


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
                 .arg(devIdStr(devId)).arg(m_devices.size()).arg(m_nDevices),
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
                 .arg(devIdStr(devId)).arg(m_devices.size()).arg(m_nDevices),
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
    QString devIdS = devIdStr(m_devId);

    if(m_elfData.isNull() || m_elfData.isEmpty())
    {
        progress(QStringLiteral("No ELF supplied for %1").arg(devIdS), -1);
        return;
    }

    static LogHandler nullLogger = {}; // Used if no `m_logger` present
    LogHandler &log = logger() ? *logger() : nullLogger;


    // [0..4%]: Load ELF
    progress(QStringLiteral("Loading ELF for %1").arg(devIdS), 0);

    MemIStream elfFileStream(reinterpret_cast<const uint8_t *>(m_elfData.data()),
                             static_cast<size_t>(m_elfData.size()));
    m_elf = std::make_unique<ELFIO::elfio>();
    if(!m_elf->load(elfFileStream))
    {
        progress(QStringLiteral("Failed to load ELF for %1").arg(devIdS), -1);
        return;
    }
    progress(QStringLiteral("ELF loaded for %1").arg(devIdS), 4);

    elfInfo(*m_elf, log);

    // [5..9%]: Send PROG_REQ and UNLOCK
    progress(QStringLiteral("Unlocking %1 to flash ELF").arg(devIdS), 5);

    connect(comms().get(), &Comms::progStarted, this, &FlashElfOp::onProgStarted);
    comms()->progStart(m_devId);

    // Wait for `onProgStarted()`
}

void FlashElfOp::onProgStarted(CAdevId devId, DeviceStats devStats)
{
    if(devId != m_devId)
    {
        // Not the device we want to flash
        return;
    }
    QString devIdS = devIdStr(m_devId);

    // Make sure we only start flashing once
    disconnect(comms().get(), &Comms::progStarted, this, &FlashElfOp::onProgStarted);

    // [9%]: PROG_REQ and UNLOCK done
    progress(QStringLiteral("%1 unlocked").arg(devIdS), 9);

    // [10..15%]: Check device stats, build flash map
    progress(QStringLiteral("Checking if %1 is compatibile with ELF").arg(devIdS), 10);
    if(devStats.elfMachine != m_elf->get_machine())
    {
        progress(QStringLiteral("%1 ELF machine mismatch").arg(devIdS), -2);
        log(CA_ERROR,
            QStringLiteral("%1 has machine type %2 but ELF e_machine is %3")
            .arg(devIdS).arg(devStats.elfMachine).arg(m_elf->get_machine()));
        return;
    }

    progress(QStringLiteral("Building ELF flash map for %1").arg(devIdS), 11);

    // FIXME IMPLEMENT: Build flash map

    // FIXME IMPLEMENT: Send page flash commands for pages in flash map
    connect(comms().get(), &Comms::pageFlashed, this, &FlashElfOp::onPageFlashed);
    connect(comms().get(), &Comms::pageFlashErrored, this, &FlashElfOp::onPageFlashErrored);
}

void FlashElfOp::onPageFlashed(CAdevId devId, uint32_t pageAddr)
{
    // FIXME IMPLEMENT: Send page flash commands for pages in flash map
}

void FlashElfOp::onPageFlashErrored(CAdevId devId, uint32_t pageAddr, uint16_t expectedCrc, uint16_t recvdCrc)
{
}

}
