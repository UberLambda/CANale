// CANale/src/canale.cc - C++/Qt implementation of CANale/include/canale.h
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "canale.hh"

#include <cstdio>
#include <algorithm>
#include <QtGlobal>
#include <QCanBus>
#include <QCanBusDevice>
#include <elfio/elf_types.hpp>
#include "util.hh"
#include "moc_canale.cpp"

CAinst::CAinst(QObject *parent)
    : QObject(parent),
      logHandler(nullptr), m_can(nullptr),
      m_canConnected(false), m_comms(new ca::Comms(this))
{
    connect(this, &CAinst::logged, [this](CAlogLevel level, QString message)
    {
        if(logHandler)
        {
            logHandler(level, qPrintable(message));
        }
    });
}

CAinst::~CAinst()
{
    if(m_can && m_canConnected)
    {
        m_can->disconnectDevice();
    }
    emit logged(CA_INFO, "CANale halt");
}

bool CAinst::init(const CAconfig &config)
{
    logHandler = config.logHandler;

    emit logged(CA_INFO, "CANale init");

    if(!config.canInterface || config.canInterface[0] == '\0')
    {
        emit logged(CA_ERROR, "No CAN interface specified");
        return false;
    }

    QStringList canToks = QString(config.canInterface).split('|');
    if(canToks.length() != 2)
    {
        emit logged(CA_ERROR,
                    QStringLiteral("Invalid CAN interface \"%1\"").arg(config.canInterface));
        return false;
    }

    emit logged(CA_INFO,
                QStringLiteral("Creating CAN link on interface \"%1\"").arg(config.canInterface));

    QString err;
    QSharedPointer<QCanBusDevice> canDev(QCanBus::instance()->createDevice(canToks[0], canToks[1], &err));
    if(!canDev)
    {
        emit logged(CA_ERROR,
                    QStringLiteral("Failed to create CAN link: %1").arg(err));
        return false;
    }

    return init(canDev);
}

bool CAinst::init(QSharedPointer<QCanBusDevice> can)
{
    if(!can)
    {
        emit logged(CA_ERROR, "CAN link not present");
        return false;
    }

    emit logged(CA_INFO, "Connecting to CAN link...");
    if(!can->connectDevice())
    {
        emit logged(CA_ERROR,
                    QStringLiteral("Failed to connect to CAN link. Error [%1]: %2")
                    .arg(can->error()).arg(can->errorString()));
        return false;
    }

    emit logged(CA_INFO, "CAN link estabilished");
    m_can = can;
    m_comms->setCan(m_can);
    return true;
}

unsigned CAinst::listSegmentsToFlash(const ELFIO::elfio &elf, std::vector<ELFIO::segment *> &outSegments)
{
    emit logged(CA_DEBUG,
                QStringLiteral("%1 ELF segments:").arg(elf.segments.size()));

    QString segmMsg;
    unsigned nOutput = 0;
    for(unsigned i = 0; i < elf.segments.size(); i ++)
    {
        ELFIO::segment *segm = elf.segments[i];

        segmMsg = QStringLiteral("> segment %1: ").arg(i);

        if(segm->get_type() & PT_LOAD)
        {
            if(segm->get_file_size() > 0)
            {
                segmMsg += QStringLiteral("loadable, flash fileSize=%1B (out of memSize=%2B) at physAddr=%3")
                            .arg(segm->get_file_size())
                            .arg(segm->get_memory_size())
                            .arg(hexStr(segm->get_physical_address(), 8));

                outSegments.push_back(segm);
                nOutput ++;
            }
            else
            {
                segmMsg += QStringLiteral("loadable but has fileSize=0B, skip");
            }
        }
        else
        {
            segmMsg += "not loadable, skip";
        }
        emit logged(CA_DEBUG, segmMsg);
    }

    return nOutput;
}

void CAinst::addOperation(ca::Operation *operation)
{
    m_operations.emplace_back(operation);
    ca::Operation *op = m_operations.back().get();

    // Remove the task from the queue as soon as it is done
    connect(&op->onProgress(), &ca::ProgressHandler::done, [op, this]()
    {
        std::remove_if(m_operations.begin(), m_operations.end(),
                       [op](auto &opPtr) { return opPtr.get() == op; });
    });

    op->start(m_comms);
}

// ---- C API to implement for include/canale.h --------------------------------

#define EXPECT_C(expr, message) do { Q_ASSERT(expr); \
    if(ca) { emit ca->logged(CA_ERROR, message); } \
    if(onProgress) { onProgress(message, -1, onProgressUserData); } \
    return; \
} while(0)

CAinst *caInit(const CAconfig *config)
{
    if(!config)
    {
        return nullptr;
    }

    auto inst = new CAinst();
    if(inst->init(*config))
    {
        return inst;
    }
    else
    {
        delete inst;
        return nullptr;
    }
}

void caHalt(CAinst *inst)
{
    delete inst;
}

void caStartDevices(CAinst *ca, unsigned long nDevIds, const CAdevId devIds[],
                    CAprogressHandler onProgress, void *onProgressUserData)
{
    EXPECT_C(ca && (devIds || nDevIds == 0), "Invalid arguments");

    QSet<CAdevId> devIdsSet;
    devIdsSet.reserve(static_cast<int>(nDevIds));
    for(auto *it = devIds; it != (devIds + nDevIds); it ++)
    {
        devIdsSet.insert(*it);
    }

    ca->addOperation(new ca::StartDevicesOp(
        ca::ProgressHandler{onProgress, onProgressUserData}, devIdsSet));
}

void caStopDevices(CAinst *ca, unsigned long nDevIds, const CAdevId devIds[],
                   CAprogressHandler onProgress, void *onProgressUserData)
{
    EXPECT_C(ca && (devIds || nDevIds == 0), "Invalid arguments");

    QSet<CAdevId> devIdsSet;
    devIdsSet.reserve(static_cast<int>(nDevIds));
    for(auto *it = devIds; it != (devIds + nDevIds); it ++)
    {
        devIdsSet.insert(*it);
    }

    ca->addOperation(new ca::StopDevicesOp(
        ca::ProgressHandler{onProgress, onProgressUserData}, devIdsSet));
}

void caFlashELF(CAinst *ca, CAdevId devId,
                unsigned long elfLen, const char *elf,
                CAprogressHandler onProgress, void *onProgressUserData)
{
    EXPECT_C(ca, "Invalid arguments");

    QByteArray elfDataArr(elf, static_cast<int>(elfLen)); // (copies the data)

    ca->addOperation(new ca::FlashElfOp(
        ca::ProgressHandler{onProgress, onProgressUserData}, devId, elfDataArr));
}
