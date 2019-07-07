// CANale/src/elf.cc - Implementation of CANale/src/elf.hh 
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "elf.hh"

#include "util.hh"

namespace ca
{

void elfInfo(const ELFIO::elfio &elf, LogHandler &logger)
{
    logger(CA_DEBUG,
           QStringLiteral("ELF machine type: %1").arg(elf.get_machine()));
    logger(CA_DEBUG,
           QStringLiteral("ELF OS ABI: %1 (version %2)").arg(elf.get_os_abi(), elf.get_abi_version()));
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

}
