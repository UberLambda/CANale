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
#include <stdint.h>
#include "api.h"

#ifndef __cplusplus
extern "C"
{
#endif

/// An opaque handle to an instance of CANale.
typedef struct CA_API CAinst CAinst;

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

/// Configuration flags for creating a CANale instance.
typedef struct CA_API CAconfig
{
    /// The CAN backend to use to connect to the CANnuccia network.
    /// Should be the name of the QtCanBus plugin to use (ex. "socketcan").
    const char *canBackend;

    /// The CAN interface to use to connect to the CANnuccia network.
    /// Should be the name of the interface/port to be used by QtCanBus (ex. "vcan0").
    const char *canInterface;

    /// Called when a message is logged by CANale.
    /// Set to null to disable logging.
    CAlogHandler logHandler;

} CAconfig;

/// Creates a new instance of CANale given its configuration parameters.
/// Returns null on error; if `config->logHandler` is set, it is invoked
/// with a description of the error.
CA_API CAinst *caInit(const CAconfig *config);

/// Destroys a CANale instance.
/// Does nothing if `inst` is null.
CA_API void caHalt(CAinst *inst);


/// The id of a CANnuccia device.
typedef uint8_t CAdevId;

/// An handler for CANale progress events.
/// `progress` is usually 0 to 100. Unless an error occurs, the handler is
/// guaranteed to be called with `progress=100` when the operation completes;
/// a negative progress value is passed whenever an error occurs.
typedef void(*CAprogressHandler)(const char *message, int progress, void *userData);

/// Sends PROG_START commands to all devices in `devIds`, followed by UNLOCKs as
/// they respond.
/// Calls the log handler and given progress handler (if any) as appropriate.
CA_API void caStartDevices(CAinst *ca, unsigned long nDevIds, const CAdevId devIds[nDevIds],
                           CAprogressHandler onProgress, void *onProgressUserData);

/// Sends PROG_DONE commands to all devices in `devIds`, waiting for their ACK.
/// Calls the log handler and given progress handler (if any) as appropriate.
CA_API void caStopDevices(CAinst *ca, unsigned long nDevIds, const CAdevId devIds[nDevIds],
                          CAprogressHandler onProgress, void *onProgressUserData);

/// Flashes an ELF file (whose contents are in `elf`) to the device board with
/// id `devId`.
/// Calls the log handler and given progress handler (if any) as appropriate.
///
/// Sends a PROG_START to the device, but not a PROG_DONE!
CA_API void caFlashELF(CAinst *ca, CAdevId devId,
                       unsigned long elfLen, const char elf[elfLen],
                       CAprogressHandler onProgress, void *onProgressUserData);


/// Returns the number of operations still enqueued into a CANale instance.
CA_API unsigned caNumEnqueued(CAinst *ca);


#ifndef __cplusplus
}
#endif

#endif // CANALE_H
