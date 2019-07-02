// CANale/src/canale.cc - C++/Qt implementation of CANale/include/canale.h
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "canale.hh"

#include <cstdio>
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

template <typename Num>
inline static QString hexStr(Num num, int nDigits=0)
{
    return QStringLiteral("0x")
            + QStringLiteral("%1").arg(num, nDigits, 16, QChar('0')).toUpper();
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


#define PROGRESS_FAIL(caInst, message, devId) do { \
    if(caInst) { emit caInst->logged(CA_ERROR, message); } \
    if(onProgress) { onProgress(message, -1, devId, onProgressUserData); } \
} while(false)

#define EXPECT(caInst, expr, message, devId) do { Q_ASSERT(expr); \
    if(!(expr)) { PROGRESS_FAIL(caInst, message, devId); return; } \
} while(false)

void CAinst::startDevices(QList<CAdevId> devIds,
                          ca::ProgressHandler onProgress, void *onProgressUserData)
{
    EXPECT(this, m_comms, "CAN link not open", 0xFF);
    // FIXME IMPLEMENT
}

void CAinst::stopDevices(QList<CAdevId> devIds,
                         ca::ProgressHandler onProgress, void *onProgressUserData)
{
    EXPECT(this, m_comms, "CAN link not open", 0xFF);
    // FIXME IMPLEMENT
}

void CAinst::flashELF(CAdevId devId, QByteArray elfData,
                      ca::ProgressHandler onProgress, void *onProgressUserData)
{
    EXPECT(this, !(elfData.isNull() || elfData.isEmpty()), "Invalid arguments", devId);
    EXPECT(this, m_comms, "CAN link not open", devId);

    emit logged(CA_INFO,
                QStringLiteral("Flashing ELF to device %1...").arg(hexStr(devId, 2)));

    ca::MemIStream elfDataStream(reinterpret_cast<uint8_t *>(elfData.data()),
                                 static_cast<size_t>(elfData.size()));
    ELFIO::elfio elf;
    if(!elf.load(elfDataStream))
    {
        PROGRESS_FAIL(this, "Failed to load ELF", devId);
        return;
    }

    emit logged(CA_DEBUG,
                QStringLiteral("ELF machine type: %1").arg(hexStr(elf.get_machine(), 4)));

    std::vector<ELFIO::segment *> segmentsToFlash;
    unsigned nSegmentsToFlash = listSegmentsToFlash(elf, segmentsToFlash);

    // FIXME: send PROG_START to the device, get stats, build a page map,
    // flash and verify the pages
    return;
}

// ---- C API to implement for include/canale.h --------------------------------

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
    if(!ca || !(devIds || nDevIds == 0))
    {
        PROGRESS_FAIL(ca, "Invalid arguments", 0xFF);
        return;
    }

    QList<CAdevId> devIdsList;
    devIdsList.reserve(static_cast<int>(nDevIds));
    for(auto *it = devIds; it != (devIds + nDevIds); it ++)
    {
        devIdsList.push_back(*it);
    }

    ca->startDevices(devIdsList, onProgress, onProgressUserData);
}

void caStopDevices(CAinst *ca, unsigned long nDevIds, const CAdevId devIds[],
                   CAprogressHandler onProgress, void *onProgressUserData)
{
    if(!ca || !(devIds || nDevIds == 0))
    {
        PROGRESS_FAIL(ca, "Invalid arguments", 0xFF);
        return;
    }

    QList<CAdevId> devIdsList;
    devIdsList.reserve(static_cast<int>(nDevIds));
    for(auto *it = devIds; it != (devIds + nDevIds); it ++)
    {
        devIdsList.push_back(*it);
    }

    ca->stopDevices(devIdsList, onProgress, onProgressUserData);
}

void caFlashELF(CAinst *ca, CAdevId devId,
                unsigned long elfLen, const char *elf,
                CAprogressHandler onProgress, void *onProgressUserData)
{
    if(!ca)
    {
        PROGRESS_FAIL(ca, "Invalid arguments", devId);
        return;
    }

    QByteArray elfDataArr(elf, static_cast<int>(elfLen)); // (copies the data)
    return ca->flashELF(devId, elfDataArr, onProgress, onProgressUserData);
}
