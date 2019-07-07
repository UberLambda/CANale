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

#include <memory>
#include <deque>
#include <QObject>
#include <QSharedPointer>
#include <QCanBusDevice>
#include <QByteArray>
#include <QList>
#include <elfio/elfio.hpp>
#include "comms.hh"
#include "types.hh"
#include "comm_op.hh"

namespace ca
{
    using Inst = ::CAinst;
}

struct CAinst : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ca::LogHandler logHandler READ logHandler WRITE setLogHandler)

public:
    CAinst(QObject *parent=nullptr);
    ~CAinst();

    /// Returns whether this CANale instance was properly `init()`ed or not.
    inline operator bool() const
    {
        return bool(m_comms);
    }

    /// Returns the Comms associated to this CAinst.
    inline QSharedPointer<ca::Comms> comms()
    {
        return m_comms;
    }

    /// Returns the log handler associated to this CAinst.
    inline ca::LogHandler &logHandler()
    {
        return m_logHandler;
    }

    /// Sets the log handler associated to this CAinst.
    inline void setLogHandler(ca::LogHandler logHandler)
    {
        m_logHandler = logHandler;
    }

public slots:
    /// Initializes this CANale instance given its init configuration.
    /// Returns true on success or false otherwise.
    bool init(const CAconfig &config);

    /// Initializes this CANale instance given its CAN link with the CANnuccia
    /// network. Calls `can->connectDevice()`.
    /// Returns true on success or false otherwise.
    bool init(QSharedPointer<QCanBusDevice> can);


    /// Enqueues an operation to be performed on this `CAinst`.
    /// The `CAinst` will take ownership of the pointer.
    ///
    /// Operations are started in FIFO order; check the operation's progress
    /// handler for its status.
    void addOperation(ca::Operation *operation);


private:
    ca::LogHandler m_logHandler; ///< The log handler associated to this CAinst.
    QSharedPointer<QCanBusDevice> m_can; ///< The link to the CAN network.
    bool m_canConnected; ///< Did `m_can->connectDevice()` succeed?
    QSharedPointer<ca::Comms> m_comms; ///< The CANnuccia protocol interface.


    std::deque<std::unique_ptr<ca::Operation>> m_operations; ///< All currently-ongoing operations.
};

#endif // CANALE_HH
