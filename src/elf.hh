// CANale/src/elf.hh - ELF interop utilities
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef ELF_HH
#define ELF_HH

#include <vector>
#include <map>
#include <QByteArray>
#include <elfio/elfio.hpp>
#include "types.hh"

namespace ca
{

using ElfioSegments = std::vector<ELFIO::segment *>;


/// Outputs core information about `elf` to `logger`.
void elfInfo(const ELFIO::elfio &elf, LogHandler &logger);

/// Appends to `segment` the list of segments in `elf` that will have to be flashed.
/// Outputs information on ELF segments to `logger`.
/// Returns the number of segments appended to `outSegments`.
///
/// Segments to be flashed have the PT_LOAD type and a `fileSize` greater
/// than zero; they will correspond to `fileSize` bytes to be written at
/// `physAddr` in the target device's flash.
unsigned listElfSegmentsToFlash(const ELFIO::elfio &elf, ElfioSegments &outSegments,
                                LogHandler &logger);


/// A flash map, mapping ELF segments to pages to be flashed.
class FlashMap
{
public:
    /// A page address, i.e. the address of the first byte of a page.
    using PageAddr = uint32_t;

    /// A page's contents.
    using PageData = QByteArray;

    /// Maps page addresses to page contents. Sorted by page address.
    using PageMap = std::map<PageAddr, PageData>;


    /// Constructs an empty flash map.
    FlashMap();

    /// Builds a flash map from a list of segments to flash and the size of a
    /// page on the target.
    ///
    /// All of the provided segments should be loadable, and they should not be
    /// destroyed until before `~FlashMap()` is run!
    FlashMap(const ElfioSegments &segments, size_t pageSize);
    ~FlashMap();

    FlashMap(const FlashMap &toCopy) = delete;
    FlashMap &operator=(const FlashMap &toCopy) = delete;
    FlashMap(FlashMap &&toMove) = default;
    FlashMap &operator=(FlashMap &&toMove) = default;

    /// The (page address -> data) map for pages still to be flashed.
    ///
    /// `erase()` them as they actually get flashed; when `pages()` is empty,
    /// flashing is done.
    inline PageMap &pages()
    {
        return m_pages;
    }
    inline const PageMap &pages() const
    {
        return m_pages;
    }

    /// Returns the number of pages in the map when the map was initially built.
    /// `pages().size() / numPages()` is the current flashing progress.
    inline size_t numPages() const
    {
        return m_numPages;
    }

private:
    size_t m_pageSize; ///< Size of a single flash page.
    size_t m_numPages; ///< Total number of pages to flash (at map build time).
    std::map<PageAddr, PageData> m_pages; ///< Pages still to be flashed.
};

}

#endif // ELF_HH
