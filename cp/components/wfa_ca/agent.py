#!/usr/bin/env python3
# encoding: utf8
#
# WTS CA Agent
#
# Copyright (c) BekenCorp. (chunjian.tian@bekencorp.com).  All rights reserved.
#
# This software may be distributed under the terms of the BSD license.
# See README for more details.
#
# wts agent

import serial
import threading
import socket
import argparse
import time

debug = True

class WtsCaAgent(object):
    '''
    WFA WTS Agent: forward TCP messages to serial port
    '''
    def __init__(self, uart_port='/dev/ttyUSB1', uart_timeout=0.1, tcp_port=9000):
        '''
        Construction Fucntion:
        '''
        self.uart_port = uart_port
        self.uart_timeout = uart_timeout
        self.tcp_port = tcp_port
        self.serial_rx_buf = b''
        self.tcp_client = None

    def serial_thread_handler(self):
        '''
        Read data from serial port, and forward data to WTS Tool.
        '''
        while True:
            data = self.ser.read(1024)
            if data:
                self.serial_rx_buf += data
                # if debug:
                #    print("Target  ->   Agent:\t", data)
            else:
                # 100ms timeout, assume all data are received
                if len(self.serial_rx_buf) > 0 and self.tcp_client:
                    if debug:
                        print('Agent   ->   Server:\t', self.serial_rx_buf)
                    self.tcp_client.send(self.serial_rx_buf)
                    # print(f'send done to {self.tcp_client}')
                    self.serial_rx_buf = b''

    def tcp_thread_handler(self):
        '''
        Receive WTS TCP request and forward to target by serial
        '''
        while True:
            # print(self.tcp_socket)
            conn, addr = self.tcp_socket.accept()
            new_thread = threading.Thread(target=self.tcp_conn_handler, args=(conn, addr))
            new_thread.start()

    def tcp_conn_handler(self, conn:socket.socket, addr):
        print(f"Connect by {addr}")
        self.tcp_client = conn
        while True :
            data = conn.recv(4096)
            if not data:
                conn.close()
                self.tcp_client = None
                return
            print("Server  ->   Agent:\t", data)
            self.ser.write(data)
            # print("Agent   ->   Target:\t", data)

    def run(self):
        # setup serial port
        self.ser = serial.Serial(self.uart_port, 115200, timeout=self.uart_timeout)

        # start tcp server
        tcp_server_address = ('',args.port)
        self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp_socket.bind(tcp_server_address)
        self.tcp_socket.listen(10)

        # create serial read thread
        self.serial_thread = threading.Thread(target=self.serial_thread_handler)

        # start TCP server read thread
        self.tcp_thread = threading.Thread(target=self.tcp_thread_handler)

        # kick thread run
        self.serial_thread.start()
        self.tcp_thread.start()

# parse commandline arguments
def parse_args():
    description = '''WFA WTS Control Agent.'''
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-d', '--device',
                        default='/dev/ttyUSB1',
                        help="Uart device, default /dev/ttyUSB1")
    parser.add_argument('-b', '--baudrate', type=int,
                        default=115200,
                        help="burn uart baudrate, defaults to 115200")
    parser.add_argument('-p', '--port', type=int,
                        default=9000,
                        help="TCP port, defaults to 9000")
    parser.add_argument('-t', '--timeout', type=float,
                        default=0.1,
                        help="UART timeout, defaults to 100ms")
    args = parser.parse_args()

    return args

# parse args
args = parse_args()

if __name__ == '__main__':
    agent = WtsCaAgent(args.device, args.timeout, args.port)
    agent.run()
