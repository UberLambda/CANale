// CANale/src/types.hh - Misc. CANale C++ types
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef TYPES_HH
#define TYPES_HH

#include <functional>
#include <QObject>
#include <QString>
#include <QtGlobal>

extern "C"
{
#include "canale.h"
#include "api.h"
}

namespace ca
{

/// A C++/Qt equivalent of a `CAprogressHandler` and its user data.
struct CA_API ProgressHandler : public QObject
{
    Q_OBJECT

public:
    /// The actual progress handler.
    std::function<void(const char *description, int progress, void *userData)> handler;

    /// The user data that is passed to `handler`.
    void *userData;


    /// Constructs a no-op progress handler.
    ProgressHandler(QObject *parent=nullptr)
        : QObject(parent), handler(), userData(nullptr)
    {
    }

    /// Constructs a progress handler from the given functor and user data.
    ProgressHandler(const decltype(handler) &handler, void *userData=nullptr,
                    QObject *parent=nullptr)
        : QObject(parent), handler(handler), userData(userData)
    {
    }
    ProgressHandler(decltype(handler) &&handler, void *userData=nullptr,
                    QObject *parent=nullptr)
        : QObject(parent), handler(std::move(handler)), userData(userData)
    {
    }

    ProgressHandler(const ProgressHandler &toCopy)
    {
        operator=(toCopy);
    }
    ProgressHandler &operator=(const ProgressHandler &toCopy)
    {
        setParent(toCopy.parent());
        handler = toCopy.handler;
        userData = toCopy.userData;
        return *this;
    }

    ProgressHandler(ProgressHandler &&toMove)
    {
        operator=(std::move(toMove));
    }
    ProgressHandler &operator=(ProgressHandler &&toMove)
    {
        setParent(toMove.parent());
        handler = std::move(toMove.handler);
        userData = std::move(toMove.userData);
        return *this;
    }

    virtual ~ProgressHandler() = default;

    /// Returns whether the handler is valid or not.
    inline operator bool() const
    {
        return bool(handler);
    }


    /// Invokes the progress handler.
    /// Emits `progressed()` or `done()` as appropriate.
    inline void operator()(QString message, int progress)
    {
        call(message, progress);
    }

signals:
    /// Emitted when the progress handler is called with a progress of 0 to 99
    /// (i.e. "operation ongoing").
    void progressed(QString message, unsigned progress);

    /// Emitted when the progress handler is called with a progress of 100 (i.e.
    /// "operation done", success=true) or a negative value (i.e. "error occurred",
    /// success=false).
    void done(QString message, bool success);

public slots:
    /// Equivalent to `operator()()`.
    void call(QString message, int progress)
    {
        if(handler)
        {
            handler(qPrintable(message), progress, userData);
        }

        if(progress >= 100)
        {
            // Operation done
            emit done(message, true);
        }
        else if(progress < 0)
        {
            // Operation failed
            emit done(message, false);
        }
        else
        {
            // Progress made
            emit progressed(message, unsigned(progress));
        }
    }
};

/// A C++/Qt equivalent of a `CAlogHandler`.
struct CA_API LogHandler : public QObject
{
    Q_OBJECT

public:
    /// The actual log handler.
    std::function<void(CAlogLevel level, const char *message)> handler;


    /// Constructs a no-op log handler.
    LogHandler(QObject *parent=nullptr)
        : QObject(parent), handler()
    {
    }

    /// Constructs a log handler from the given functor.
    LogHandler(const decltype(handler) &handler, QObject *parent=nullptr)
        : QObject(parent), handler(handler)
    {
    }
    LogHandler(decltype(handler) &&handler, QObject *parent=nullptr)
        : QObject(parent), handler(std::move(handler))
    {
    }

    LogHandler(const LogHandler &toCopy)
    {
        operator=(toCopy);
    }
    LogHandler &operator=(const LogHandler &toCopy)
    {
        setParent(toCopy.parent());
        handler = toCopy.handler;
        return *this;
    }

    LogHandler(LogHandler &&toMove)
    {
        operator=(std::move(toMove));
    }
    LogHandler &operator=(LogHandler &&toMove)
    {
        setParent(toMove.parent());
        handler = std::move(toMove.handler);
        return *this;
    }

    virtual ~LogHandler() = default;

    /// Returns whether the handler is valid or not.
    inline operator bool() const
    {
        return bool(handler);
    }


    /// Invokes the log handler.
    /// Emits `logged()` as appropriate.
    inline void operator()(CAlogLevel level, QString message)
    {
        call(level, message);
    }

signals:
    /// Emitted when a message is logged.
    void logged(CAlogLevel level, QString message);

public slots:
    /// Equivalent to `operator()()`.
    void call(CAlogLevel level, QString message)
    {
        if(handler)
        {
            handler(level, qPrintable(message));
        }
        emit logged(level, message);
    }
};

}

#endif // TYPES_HH
