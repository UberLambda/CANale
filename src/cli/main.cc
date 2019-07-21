// CANale/src/cli/main.cc - The entry point of CANale's command-line interface
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <numeric>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include "canale.hh"
#include "util.hh"


/// A convenience function more or less equivalent to `QObject::tr`.
inline QString tr(const char *text)
{
    return QCoreApplication::translate("canale-cli", text);
};



/// Sets up the command-line argument parser.
void initArgParser(QCommandLineParser &argParser)
{
    QCoreApplication::setApplicationName("CANale");
    QCoreApplication::setApplicationVersion("0.1");

    argParser.setApplicationDescription(tr("Manages a CANnuccia network."));
    argParser.addHelpOption();
    argParser.addOptions({
        {{"backend", "b"},
         tr("The CAN backend to use (ex. 'socketcan')."), "backend"},
        {{"interface", "i"},
         tr("The CAN interface to use (ex. 'vcan0')."), "interface"},
    });
    argParser.addPositionalArgument("operations",
                                    tr("The operations to perform, in order."), "operations...");
}

/// Returns an operation as parsed from a string description.
/// The operation will be created to use the given progress handler.
/// Returns true if successful or false otherwise (parsing error).
/// Logs any errors to the given log handler.
ca::Operation *parseOperation(QString opDescr, ca::ProgressHandler &onProgress, ca::LogHandler &log)
{
    QStringList tokens = opDescr.split("+");

    enum class OpType
    {
        StartDevices,
        StopDevices,
        FlashElf,

    } opType;

    QString opTypeName = tokens.first().toLower();
    if(opTypeName == "start")
    {
        opType = OpType::StartDevices;
    }
    else if(opTypeName == "stop")
    {
        opType = OpType::StopDevices;
    }
    else if(opTypeName == "flash")
    {
        opType = OpType::FlashElf;
    }
    else
    {
        log(CA_ERROR, tr("Unrecognized operation: \"%1\"").arg(opDescr));
        return nullptr;
    }

    auto parseDevId = [](const QString &str, CAdevId &outDevId) -> bool
    {
        long devIdLong;
        bool numOk = ca::parseInt(str, devIdLong);
        numOk = numOk
                && (devIdLong >= std::numeric_limits<CAdevId>::min())
                && (devIdLong <= std::numeric_limits<CAdevId>::max());

        if(numOk)
        {
            outDevId = static_cast<CAdevId>(devIdLong);
            return true;
        }
        else
        {
            return false;
        }
    };

    auto parseDevList = [&](const QString &listStr, QSet<CAdevId> &outList) -> bool
    {
        QStringList idStrs = listStr.split(",");
        for(const QString &idStr : idStrs)
        {
            CAdevId devId;
            bool idOk = parseDevId(idStr, devId);
            if(idOk)
            {
                outList.insert(devId);
            }
            else
            {
                log(CA_ERROR, QStringLiteral("Invalid device id: %1").arg(idStr));
                return false;
            }
        }
        return true;
    };

    switch(opType)
    {

    case OpType::StartDevices:
    case OpType::StopDevices:
    {
        if(tokens.length() != 2)
        {
            log(CA_ERROR, tr("Invalid format for start/stop operation: \"%1\"").arg(opDescr));
            return nullptr;
        }

        QSet<CAdevId> devices;
        if(!parseDevList(tokens[1], devices))
        {
            return nullptr;
        }

        if(opType == OpType::StartDevices)
        {
            return new ca::StartDevicesOp(onProgress, devices);
        }
        else
        {
            return new ca::StopDevicesOp(onProgress, devices);
        }
    }

    case OpType::FlashElf:
    {
        if(tokens.length() != 3)
        {
            log(CA_ERROR, tr("Invalid format for flash operation: \"%1\"").arg(opDescr));
            return nullptr;
        }

        CAdevId devId;
        if(!parseDevId(tokens[1], devId))
        {
            log(CA_ERROR, tr("Invalid device id: %1").arg(tokens[1]));
            return nullptr;
        }

        QFile elfFile(tokens[2]);
        if(!elfFile.open(QFile::ReadOnly))
        {
            log(CA_ERROR, tr("Failed to open ELF file: '%1'").arg(tokens[2]));
            return nullptr;
        }

        return new ca::FlashElfOp(onProgress, devId, elfFile.readAll());
    }

    }

    Q_UNREACHABLE();
}


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser argParser;
    initArgParser(argParser);
    argParser.process(app);

    std::string backendStr(qPrintable(argParser.value("backend")));
    std::string interfaceStr(qPrintable(argParser.value("interface")));

    CAconfig config;
    config.canBackend = backendStr.c_str();
    config.canInterface = interfaceStr.c_str();
    config.logHandler = [](CAlogLevel level, const char *msg)
    {
        qWarning() << level << "-" << msg;
    };

    CAinst inst;
    if(!inst.init(config))
    {
        return 1;
    }

    if(argParser.positionalArguments().empty())
    {
        qWarning() << "Nothing to do";
        return 0;
    }

    auto onProgressFunc = [&inst, &app](const char *descr, int progress, void *userData)
    {
        if(progress < 0)
        {
            qCritical() << descr << QStringLiteral(": failed (error %1)").arg(progress);
        }

        // TODO: Show ASCII art progress bar

        if((progress < 0 || progress >= 100) && (inst.numEnqueued() == 0))
        {
            // Last operation, end the program
            qWarning() << "All operations done";
            app.quit();
        }
    };
    ca::ProgressHandler onProgress(onProgressFunc);

    // Parse all operations from command-line first
    QList<ca::Operation *> operations;
    for(const QString &opDescr : argParser.positionalArguments())
    {
        ca::Operation *op = parseOperation(opDescr, onProgress, inst.logHandler());
        if(!op)
        {
            return 2;
        }
        operations.push_back(op);
    }

    // Start them only if they could all be parsed
    for(ca::Operation *op : operations)
    {
        inst.addOperation(op);
    }

    return app.exec();
}
