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
import time
import argparse
import can  # $ pip3 install python-can

def parse_args():
    '''Returns parsed command-line arguments.'''
    parser = argparse.ArgumentParser(description='Simulates CANnuccia devices over a CAN link',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-I', '--interface', default='socketcan', help='the python-can interface to use')
    parser.add_argument('-C', '--channel', default='vcan0', help='the python-can channel to use')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()

    link = can.Bus(interface=args.interface, channel=args.channel)

    # FIXME Test!
    while True:
        msg = can.Message(is_extended_id=True, arbitration_id=(0xCA000000 >> 3),
                          data=b'TEST')
        link.send(msg)
        time.sleep(1)
