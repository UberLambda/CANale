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
#include <QSerialPort>
#include <QSerialPortInfo>
#include "moc_canale.cpp"

CAinst::CAinst(QObject *parent)
    : QObject(parent),
      logHandler(nullptr), progressHandler(nullptr), m_serial()
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

    emit logged(CA_DEBUG, "CANale begin");
}

CAinst::~CAinst()
{
    emit logged(CA_DEBUG, "CANale end");
}

bool CAinst::init(QSharedPointer<QIODevice> device)
{
    if(!device)
    {
        emit logged(CA_ERROR, "Master board link not open");
        return false;
    }

    bool readable = device->isReadable(), writable = device->isWritable();
    if(!readable || !writable)
    {
        QString error = QString("Master board link %1%2%3")
                .arg(readable ? "" : "not readable")
                .arg((!readable && !writable) ? " and " : "")
                .arg(writable ? "" : "not writable");
        emit logged(CA_ERROR, error);
    }

    emit logged(CA_INFO, "Master board link open");
    m_serial = device;
    return true;
}

bool CAinst::flashELF(unsigned devId, unsigned long elfSize, const unsigned char elfData[])
{
    // FIXME IMPLEMENT
    return false;
}

// ---- C API to implement for include/canale.h --------------------------------

static constexpr qint32 DEFAULT_SERIAL_BAUD = 115200;

CAinst *caInit(const CAconfig *config)
{
    if(!config)
    {
        return nullptr;
    }

    auto serial = QSharedPointer<QSerialPort>::create();
    if(config->serialPort)
    {
        serial->setPortName(config->serialPort);
    }
    serial->setBaudRate(config->serialBaud ? qint32(config->serialBaud) : DEFAULT_SERIAL_BAUD);

    if(!serial->open(QIODevice::ReadWrite))
    {
        if(config->logHandler)
        {
            QString error = QString("Failed to open serial \"%1\" for master board link: %2")
                    .arg(config->serialPort).arg(serial->errorString());
            config->logHandler(CA_ERROR, qPrintable(error));
        }
    }

    auto inst = new CAinst();
    inst->logHandler = config->logHandler;
    inst->progressHandler = config->progressHandler;

    if(inst->init(serial))
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
