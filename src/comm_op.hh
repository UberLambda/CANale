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
#include <memory>
#include <QObject>
#include <QString>
#include <QSet>
#include <QByteArray>
#include <QSharedPointer>
#include <elfio/elfio.hpp>
#include "types.hh"
#include "elf.hh"


namespace ca
{

class Comms; // (#include "comms.hh")
struct DeviceStats; // (#include "comms.hh")

/// An operation involving `Comms`; it sends and receives messages/ACKs and keeps
/// track of its own progress.
class Operation : public QObject
{
    Q_OBJECT

public:
    /// Initializes the operation given the progress handler it will invoke.
    Operation(ProgressHandler onProgress, QObject *parent=nullptr);
    virtual ~Operation();

    /// Returns the progress handler passed to the constructor.
    inline ProgressHandler &onProgress()
    {
        return m_onProgress;
    }

    /// Returns whether the operation was `start()`ed or not.
    inline bool isStarted() const
    {
        return m_started;
    }

public slots:
    /// Starts the operation.
    /// It will use `Comms` to communicate from/to devices and `logger` (if any)
    /// to log information about the ongoing operation.
    void start(QSharedPointer<Comms> comms, ca::LogHandler *logger);

protected:
    /// Invoked when the operation is `start()`ed.
    virtual void started() = 0;

    /// Returns the comms passed to `start()`.
    inline QSharedPointer<Comms> &comms()
    {
        return m_comms;
    }

    /// Returns the log handler passed to `start()` (if any).
    inline ca::LogHandler *logger()
    {
        return m_logger;
    }

    /// Returns `logger()` if present or a no-op logger if it is not.
    inline ca::LogHandler &loggerSafe()
    {
        static LogHandler nullLogger = {};
        return m_logger ? *m_logger : nullLogger;
    }

    /// Calls `onProgress(message, progress)`. If `doLog` is `true` also logs the
    /// progress message (as CA_INFO or CA_ERROR depending on if `progress` is
    /// negative).
    inline void progress(QString message, int progress, bool doLog=true)
    {
        m_onProgress(message, progress);
        if(doLog)
        {
            CAlogLevel logLevel;
            QString logMsg;
            if(progress >= 0)
            {
                logLevel = CA_INFO;
                logMsg = QStringLiteral("[%2%] %1").arg(message).arg(progress, 3);
            }
            else
            {
                logLevel = CA_ERROR;
                logMsg = QStringLiteral("%1 [error %2]").arg(message).arg(-progress);
            }
            log(logLevel, logMsg);
        }
    }

    /// Convenience function to call `logger()` only if it is non-null.
    inline void log(CAlogLevel level, QString message)
    {
        if(m_logger)
        {
            m_logger->call(level, message);
        }
    }

private:
    ProgressHandler m_onProgress;
    bool m_started;
    QSharedPointer<Comms> m_comms;
    ca::LogHandler *m_logger;
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

/// An `Operation` that unlocks a target and flashes an ELF file to it.
///
/// Retries flashing a page until it succeeds (CRC matching); potentially
/// retries forever!
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
    std::unique_ptr<ELFIO::elfio> m_elf;
    FlashMap m_flashMap;

    void started() override;

private slots:
    void onProgStarted(CAdevId devId, DeviceStats devStats);
    void onPageFlashed(CAdevId devId, uint32_t pageAddr);
    void onPageFlashErrored(CAdevId devId, uint32_t pageAddr, uint16_t expectedCrc, uint16_t recvdCrc);
};

}

#endif // COMM_OP_HH
