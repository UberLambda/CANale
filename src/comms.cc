// CANale/src/comms.cc - Implementation of CANale/src/comms.hh
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "comms.hh"

extern "C"
{
#include "common/can_msgs.h"
}
#include "moc_comms.cpp"

namespace ca
{

Comms::Comms(QObject *parent)
    : QObject(parent), m_can(nullptr)
{
}

Comms::~Comms() = default;

void Comms::progStart(unsigned devId, DeviceStats *outDeviceStats)
{
    // FIXME IMPLEMENT
}

void Comms::progEnd(unsigned devId)
{
    // FIXME IMPLEMENT
}

void Comms::flashPage(unsigned devId, uint32_t pageAddr, size_t pageLen, const uint8_t pageData[],
                      QTime timeout)
{
    // FIXME IMPLEMENT
}

}

