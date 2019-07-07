// CANale/src/elf.cc - Implementation of CANale/src/elf.hh 
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "elf.hh"

#include "util.hh"
#include <elfio/elf_types.hpp>

namespace ca
{

void elfInfo(const ELFIO::elfio &elf, LogHandler &logger)
{
    logger(CA_DEBUG,
           QStringLiteral("ELF machine type: %1").arg(elf.get_machine()));

    QString abiStr;
    if(elf.get_os_abi() != ELFOSABI_NONE)
    {
        abiStr = QStringLiteral("%1 (version %2)").arg(elf.get_os_abi()).arg(elf.get_abi_version());
    }
    else
    {
        abiStr = QStringLiteral("none");
    }
    logger(CA_DEBUG, QStringLiteral("ELF OS ABI: %1").arg(abiStr));
}

unsigned listElfSegmentsToFlash(const ELFIO::elfio &elf, std::vector<ELFIO::segment *> &outSegments,
                                LogHandler &logger)
{
    logger(CA_DEBUG, QStringLiteral("%1 ELF segments:").arg(elf.segments.size()));

    QString segmMsg;
    unsigned nOutput = 0;
    for(unsigned i = 0; i < elf.segments.size(); i ++)
    {
        ELFIO::segment *segm = elf.segments[i];

        segmMsg = QStringLiteral("> segment %1: ").arg(i);

        if(segm->get_type() & PT_LOAD)
        {
            if(segm->get_file_size() > 0)
            {
                segmMsg += QStringLiteral("loadable, flash fileSize=%1B (out of memSize=%2B) at physAddr=%3")
                            .arg(segm->get_file_size())
                            .arg(segm->get_memory_size())
                            .arg(hexStr(segm->get_physical_address(), 8));

                outSegments.push_back(segm);
                nOutput ++;
            }
            else
            {
                segmMsg += QStringLiteral("loadable but has fileSize=0B, skip");
            }
        }
        else
        {
            segmMsg += "not loadable, skip";
        }
        logger(CA_DEBUG, segmMsg);
    }

    return nOutput;
}

FlashMap::FlashMap()
    : m_pageSize(0), m_numPages(0), m_pages{}
{
}

FlashMap::FlashMap(const ElfioSegments &segments, size_t pageSize)
    : m_pageSize(pageSize)
{
    Q_ASSERT(pageSize != 0);

    for(ELFIO::segment *segm : segments)
    {
        Q_ASSERT((segm->get_type() & PT_LOAD) && "Segment not loadable");

        // All data for the segment that comes from the ELF file is to be flashed
        auto startAddr = static_cast<PageAddr>(segm->get_physical_address());
        auto nSegmPages = static_cast<size_t>(segm->get_file_size() / pageSize); // (truncated)

        // Reference (do not copy) the data in the ELF segment for all entire pages we can read from it
        size_t segmOffset = 0;
        for(size_t i = 0; i < nSegmPages; i ++)
        {
            PageAddr pageAddr = startAddr + static_cast<PageAddr>(segmOffset);
            m_pages[pageAddr] = QByteArray::fromRawData(segm->get_data() + segmOffset,
                                                        static_cast<int>(pageSize));
            segmOffset += pageSize;
        }

        // If any data is left over, copy it to a new page and zero-fill the remaining bytes
        size_t left = segm->get_file_size() - segmOffset;
        if(left > 0)
        {
            PageAddr pageAddr = startAddr + static_cast<PageAddr>(segmOffset);
            auto &page = m_pages[pageAddr];
            page = QByteArray(segm->get_data() + segmOffset, static_cast<int>(left));
            page.append(static_cast<int>(pageSize - left), 0);
        }
    }

    m_numPages = m_pages.size();
}

FlashMap::~FlashMap() = default;

}
