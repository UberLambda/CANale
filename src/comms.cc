// CANale/src/comms.cc - Implementation of CANale/src/comms.hh
//
// Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "comms.hh"

#include <QFuture>

extern "C"
{
#include "common/can_msgs.h"
}
#include "util.hh"
#include "moc_comms.cpp"

namespace ca
{


/// Translates a bxCAN-formatted CAN EID to QtCanBus' EID format.
inline static quint32 translateEID(const uint32_t cnAddr)
{
    return cnAddr >> 3;
}

/// Masks a CANnuccia device id into the given bxCAN-formatted CAN EID, then
/// translates it to  QtCanBus' EID format.
inline static quint32 translateEID(const uint32_t cmd, const uint8_t devId)
{
    return translateEID(cnCANDevMask(cmd, devId));
}

/// Translates a QtCanBus EID to a bxCAN-formatted one;
/// returns a <bxCAN ID, device id> pair.
inline static std::tuple<uint32_t, uint8_t> untranslateEID(const quint32 qtAddr)
{
    uint32_t eid = (qtAddr << 3) | 0x00000004u;
    return {eid, (eid & 0x00000FF0u) >> 4};
}

#define EXPECT_CAN() do { Q_ASSERT(*this); if(!*this) { return; } } while(false)


Comms::Comms(QObject *parent)
    : QObject(parent), m_can(nullptr)
{
}

Comms::~Comms() = default;


void Comms::sendSelectPageCmd(DevId devId, uint32_t pageAddr)
{
    quint32 msgId = translateEID(CN_CAN_MSG_PROG_DONE, devId);
    QByteArray payload(4, 0);
    writeU32LE(reinterpret_cast<uint8_t *>(payload.data()), pageAddr);
    m_can->writeFrame(QCanBusFrame(msgId, payload));
}

void Comms::sendPageWriteCmds(DevId devId, QByteArray pageData)
{
    quint32 msgId = translateEID(CN_CAN_MSG_WRITE, devId);

    // Send writes in blocks of 8 bytes
    int i;
    for(i = 0; i < pageData.size(); i += 8)
    {
        m_can->writeFrame(QCanBusFrame(msgId, pageData.mid(i, 8)));
    }

    // Send any leftovers
    int left = pageData.size() - i;
    if(left > 0)
    {
        m_can->writeFrame(QCanBusFrame(msgId, pageData.mid(i, left)));
    }
}

void Comms::selectNextPageToFlash(DevId devId)
{
    DeviceState &devState = m_deviceStates[devId];
    for(auto it = devState.pageFlashData.begin(); it != devState.pageFlashData.end(); it ++)
    {
        uint32_t nextPageAddr = it->first;
        if(nextPageAddr != devState.selPageAddr)
        {
            sendSelectPageCmd(devId, nextPageAddr);
            break;
        }
    }
}


void Comms::progStart(DevId devId)
{
    EXPECT_CAN();

    // progStart(): [PROG_REQ] -> PROG_REQ_RESP -> UNLOCK -> UNLOCKED
    quint32 msgId = translateEID(CN_CAN_MSG_PROG_REQ, devId);
    m_can->writeFrame(QCanBusFrame(msgId, {}));
}

void Comms::progEnd(DevId devId)
{
    EXPECT_CAN();

    // progEnd(): [PROG_DONE] -> PROG_DONE_ACK
    quint32 msgId = translateEID(CN_CAN_MSG_PROG_DONE, devId);
    m_can->writeFrame(QCanBusFrame(msgId, {}));
}

void Comms::flashPage(DevId devId, uint32_t pageAddr, QByteArray pageData)
{
    Q_ASSERT(pageAddr != DeviceState::NO_PAGE); // (reserved value)

    // Add/replace the writes to this flash page on this device
    DeviceState &devState = m_deviceStates[devId];
    devState.pageFlashData[pageAddr] = pageData;

    // If no page is currently being flashed, select the page to be written now
    if(devState.selPageAddr == DeviceState::NO_PAGE)
    {
        // flashPage(): [SELECT_PAGE] -> PAGE_SELECTED -> WRITE... & CHECK_WRITES
        //              -> WRITES_CHECKED -> COMMIT_WRITES -> WRITES_COMMITTED
        sendSelectPageCmd(devId, pageAddr);
    }

    // When any page is selected a `PAGE_SELECTED` message will be received and
    // the flashing operation will continue.
}

void Comms::framesReceived()
{
    EXPECT_CAN();

    QCanBusFrame frame;
    while((frame = m_can->readFrame()).isValid())
    {
        auto eidPair = untranslateEID(frame.frameId());

        uint32_t msg = std::get<0>(eidPair) & CN_CAN_MSGID_MASK;
        DevId devId = std::get<1>(eidPair);
        DeviceState &devState = m_deviceStates[devId];

        switch(msg)
        {

        case CN_CAN_MSG_PROG_REQ_RESP:
        {
            // progStart(): PROG_REQ -> [PROG_REQ_RESP -> UNLOCK] -> UNLOCKED
            //
            // Expected payload format:
            // - pageSizePow2: U8
            // - pageCount: U16 LE
            // - elfMachine: U16 LE
            if(frame.payload().size() != 5)
            {
                // Broken payload!
                // TODO: Log this as an error?
                break;
            }
            auto payload = reinterpret_cast<uint8_t *>(frame.payload().data());

            devState.stats.pageSize = (1 << uint32_t(payload[0]));
            devState.stats.nFlashPages = readU16LE(&payload[1]);
            devState.stats.elfMachine = readU16LE(&payload[3]);

            uint32_t unlockMsgId = translateEID(CN_CAN_MSG_UNLOCK, devId);
            m_can->writeFrame(QCanBusFrame(unlockMsgId, {}));

        } break;

        case CN_CAN_MSG_UNLOCKED:
            // progStart(): PROG_REQ -> PROG_REQ_RESP -> UNLOCK -> [UNLOCKED]
            //
            // Send out the device stats gathered at step 2/4
            // (They will be zero-initialized if no state was present for this device)
            emit progStarted(devId, devState.stats);
            break;

        case CN_CAN_MSG_PROG_DONE_ACK:
            // progEnd(): PROG_DONE -> [PROG_DONE_ACK]
            emit progEnded(devId);
            break;

        case CN_CAN_MSG_PAGE_SELECTED:
        {
            // flashPage(): SELECT_PAGE -> [PAGE_SELECTED -> WRITE... & CHECK_WRITES]
            //              -> WRITES_CHECKED -> COMMIT_WRITES -> WRITES_COMMITTED
            //
            // Expected payload format:
            // - pageAddr: U32 LE
            if(frame.payload().size() != 4)
            {
                // Broken payload, abort.
                // TODO: Log this as an error?
                break;
            }
            auto payload = reinterpret_cast<uint8_t *>(frame.payload().data());

            // Confirm the address of the page that is now selected
            devState.selPageAddr = readU32LE(payload);

            auto pageDataIt = devState.pageFlashData.find(devState.selPageAddr);
            if(pageDataIt != devState.pageFlashData.end())
            {
                // There is some data to be flashed to the currently-selected page;
                // send the WRITE commands
                sendPageWriteCmds(devId, pageDataIt->second);

                // Ask for a CRC16 of the WRITEs that were just sent. The device
                // should repond  with a WRITES_CHECKED when it's done computing
                // it
                uint32_t checkWritesMsg = translateEID(CN_CAN_MSG_CHECK_WRITES, devId);
                m_can->writeFrame(QCanBusFrame(checkWritesMsg, {}));
            }
            else
            {
                // A page was selected, but no data is to be written to it.
                // Just select the next page that is actually to be flashed
                // TODO: Log this as a warning?
                selectNextPageToFlash(devId);
            }

        } break;

        case CN_CAN_MSG_WRITES_CHECKED:
        {
            // flashPage(): SELECT_PAGE -> PAGE_SELECTED -> WRITE... & CHECK_WRITES
            //              -> [WRITES_CHECKED -> COMMIT_WRITES] -> WRITES_COMMITTED
            auto pageDataIt = devState.pageFlashData.find(devState.selPageAddr);
            if(pageDataIt == devState.pageFlashData.end())
            {
                // We received a CRC16 for a page but we don't think we have
                // asked for it to be flashed - otherwise we would have its data
                // at `pageFlashData[selPageAddr]`. This should likely never happen.
                // TODO: Log this as a warning?
                // Just select a page is actually to be flashed (if any)
                selectNextPageToFlash(devId);
                break;
            }

            // Get the CRC16 of the writes from the device. Expected payload format:
            // - crc16: U16 LE
            uint16_t recvdCRC;
            if(frame.payload().size() == 2)
            {
                auto payload = reinterpret_cast<uint8_t *>(frame.payload().data());
                recvdCRC = readU16LE(payload);
            }
            else
            {
                // Broken payload, this should never happen.
                // Set the "received" CRC to a weird value so that it hopefully
                // will never match with the locally-computed one
                // TODO: Log this as a warning?
                recvdCRC = 0xFFFFu;
            }

            // Calculate the CRC16 of the writes locally
            auto writtenData = reinterpret_cast<const uint8_t *>(pageDataIt->second.data());
            auto writtenDataLen = static_cast<unsigned long>(pageDataIt->second.size());
            uint16_t expectedCRC = crc16(writtenDataLen, writtenData);

            if(recvdCRC == expectedCRC)
            {
                // CRC matches, commit the writes to the page
                uint32_t commitWritesMsg = translateEID(CN_CAN_MSG_COMMIT_WRITES, devId);
                m_can->writeFrame(QCanBusFrame(commitWritesMsg, {}));
            }
            else
            {
                // CRC mismatch, don't commit writes
                emit pageFlashErrored(devId, devState.selPageAddr, expectedCRC, recvdCRC);

                // Give up on writing this page and SELECT_PAGE the next one to
                // be flashed (if any)
                devState.pageFlashData.erase(devState.selPageAddr);
                devState.selPageAddr = DeviceState::NO_PAGE;
                selectNextPageToFlash(devId);
            }

        } break;

        case CN_CAN_MSG_WRITES_COMMITTED:
        {
            // flashPage(): SELECT_PAGE -> PAGE_SELECTED -> WRITE... & CHECK_WRITES
            //              -> WRITES_CHECKED -> COMMIT_WRITES -> [WRITES_COMMITTED]
            //
            // Get the address of the committed page. Expected payload format:
            // - pageAddr: U32 LE
            uint32_t pageAddr;
            if(frame.payload().size() == 4)
            {
                auto payload = reinterpret_cast<uint8_t *>(frame.payload().data());
                pageAddr = readU32LE(payload);
            }
            else
            {
                // Broken payload, this should definitely never happen.
                // FIXME IMPORTANT: Handle this situation better and at least log an error
                Q_ASSERT(false && "Broken WRITES_COMMITTED payload");

                // We don't know what page the writes were committed to, so we
                // guess we have committed writes to the currently-selected page.
                // !! If this assumption is wrong flashing will likely fail /   !!
                // !! never end for two pages - the currently-selected page and !!
                // !! the page that writes were actually committed to!          !!
                pageAddr = devState.selPageAddr;
            }

            emit pageFlashed(devId, pageAddr);

            // This page has now be written to; remove it from queue of pages to
            // write and SELECT_PAGE the next one to be flashed (if any)
            devState.pageFlashData.erase(devState.selPageAddr);
            devState.selPageAddr = DeviceState::NO_PAGE;
            selectNextPageToFlash(devId);

        } break;

        default:
            // Ignored CAN message
            break;
        }
    }
}


}
