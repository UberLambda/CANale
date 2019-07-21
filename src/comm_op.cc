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
    : QObject(parent), m_onProgress(std::move(onProgress)), m_started(false)
{
}

Operation::~Operation()
{
}

void Operation::start(QSharedPointer<Comms> comms, LogHandler *logger)
{
    m_comms = comms;
    m_logger = logger;
    m_started = true;
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
    connect(comms().get(), &ca::Comms::progStarted, this, &StartDevicesOp::onProgStarted);

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

    int progr = std::min(static_cast<int>(100.0f * (m_nDevices - m_devices.size()) / m_nDevices), 99);
    progress(QStringLiteral("Unlocked device %1 (%2 of %3)")
             .arg(devIdStr(devId)).arg(m_devices.size() + 1).arg(m_nDevices),
             progr);

    if(m_devices.empty())
    {
        // IMPORTANT: disconnect ourselves from any future events
        disconnect(comms().get(), &ca::Comms::progStarted, this, &StartDevicesOp::onProgStarted);

        // Done!
        progress(QStringLiteral("Unlocked %1 device[s]").arg(m_nDevices), 100);
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
    connect(comms().get(), &ca::Comms::progEnded, this, &StopDevicesOp::onProgEnd);

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

    int progr = std::min(static_cast<int>(100.0f * (m_nDevices - m_devices.size()) / m_nDevices), 99);
    progress(QStringLiteral("Locked device %1 (%2 of %3)")
             .arg(devIdStr(devId)).arg(m_devices.size() + 1).arg(m_nDevices),
             progr);

    if(m_devices.empty())
    {
        // IMPORTANT: disconnect ourselves from any future events
        disconnect(comms().get(), &ca::Comms::progEnded, this, &StopDevicesOp::onProgEnd);

        // Done!
        progress(QStringLiteral("Locked %1 device[s]").arg(m_nDevices), 100);
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

    elfInfo(*m_elf, loggerSafe());

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

    // [10..14%]: Check device stats, list segments, build flash map
    progress(QStringLiteral("Checking if %1 is compatibile with ELF").arg(devIdS), 10);
    if(devStats.elfMachine != m_elf->get_machine())
    {
        progress(QStringLiteral("%1 ELF machine mismatch").arg(devIdS), -2);
        log(CA_ERROR,
            QStringLiteral("%1 has machine type %2 but ELF e_machine is %3")
            .arg(devIdS).arg(devStats.elfMachine).arg(m_elf->get_machine()));
        return;
    }

    progress(QStringLiteral("Listing ELF segments to flash to %1").arg(devIdS), 11);
    ElfioSegments segments;
    listElfSegmentsToFlash(*m_elf, segments, loggerSafe());

    progress(QStringLiteral("Building ELF flash map for %1").arg(devIdS), 12);
    m_flashMap = FlashMap(segments, devStats.pageSize);

    progress(QStringLiteral("ELF flash map for %1 built").arg(devIdS), 13);
    log(CA_DEBUG,
        QStringLiteral("%1: %3 pages of size %2B to be flashed")
        .arg(devIdS).arg(devStats.pageSize).arg(m_flashMap.numPages()));

    if(m_flashMap.pages().size() == 0)
    {
        progress(QStringLiteral("Nothing to flash to %1; ELF flash map is empty").arg(devIdS),
                 100);
        return;
    }

    // [15..100%]: Send page flash commands for pages in flash map
    progress(QStringLiteral("Flashing pages to %1").arg(devIdS), 15);

    auto firstPage = m_flashMap.pages().begin();
    connect(comms().get(), &Comms::pageFlashed, this, &FlashElfOp::onPageFlashed);
    connect(comms().get(), &Comms::pageFlashErrored, this, &FlashElfOp::onPageFlashErrored);
    comms()->flashPage(devId, firstPage->first, firstPage->second);

    // Asked to flash the first page; wait for `onPageFlashed()` or `onPageFlashErrored()`
}

void FlashElfOp::onPageFlashed(CAdevId devId, uint32_t pageAddr)
{
    if(devId != m_devId)
    {
        // Not the device we are flashing
        return;
    }
    QString devIdS = devIdStr(m_devId);

    // [15..100%]: Page flashing
    m_flashMap.pages().erase(pageAddr);

    size_t nPagesFlashed = m_flashMap.numPages() - m_flashMap.pages().size();
    constexpr int prevProgress = 15;
    int progr = std::min(prevProgress + static_cast<int>(float(100 - prevProgress) * nPagesFlashed / m_flashMap.numPages()), 99);
    progress(QStringLiteral("Flashed %2 of %3 to %1")
             .arg(devIdS).arg(nPagesFlashed).arg(m_flashMap.numPages()), progr);

    if(m_flashMap.pages().empty())
    {
        // IMPORTANT: Make sure to disconnect ourselves from all future events
        disconnect(comms().get(), &Comms::pageFlashed, this, &FlashElfOp::onPageFlashed);
        disconnect(comms().get(), &Comms::pageFlashErrored, this, &FlashElfOp::onPageFlashErrored);

        progress(QStringLiteral("Done flashing %1").arg(devIdS), 100);
        return;
    }

    auto nextPage = m_flashMap.pages().begin();
    comms()->flashPage(devId, nextPage->first, nextPage->second);

    // Asked to flash the next page; wait for `onPageFlashed()` or `onPageFlashErrored()`
}

void FlashElfOp::onPageFlashErrored(CAdevId devId, uint32_t pageAddr, uint16_t expectedCrc, uint16_t recvdCrc)
{
    if(devId != m_devId)
    {
        // Not the device we are flashing
        return;
    }

    auto failedPage = m_flashMap.pages().find(pageAddr);
    if(failedPage == m_flashMap.pages().end())
    {
        log(CA_WARNING,
            QStringLiteral("%1: page at %2 failed to flash, but wasn't supposed to be flashed anyways")
            .arg(devIdStr(m_devId)).arg(hexStr(pageAddr, sizeof(pageAddr) * 2)));
        return;
    }

    log(CA_WARNING,
        QStringLiteral("%1: flashing failed for page at %2 (expected CRC: %3, received: %4)")
        .arg(devIdStr(m_devId)).arg(hexStr(pageAddr, sizeof(pageAddr) * 2))
        .arg(hexStr(expectedCrc)).arg(hexStr(recvdCrc)));

    // Retry flashing the page (potentially forever!)
    comms()->flashPage(m_devId, failedPage->first, failedPage->second);
}

}
