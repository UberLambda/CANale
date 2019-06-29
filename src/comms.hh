// CANale/src/comms.hh - Manages communications with the master board
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef COMMS_HH
#define COMMS_HH

#include <QSharedPointer>
#include <QCanBusDevice>
#include <QObject>
#include <QTime>

namespace ca
{

/// Statistics about a CANnuccia device.
struct DeviceStats
{
    uint32_t pageSize; ///< The size of a flash page in bytes.
};

/// Implementation of the CANnuccia protocol over `QCanBusDevice`.
class Comms : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSharedPointer<QCanBusDevice> can READ can WRITE setCan)

public:
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
        m_can = can;
    }

    /// Returns whether a CAN link with the CANnuccia network is present or not.
    inline operator bool() const
    {
        return bool(m_can);
    }

    /// Sends a PROG_REQ to the device with id `devId`, followed by an UNLOCK;
    /// waits for ACKs from the device.
    /// Outputs stats about the device to `outDeviceStats` if it is not null.
    ///
    /// Keeps trying until the PROG_REQ is properly acknowledged (potentially
    /// stalling forever).
    void progStart(unsigned devId, DeviceStats *outDeviceStats);

    /// Sends a PROG_DONE to the device with id devId; waits for ACK from the device.
    /// keepwaits for an acknowledgement for the device.
    ///
    /// Keeps trying until a PROG_DONE_ACK is received (potentially stalling
    /// forever).
    void progEnd(unsigned devId);

    /// Writes to the flash page at `pageAddr` in the device with id `devId`.
    /// Calculates the CRC16/XMODEM of the writes and compares it with the target;
    /// if they don't match or no response is received within the `timeout`
    /// (forever if `timeout.isNull()`) retries writing - potentially retrying forever.
    void flashPage(unsigned devId, uint32_t pageAddr, size_t pageLen, const uint8_t pageData[],
                   QTime timeout=QTime(0, 0, 1));

private:
    QSharedPointer<QCanBusDevice> m_can;
};

}

#endif // COMMS_HH
