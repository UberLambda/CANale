// CANale/src/util.hh - Misc. utilities
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef UTIL_HH
#define UTIL_HH

#include <cstdint>

namespace ca
{

/// Calculates the CRC16/XMODEM of some data.
///
/// (Ported from CANnuccia/src/stm32/util.c)
inline uint16_t crc16(unsigned long len, const uint8_t data[])
{
    constexpr uint16_t CRC16_INITVAL = 0x0000;
    constexpr uint16_t CRC16_POLYNOMIAL = 0x1021;

    // CRC16/XMODEM. See: http://mdfs.net/Info/Comp/Comms/CRC16.htm
    // NOTE: STM32's hardware CRC module can only calculate CRC32/Ethernet so
    //       it can't be used for this CRC16 :(
    // NOTE: int is 32-bit so masking the lowest 16 bits is needed. It also
    //       likely is faster to work on vs. uint16_t
    int crc = CRC16_INITVAL;
    for(const uint8_t *it = data; it < (data + len); it ++)
    {
        crc ^= (*it << 8);
        for(int i = 0; i < 8; i ++)
        {
            crc <<= 1;
            if(crc & 0x10000)
            {
                crc = (crc ^ CRC16_POLYNOMIAL) & 0xFFFF;
            }
        }
    }
    return uint16_t(crc);
}

}

#endif // UTIL_HH
