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

#include <QObject>
#include <QSharedPointer>
#include <QIODevice>

struct CAinst : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CAlogHandler logHandler MEMBER logHandler)
    Q_PROPERTY(CAprogressHandler progressHandler MEMBER progressHandler)

public:
    CAlogHandler logHandler;
    CAprogressHandler progressHandler;

    CAinst(QObject *parent=nullptr);
    ~CAinst();

    /// Returns whether this CANale instance was properly `init()`ed or not.
    inline operator bool() const;

signals:
    /// Emitted when a new message is to be logged.
    /// Called whenever `logHandler` is to be called.
    void logged(CAlogLevel level, QString message);

    /// Emitted when some progress is made (`CAprogressHandler` equivalent).
    /// Called whenever `progressHandler` is to be called.
    void progressed(QString descr, unsigned progress);

public slots:
    /// Initializes this CANale instance, given the [serial] device used to
    /// communicate with the master board. The device should already be open in
    /// read/write mode.
    /// Returns true on success or false otherwise.
    bool init(QSharedPointer<QIODevice> device);

    /// Flashes the contents of an ELF file to the device board with id `devId`.
    /// Emits `progressed()` and `logged()` as appropriate.
    /// Returns true if flash succeeded, or false on error.
    bool flashELF(unsigned devId, unsigned long elfSize, const unsigned char elfData[]);

private:
    QSharedPointer<QIODevice> m_serial; ///< The link to the master board.
};

namespace ca
{
    using Inst = ::CAinst;
};

#endif // CANALE_HH