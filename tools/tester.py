#!/usr/bin/env python3
# coding: utf-8

# CANale/tools/tester.py - CANnuccia device simulator (for testing CANale)
#
# Copyright (c) 2019, Paolo Jovon <paolo.jovon@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os, sys
import re
import time
import struct
import argparse
import binascii
import textwrap
import logging as log
from typing import Any
import can  # $ pip3 install python-can


log.basicConfig(level=log.INFO, stream=sys.stderr, format='[%(asctime)s] %(message)s')


class CanMsgs:
    '''Parses MSG_ID #defines from CANnuccia's `can_msgs.h`.'''
    CANNUCCIA_ROOT = os.path.join(os.path.dirname(__file__), '..', 'src', '3rdparty', 'CANnuccia')
    CANNUCCIA_MSGS_H_PATH = os.path.join(CANNUCCIA_ROOT, 'src', 'common', 'can_msgs.h')

    def __init__(self):
        def_re = re.compile(r'^\s*#\s*define\s+CN_CAN_(?P<name>[A-Z0-9_]+)\s+(?P<value>.+)(?<!\s)', re.MULTILINE)
        num_re = re.compile(r'((0[xXbB]?)?[A-Fa-f0-9]+)[lLuUf]+')

        self.defs = {}
        with open(self.CANNUCCIA_MSGS_H_PATH, 'r') as msgs_h:
            for def_match in def_re.finditer(msgs_h.read()):
                name = def_match.group('name')

                value_str = def_match.group('value')
                # Remove L, U, f numeric literal prefixes that Python doesn't recognize
                value_str = num_re.sub(r'\1', value_str)
                # Evaluate expression (given the previously-read #defines)
                value = eval(value_str, self.defs)

                self.defs[name] = value

    def __getattr__(self, name: str) -> int:
        return self.defs[name]

CAN = CanMsgs()


def bxcan_id(id: int) -> int:
    '''Converts a can EID to bxCAN format.'''
    return (id << 3)


def fmt_msg(msg: can.Message):
    '''Formats a CANnuccia message.'''
    eid = bxcan_id(msg.arbitration_id)
    eid_str = hex(eid).upper()[2:]
    data_str = binascii.hexlify(msg.data).decode('utf-8').upper()
    data_str = '[' + ' '.join(textwrap.wrap(data_str, 2)) + ']'
    return f'{eid_str} {data_str}'


class TesterListener(can.Listener):
    '''Simulates a CANnuccia device on a CAN bus'''

    def __init__(self, bus: can.Bus,
        page_size: int = 1024, num_pages: int = 128, elf_machine: int = 83):

        self.bus = bus

        self.page_size = page_size
        '''The size in bytes of the flash pages in the simulated devices'''
        self.num_pages = num_pages
        '''The total number of pages of size `page_size` in the simulated devices'''
        self.elf_machine = elf_machine
        '''The ELF machine type of the simulated devices'''

        # Bind all `self.handle_<message name>` methods to the respective CAN message being received
        self.msg_handlers = {}
        handler_prefix = 'handle_'
        for handler_name, handler in {k: getattr(self, k) for k in dir(self) if k.startswith(handler_prefix)}.items():
            msg_name = f'MSG_{handler_name[len(handler_prefix):].upper()}'
            msg_id = getattr(CAN, msg_name, None)

            if msg_id is not None:
                self.msg_handlers[msg_id] = handler
            else:
                raise UserWarning(f'TesterListener.{handler_name}() present but no relative CAN.{msg_name} found')

    def send_msg(self, msg_type: int, dev_id: int, data: bytes = b''):
        '''Sends a message to the bus with the given CANnuccia message type, device id and payload.'''
        dev_id_mask = (dev_id & 0xFF) << 4;
        eid = (msg_type | dev_id_mask) >> 3

        msg = can.Message(arbitration_id=eid, is_extended_id=True, data=data)
        log.info(f'> {fmt_msg(msg)}')
        self.bus.send(msg)

    def on_error(self, exc: Exception):
        raise exc

    def on_message_received(self, msg: can.Message):
        msg_id = bxcan_id(msg.arbitration_id)
        msg_type = msg_id & 0xFFFFF000
        dev_id = (msg_id & 0x000000FF0) >> 4

        handler = self.msg_handlers.get(msg_type, None)
        if handler:
            log.info(f'< {fmt_msg(msg)}')
            handler(msg=msg, msg_type=msg_type, dev_id=dev_id)
        else:
            log.debug(f'Ignored: {str(msg)}')

    def stop(self):
        log.info('TesterListener stopped')


    # ----- Add `handle_<messagename>()` methods below -----

    def handle_prog_req(self, msg: can.Message, msg_type: int, dev_id: int):
        log.info(f'PROG_REQ for 0x{dev_id:X}')

        # Payload of a PROG_REQ_RESP:
        data = struct.pack('<BHH',
            self.page_size.bit_length() - 1,  # 1. log2(size of a flash page): U8
            self.num_pages,                   # 2. Total number of flash pages: U16 LE
            self.elf_machine,                 # 3. ELF machine type (e_machine): U16 LE
        )
        self.send_msg(CAN.MSG_PROG_REQ_RESP, dev_id, data)

    def handle_prog_done(self, msg: can.Message, msg_type: int, dev_id: int):
        log.info(f'DONE 0x{dev_id:X}')

        # FIXME: Set "done" flag for device

        self.send_msg(CAN.MSG_PROG_DONE_ACK, dev_id)

    def handle_unlock(self, msg: can.Message, msg_type: int, dev_id: int):
        log.info(f'UNLOCK {dev_id:X}')

        # FIXME: Set "unlocked" flag for device

        self.send_msg(CAN.MSG_UNLOCKED, dev_id)

    def handle_select_page(self, msg: can.Message, msg_type: int, dev_id: int):
        # Payload of a SELECT_PAGE:
        # 1. Address of the first byte of the page: U32 LE
        page_addr = struct.unpack('< L', msg.data)[0]

        log.info(f'SELECT_PAGE at 0x{page_addr:X} for 0x{dev_id:X}')

        # FIXME: Set addr of page selected for device

        # Payload of a PAGE_SELECTED:
        # 1. Address of the first byte of the page: U32 LE
        self.send_msg(CAN.MSG_PAGE_SELECTED, dev_id, msg.data)

    def handle_seek(self, msg: can.Message, msg_type: int, dev_id: int):
        # Payload of a SEEK:
        # 1. Offset in bytes into the page: U32 LE
        offset = struct.unpack('< L', msg.data)[0]

        # FIXME: Set page offset for device

    def handle_write(self, msg: can.Message, msg_type: int, dev_id: int):
        # Payload of a WRITE:
        # Bytes to write to the page: 1..8 U8

        # FIXME: Write data to temporary page
        pass

    def handle_check_writes(self, msg: can.Message, msg_type: int, dev_id: int):
        log.info(f'CHECK_WRITES for 0x{dev_id:X}')

        # FIXME: Calculate CRC of writes to temporary page
        crc = 0xFEFE

        # Payload of WRITES_CHECKED:
        data = struct.pack('< H',
            crc,  # 1. CRC16/xmodem of writes: U16 LE
        )
        self.send_msg(CAN.MSG_WRITES_CHECKED, dev_id, data)

    def handle_commit_writes(self, msg: can.Message, msg_type: int, dev_id: int):
        log.info(f'COMMIT_WRITES for 0x{dev_id:X}')

        # FIXME: Get the address of the page being written now
        page_addr = 0xFEFEAABB

        # Payload of WRITES_COMMITTED:
        data = struct.pack('< L',
            page_addr,  # 1. Address of written-to page: U32 LE
        )
        self.send_msg(CAN.MSG_WRITES_COMMITTED, dev_id, data)


def parse_args():
    '''Returns parsed command-line arguments.'''
    parser = argparse.ArgumentParser(description='Simulates CANnuccia devices over a CAN link',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-I', '--interface', default='socketcan', help='the python-can interface to use')
    parser.add_argument('-C', '--channel', default='vcan0', help='the python-can channel to use')
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()

    bus = can.Bus(interface=args.interface, channel=args.channel)
    notifier = can.Notifier(bus, [TesterListener(bus)])
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print('Quit')
