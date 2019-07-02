// CANale/src/comm_op.cc - Implementation of CANale/src/comm_op.hh
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "comm_op.hh"

namespace ca
{

Operation::Operation(ProgressHandler onProgress, QObject *parent)
    : QObject(parent), m_onProgress(std::move(onProgress))
{
}

Operation::~Operation()
{
}

void Operation::start(QSharedPointer<Comms> comms)
{
    m_comms = comms;
    started();
}

void Operation::progress(QString message, int progress)
{
    m_onProgress(message, progress);
}


StartDevicesOp::StartDevicesOp(ProgressHandler onProgress,
                               QList<CAdevId> devices, QObject *parent)
    : Operation(onProgress, parent), m_devices(devices)
{
}

void StartDevicesOp::started()
{
    // FIXME IMPLEMENT!
    progress(QStringLiteral("TODO"), -1);
}


StopDevicesOp::StopDevicesOp(ProgressHandler onProgress,
                               QList<CAdevId> devices, QObject *parent)
    : Operation(onProgress, parent), m_devices(devices)
{
}

void StopDevicesOp::started()
{
    // FIXME IMPLEMENT!
    progress(QStringLiteral("TODO"), -1);
}


FlashElfOp::FlashElfOp(ProgressHandler onProgress,
                       CAdevId devId, QByteArray elfData, QObject *parent)
    : Operation(onProgress, parent)
{
}

void FlashElfOp::started()
{
    // FIXME IMPLEMENT!
    progress(QStringLiteral("TODO"), -1);
}

}
