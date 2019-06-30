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
#include <elfio/elfio.hpp>
#include "util.hh"
#include "moc_canale.cpp"

CAinst::CAinst(QObject *parent)
    : QObject(parent),
      logHandler(nullptr), progressHandler(nullptr), m_can(nullptr),
      m_canConnected(false), m_comms(new ca::Comms(this))
{
    connect(this, &CAinst::logged, [this](CAlogLevel level, QString message)
    {
        if(logHandler)
        {
            logHandler(level, qPrintable(message));
        }
    });
    connect(this, &CAinst::progressed, [this](QString descr, unsigned progress)
    {
        if(progressHandler)
        {
            progressHandler(qPrintable(descr), progress);
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
    progressHandler = config.progressHandler;

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
                    QString("Invalid CAN interface \"%1\"").arg(config.canInterface));
        return false;
    }

    emit logged(CA_INFO, QString("Creating CAN link on interface \"%1\"").arg(config.canInterface));

    QString err;
    QSharedPointer<QCanBusDevice> canDev(QCanBus::instance()->createDevice(canToks[0], canToks[1], &err));
    if(!canDev)
    {
        emit logged(CA_ERROR,
                    QString("Failed to create CAN link: %2").arg(err));
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
                    QString("Failed to connect to CAN link. Error [%1]: %2")
                    .arg(can->error()).arg(can->errorString()));
        return false;
    }

    emit logged(CA_INFO, "CAN link estabilished");
    m_can = can;
    m_comms->setCan(m_can);
    return true;
}

bool CAinst::flashELF(unsigned devId, QByteArray elfData)
{
    if(elfData.isNull() || elfData.isEmpty() || !m_comms)
    {
        return false;
    }

    ca::MemIStream elfDataStream(reinterpret_cast<uint8_t *>(elfData.data()),
                                 static_cast<size_t>(elfData.size()));
    ELFIO::elfio elf;
    if(!elf.load(elfDataStream))
    {
        emit logged(CA_ERROR, "Failed to load ELF");
        return false;
    }

    // FIXME IMPLEMENT
    return false;
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

int caFlash(CAinst *ca, unsigned devId, const char *elfPath)
{
    FILE *fp = fopen(elfPath, "rb");
    if(!fp)
    {
        return 0;
    }
    int result = caFlashFp(ca, devId, fp);
    fclose(fp);
    return result;
}

int caFlashFp(CAinst *ca, unsigned devId, FILE *elfFp)
{
    if(!ca || !elfFp)
    {
        return 0;
    }

    fseek(elfFp, 0, SEEK_END);
    auto elfSize = static_cast<unsigned long>(ftell(elfFp));
    fseek(elfFp, 0, SEEK_SET);
    auto elfData = new unsigned char[elfSize];
    size_t readSize = fread(elfData, 1, elfSize, elfFp);

    int result = caFlashMem(ca, devId, readSize, elfData);

    delete[] elfData;
    return result;
}

int caFlashMem(CAinst *ca, unsigned devId, unsigned long elfSize, const unsigned char elfData[])
{
    if(!ca)
    {
        return 0;
    }
    QByteArray elfDataArr(reinterpret_cast<const char *>(elfData),
                          static_cast<int>(elfSize)); // (copies the data)
    return ca->flashELF(devId, elfDataArr);
}
