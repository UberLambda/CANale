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
#include <QList>
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
    Operation(ProgressHandler onProgress, QObject *parent=nullptr);
    virtual ~Operation();

public slots:
    void start(QSharedPointer<Comms> comms);

signals:
    void progressed(QString message, unsigned progress);
    void done(QString message, bool success);

protected:
    virtual void started() = 0;

    void progress(QString message, int progress);

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
public:
    StartDevicesOp(ProgressHandler onProgress,
                   QList<CAdevId> devices,
                   QObject *parent=nullptr);
    ~StartDevicesOp() override = default;

private:
    QList<CAdevId> m_devices;

    void started() override;
};

/// An `Operation` that sends PROG_DONE commands to a list of devices (and
/// waits for their response).
class StopDevicesOp : public Operation
{
public:
    StopDevicesOp(ProgressHandler onProgress,
                  QList<CAdevId> devices,
                  QObject *parent=nullptr);
    ~StopDevicesOp() override = default;

private:
    QList<CAdevId> m_devices;

    void started() override;
};

/// An `Operation` that flashes an ELF file to a target.
class FlashElfOp : public Operation
{
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
