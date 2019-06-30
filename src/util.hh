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
#include <istream>
#include <streambuf>

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


/// Reads a little-endian U16 from 2 bytes.
///
/// (Ported from CANnuccia/src/common/util.h)
inline uint16_t readU16LE(const uint8_t bytes[])
{
    return bytes[0] | uint16_t(bytes[1] << 8);
}

/// Reads a little-endian U32 from 4 bytes.
///
/// (Ported from CANnuccia/src/common/util.h)
inline uint32_t readU32LE(const uint8_t bytes[])
{
    return bytes[0]
            | uint32_t(bytes[1] << 8)
            | uint32_t(bytes[2] << 16)
            | uint32_t(bytes[3] << 24);
}

/// Writes a little-endian U32 to 4 bytes.
///
/// (Ported from CANnuccia/src/common/util.h)
inline void writeU32LE(uint8_t outBytes[], uint32_t u32)
{
    outBytes[0] = (u32 & 0x000000FFu);
    outBytes[1] = (u32 & 0x0000FF00u) >> 8;
    outBytes[2] = (u32 & 0x00FF0000u) >> 16;
    outBytes[3] = (u32 & 0xFF000000u) >> 24;
}

/// A `std::streambuf` that operates on a memory block.
// See: https://stackoverflow.com/a/13059195, https://stackoverflow.com/a/46069245
class MemStreambuf : public std::streambuf
{
public:
    MemStreambuf(uint8_t *data, size_t size)
    {
        auto dataP = reinterpret_cast<char *>(data);
        setg(dataP, dataP, dataP + size);
    }

    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
    {
        return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode which = std::ios_base::in) override
    {
        (void)which;

        switch(dir)
        {
        case std::ios_base::beg:
            setg(eback(), eback() + off, egptr());
            break;

        case std::ios_base::cur:
            gbump(static_cast<int>(off));
            break;

        default: // std::ios_base::end
            setg(eback(), egptr() + off, egptr());
            break;
        }

      return gptr() - eback();
    }
};

/// A `std::istream` that reads from a memory block.
// See: https://stackoverflow.com/a/13059195
class MemIStream : virtual MemStreambuf, public std::istream
{
public:
    MemIStream (const uint8_t *data, size_t size)
        : MemStreambuf(const_cast<uint8_t *>(data), size),
          std::istream(static_cast<MemStreambuf *>(this))
    {
    }
};

}

#endif // UTIL_HH
