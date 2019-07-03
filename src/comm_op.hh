// CANale/src/comm_op.hh - C++/Qt type used to store an in-progress operation
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef COMM_OP_HH
#define COMM_OP_HH

#include <functional>
#include <QObject>
#include <QString>
#include <QSet>
#include <QByteArray>
#include <QSharedPointer>
#include "types.hh"

namespace ca
{

class Comms; // comms.hh

/// An operation involving `Comms`; it sends and receives messages/ACKs and keeps
/// track of its own progress.
class Operation : public QObject
{
    Q_OBJECT

public:
    /// Initializes the operation given the progress handler it will invoke.
    Operation(ProgressHandler onProgress, QObject *parent=nullptr);
    virtual ~Operation();

public slots:
    /// Starts the operation. It will use `Comms` to communicate from/to devices.
    void start(QSharedPointer<Comms> comms);

    /// Returns the progress handler passed to the constructor.
    inline ProgressHandler &onProgress()
    {
        return m_onProgress;
    }

protected:
    /// Invoked when the operation is `start()`ed.
    virtual void started() = 0;

    /// Returns the comms passed to `start()`.
    inline QSharedPointer<Comms> &comms()
    {
        return m_comms;
    }

private:
    ProgressHandler m_onProgress;
    QSharedPointer<Comms> m_comms;
};

/// An `Operation` that sends PROG_REQ + UNLOCK commands to a list of devices
/// (and waits for their response).
class StartDevicesOp : public Operation
{
    Q_OBJECT

public:
    StartDevicesOp(ProgressHandler onProgress,
                   QSet<CAdevId> devices,
                   QObject *parent=nullptr);
    ~StartDevicesOp() override = default;

private:
    QSet<CAdevId> m_devices;
    int m_nDevices;

    void started() override;

private slots:
    void onProgStarted(CAdevId devId);
};

/// An `Operation` that sends PROG_DONE commands to a list of devices (and
/// waits for their response).
class StopDevicesOp : public Operation
{
    Q_OBJECT

public:
    StopDevicesOp(ProgressHandler onProgress,
                  QSet<CAdevId> devices,
                  QObject *parent=nullptr);
    ~StopDevicesOp() override = default;

private:
    QSet<CAdevId> m_devices;
    int m_nDevices;

    void started() override;

private slots:
    void onProgEnd(CAdevId devId);
};

/// An `Operation` that flashes an ELF file to a target.
class FlashElfOp : public Operation
{
    Q_OBJECT

public:
    FlashElfOp(ProgressHandler onProgress,
               CAdevId devId, QByteArray elfData,
               QObject *parent=nullptr);
    ~FlashElfOp() override = default;

private:
    CAdevId m_devId;
    QByteArray m_elfData;

    void started() override;
};

}

#endif // COMM_OP_HH
