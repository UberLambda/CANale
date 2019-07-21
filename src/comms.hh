// CANale/src/comms.hh - Manages communications with the master board
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef COMMS_HH
#define COMMS_HH

#include <utility>
#include <unordered_map>
#include <QObject>
#include <QByteArray>
#include <QCanBusDevice>
#include <QSharedPointer>
#include "types.hh"

namespace ca
{

/// Statistics about a CANnuccia device.
struct DeviceStats
{
    uint32_t pageSize; ///< The size of a flash page in bytes.
    uint16_t nFlashPages; ///< The total number of `pageSize`d flash pages.
    uint16_t elfMachine; ///< The ELF machine type (`e_machine`).
};

/// Implementation of the CANnuccia protocol over `QCanBusDevice`.
class Comms : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSharedPointer<QCanBusDevice> can READ can WRITE setCan)

public:
    /// The id of a CANnuccia device.
    using DevId = CAdevId;


    Comms(QObject *parent=nullptr);
    ~Comms();

    /// Gets the link used to communicate with the CANnuccia network.
    inline QSharedPointer<QCanBusDevice> can()
    {
        return m_can;
    }

    /// Sets the link to use to communicate with the CANnuccia network.
    inline void setCan(QSharedPointer<QCanBusDevice> can)
    {
        if(m_can)
        {
            disconnect(m_can.get(), &QCanBusDevice::framesReceived,
                       this, &Comms::framesReceived);
        }
        m_can = can;
        if(can)
        {
            connect(m_can.get(), &QCanBusDevice::framesReceived,
                    this, &Comms::framesReceived);
        }
    }

    /// Returns whether a CAN link with the CANnuccia network is present or not.
    inline operator bool() const
    {
        return bool(m_can);
    }

public slots:
    /// Sends a PROG_REQ to the device with id `devId`. If and when the PROG_REQ_RESP
    /// is received, sends an UNLOCK command. Finally, if and when UNLOCKED is
    /// received, emits `progStarted()`.
    void progStart(DevId devId);

    /// Sends a PROG_DONE to the device with id `devId`.
    /// Emits `progEnded()` if and when PROG_DONE_ACK is received.
    void progEnd(DevId devId);

    /// Writes to the flash page at `pageAddr` in the device with id `devId`.
    /// Calculates the CRC16/XMODEM of the writes and compares it with the target;
    /// emits `pageFlashed()` if and when the checksum reponse is received from the
    /// device.
    void flashPage(DevId devId, uint32_t pageAddr, QByteArray pageData);

signals:
    /// Emitted after programming a device is started (PROG_REQ_RESP + UNLOCKED).
    /// Outputs the stats obtained from the PROG_REQ_RESP.
    void progStarted(DevId devId, DeviceStats outDeviceStats);

    /// Emitted after a device exits programming mode (PROG_DONE_ACK).
    void progEnded(DevId devId);

    /// Emitted after a device has sent the CRC16/XMODEM of a page after it being
    /// written, it matched the expected value, and it successfully committed
    /// the writes to flash.
    void pageFlashed(DevId devId, uint32_t pageAddr);

    /// Emitted after a device has sent the CRC16/XMODEM of a page after it being
    /// written, it did not match the expected value, and so no writes were
    /// committed to that page.
    void pageFlashErrored(DevId devId, uint32_t pageAddr, uint16_t expectedCrc, uint16_t recvdCrc);


private:
    QSharedPointer<QCanBusDevice> m_can;

    struct DeviceState
    {
        static constexpr uint32_t NO_PAGE = static_cast<uint32_t>(-1);

        DeviceStats stats{0, 0, 0}; ///< Stats about this device
        std::unordered_map<uint32_t, QByteArray> pageFlashData{}; ///< page address -> data to flash there
        uint32_t selPageAddr{NO_PAGE}; ///< Currently-selected page (as indicated by PAGE_SELECTED)
                                       ///< or NO_PAGE if no page is being flashed currently
    };
    std::unordered_map<DevId, DeviceState> m_deviceStates;

    /// Sends a command to the device at `devId` asking it to SELECT_PAGE
    /// the flash page at `pageAddr`.
    void sendSelectPageCmd(DevId devId, uint32_t pageAddr);

    /// Sends WRITE commands to write `pageData` to the device at `devId`.
    /// The WRITEs will have <=8 bytes of payload data each.
    void sendPageWriteCmds(DevId devId, QByteArray pageData);

    /// Sends a SELECT_PAGE command to the device at `devId`, selecting the first
    /// page in `m_deviceStates[devId].pageFlashData` whose address is NOT
    /// `m_deviceStats[devId].selPageAddr`.
    /// Does nothing if there are no pages to flash for the device.
    void selectNextPageToFlash(DevId devId);

private slots:
    /// Handles CAN frames being received.
    void framesReceived();
};

}

#endif // COMMS_HH
