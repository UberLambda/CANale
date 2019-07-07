// CANale/src/elf.hh - ELF interop utilities
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef ELF_HH
#define ELF_HH

#include <elfio/elfio.hpp>
#include <elfio/elf_types.hpp>
#include "types.hh"

namespace ca
{

/// Outputs core information about `elf` to `logger`.
void elfInfo(const ELFIO::elfio &elf, LogHandler &logger);

/// Appends to `segment` the list of segments in `elf` that will have to be flashed.
/// Outputs information on ELF segments to `logger`.
/// Returns the number of segments appended to `outSegments`.
///
/// Segments to be flashed have the PT_LOAD type and a `fileSize` greater
/// than zero; they will correspond to `fileSize` bytes to be written at
/// `physAddr` in the target device's flash.
unsigned listElfSegmentsToFlash(const ELFIO::elfio &elf, std::vector<ELFIO::segment *> &outSegments,
                                LogHandler &logger);

}

#endif // ELF_HH
