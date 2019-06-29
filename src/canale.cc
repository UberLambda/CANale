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
#include "moc_canale.cpp"

CAinst::CAinst(QObject *parent)
    : QObject(parent),
      logHandler(nullptr), progressHandler(nullptr), m_can(nullptr), m_canConnected(false)
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
    return true;
}

bool CAinst::flashELF(unsigned devId, unsigned long elfSize, const unsigned char elfData[])
{
    // FIXME IMPLEMENT
    if(!elfData || !elfSize)
    {
        return false;
    }
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
    return caFlashFp(ca, devId, fopen(elfPath, "r"));
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
    int result = caFlashMem(ca, devId, elfSize, elfData);
    delete[] elfData;
    return result;
}

int caFlashMem(CAinst *ca, unsigned devId, unsigned long elfSize, const unsigned char elfData[])
{
    if(!ca)
    {
        return 0;
    }
    return ca->flashELF(devId, elfSize, elfData);
}
