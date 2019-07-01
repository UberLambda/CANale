// CANale/src/canale.hh - C++/Qt CANale API based on CANale/include/canale.h
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CANALE_HH
#define CANALE_HH

#include "canale.h"

#include <functional>
#include <QObject>
#include <QSharedPointer>
#include <QCanBusDevice>
#include <QByteArray>
#include <QList>
#include <elfio/elfio.hpp>
#include "comms.hh"

namespace ca
{
    using Inst = ::CAinst;

    /// A C++ equivalent of a `CAprogressHandler`.
    using ProgressHandler = std::function<void(const char *description, int progress,
                                               CAdevId devId, void *userData)>;
}

struct CAinst : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CAlogHandler logHandler MEMBER logHandler)

public:

    CAlogHandler logHandler;

    CAinst(QObject *parent=nullptr);
    ~CAinst();

    /// Returns whether this CANale instance was properly `init()`ed or not.
    inline operator bool() const
    {
        return bool(m_comms);
    }

    /// Returns the Comms associated to this CAinst.
    inline ca::Comms *comms()
    {
        return m_comms;
    }

signals:
    /// Emitted when a new message is to be logged.
    /// Called whenever `logHandler` is to be called.
    void logged(CAlogLevel level, QString message);

public slots:
    /// Initializes this CANale instance given its init configuration.
    /// Returns true on success or false otherwise.
    bool init(const CAconfig &config);

    /// Initializes this CANale instance given its CAN link with the CANnuccia
    /// network. Calls `can->connectDevice()`.
    /// Returns true on success or false otherwise.
    bool init(QSharedPointer<QCanBusDevice> can);


    /// Sends PROG_START commands to all devices in `devIds`, followed by UNLOCKs as
    /// they respond.
    /// Calls the log handler and given progress handler (if any) as appropriate.
    void startDevices(QList<CAdevId> devIds,
                      ca::ProgressHandler onProgress, void *onProgressUserData);

    /// Sends PROG_DONE commands to all devices in `devIds`, waiting for their ACK.
    /// Calls the log handler and given progress handler (if any) as appropriate.
    void stopDevices(QList<CAdevId> devIds,
                     ca::ProgressHandler onProgress, void *onProgressUserData);

    /// Flashes an ELF file (whose contents are in `elfData`) to the device board with
    /// id `devId`.
    /// Calls the log handler and given progress handler (if any) as appropriate.
    void flashELF(CAdevId devId, QByteArray elfData,
                  ca::ProgressHandler onProgress, void *onProgressUserData);


private:
    QSharedPointer<QCanBusDevice> m_can; ///< The link to the CAN network.
    bool m_canConnected; ///< Did `m_can->connectDevice()` succeed?
    ca::Comms *m_comms; ///< The CANnuccia protocol interface.

    /// Appends the list of segments in `elf` that will have to be flashed.
    /// Also logs some diagnostic output about `elf`.
    /// Returns the number of segments appended to `outSegments`.
    ///
    /// Segments to be flashed have the PT_LOAD type and a `fileSize` greater
    /// than zero; they will correspond to `fileSize` bytes to be written at
    /// `physAddr` in the target device's flash.
    unsigned listSegmentsToFlash(const ELFIO::elfio &elf, std::vector<ELFIO::segment *> &outSegments);
};

#endif // CANALE_HH
