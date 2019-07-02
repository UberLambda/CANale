// CANale/src/cli/main.cc - The entry point of CANale's command-line interface
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include "canale.hh"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    CAconfig config;
    config.canInterface = "socketcan|vcan0";
    config.logHandler = [](CAlogLevel level, const char *msg)
    {
        qDebug() << level << "-" << msg;
    };

    CAinst inst;
    inst.init(config);

    auto onProgressHandler = [](const char *message, int progress, void *userData)
    {
        qDebug().nospace() << message << ": " << progress << "%";
    };
    ca::ProgressHandler onProgress{onProgressHandler};

    QFile elfFile("/home/paolo/devel/CANnuccia/build/avr-release/cn.elf");
    elfFile.open(QFile::ReadOnly);
    QByteArray elfData = elfFile.readAll();
    inst.addOperation(new ca::FlashElfOp(onProgress, 0xEF, elfData));

    //return app.exec();
    return 0;
}
