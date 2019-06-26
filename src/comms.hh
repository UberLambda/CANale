// CANale/src/comms.hh - Manages communications with the master board
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef COMMS_HH
#define COMMS_HH

#include <cstdint>
#include <cstddef>
#include <QObject>
#include <QTime>

namespace ca
{

/// Statistics about a CANnuccia device.
struct DeviceStats
{
    uint32_t pageSize; ///< The size of a flash page in bytes.
};

/// The interface to the CANnuccia master board.
class Comms : public QObject
{
    Q_OBJECT

public:
    Comms(QObject *parent=nullptr);
    ~Comms();

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

signals:
    /// Emitted when a CAN packet is to be written to the master board.
    /// `id` is the destination CAN EID, in bxCAN format. `dataLen` will be <= 8.
    void canTX(uint32_t id, size_t dataLen, const uint8_t data[]);

public slots:
    /// Trigger this when a CAN packet is read from the master board.
    /// `id` is the sender's CAN EID, in bxCAN format. `dataLen` must be <= 8.
    void canRX(uint32_t id, size_t dataLen, const uint8_t data[]);
};

}

#endif // COMMS_HH
