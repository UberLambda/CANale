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
#include <QtGlobal>
#include <QString>

extern "C"
{
#include "canale.h"
}

namespace ca
{

/// A C++ equivalent of a `CAprogressHandler` and its user data.
struct ProgressHandler
{
    std::function<void(const char *description, int progress, void *userData)> handler{};
    void *userData = nullptr;

    inline operator bool() const
    {
        return bool(handler);
    }

    inline void operator()(QString description, int progress)
    {
        if(handler)
        {
            handler(qPrintable(description), progress, userData);
        }
    }
};

}

#endif // TYPES_HH
