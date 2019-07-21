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

        print(self.defs)

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

    def __init__(self, bus: can.Bus):
        self.bus = bus

    def send_msg(self, bx_id: int, data: bytes = b''):
        '''Sends a message to the bus with the given bxCAN id and payload.'''
        msg = can.Message(arbitration_id=(bx_id >> 3), is_extended_id=True, data=data)
        log.info(f'> {fmt_msg(msg)}')
        self.bus.send(msg)

    def on_error(self, exc: Exception):
        raise exc

    def on_message_received(self, msg: can.Message):
        log.info(f'< {fmt_msg(msg)}')

    def stop(self):
        log.info('TesterListener stopped')



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
