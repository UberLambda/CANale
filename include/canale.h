// CANale/include/canale.h - Public C API for CANale
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CANALE_H
#define CANALE_H

#include <stdio.h>

#ifndef __cplusplus
extern "C"
{
#endif

#if defined(WIN32) || defined(_WIN32)
#   if defined(CA_EXPORTS)
#       define CA_API __declspec(dllexport)
#   else
#       define CA_API __declspec(dllimport)
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   define CA_API __attribute__((visibility("default")))
#else
#   define CA_API
#endif

/// An opaque handle to an instance of CANale.
typedef struct CAinst CAinst;

/// The level of a CANale log message.
typedef enum CAlogLevel
{
    CA_DEBUG,
    CA_INFO,
    CA_WARNING,
    CA_ERROR,

} CAlogLevel;

/// An handler for CANale log messages.
typedef void(*CAlogHandler)(CAlogLevel level, const char *message);

/// An handler for CANale progress events. `progress` is 0 to 100.
typedef void(*CAprogressHandler)(const char *descr, unsigned progress);

/// Configuration flags for creating a CANale instance.
typedef struct CAconfig
{
    /// The CAN interface to use to connect to the CANnuccia network, in the
    /// format "<backend>|<interface>". <backend> is the name of the QtCanBus
    /// plugin to use (ex. "socketcan"); <interface> is the CAN interface (ex.
    /// "can0").
    const char *canInterface;

    /// Called when a message is logged by CANale.
    /// Set to null to disable logging.
    CAlogHandler logHandler;

    /// Called when some progress is made by CANale.
    /// Set to null to disable progress reporting.
    CAprogressHandler progressHandler;

} CAconfig;

/// Creates a new instance of CANale given its configuration parameters.
/// Returns null on error; if `config->logHandler` is set, it is invoked
/// with a description of the error.
CA_API CAinst *caInit(const CAconfig *config);

/// Destroys a CANale instance.
/// Does nothing if `inst` is null.
CA_API void caHalt(CAinst *inst);


/// Flashes the ELF file at `elfPath` to the device board with id `devId`.
/// Calls CANale progress and log handlers as appropriate.
/// Returns true if flash succeeded, or false on error.
CA_API int caFlash(CAinst *ca, unsigned devId, const char *elfPath);

/// Same as `caFlash()` but reads the ELF file from a file pointer.
CA_API int caFlashFp(CAinst *ca, unsigned devId, FILE *elfFp);

/// Same as `caFlash()` but reads the ELF file from memory.
CA_API int caFlashMem(CAinst *ca, unsigned devId, unsigned long elfSize, const unsigned char elfData[elfSize]);


#ifndef __cplusplus
}
#endif

#endif // CANALE_H
