import binascii
import json
import socket
import struct
import sys
import time
import threading
from signal import signal, SIGINT
from sys import exit

#localIP     = "192.168.69.1"
#192.168.69.1 wlan
#172.16.0.2 eth0 

class Application:
    def __init__( self ):
        signal( SIGINT, lambda signal, frame: self.signalhandler() )
        self.killed = False
        self.buffer_size  = 1024
        self.remote_nodes =    {}
        
        self.discovery_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        self.discovery_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.discovery_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.discovery_socket.settimeout(0.1)
        
    ####
    def signalhandler(self):
        print("handle break")
        exit(0)

    ####
    def updateIpmDb(self, message, client):
        clientmsg = "Message from Client:{}".format(message)
        #print(clientmsg)
        #clientip  = "Client IP Address:{}".format(client)
        clientip = client[0]

        discovery_data = struct.unpack('B B B B I 96s', message) #https://docs.python.org/3/library/struct.html
        version = discovery_data[4]
        tokens = discovery_data[5].split(b'\0')
        #i=0
        #for name in tokens:
        #	print(str(i) + str(" ") + str(name))
        #	i=i+1
        serialnum = tokens[0]
        name = tokens[31]

        self.remote_nodes[clientip] = {
             'serailnum':serialnum,
            'name':name,
            'ip':clientip
        }
        print(self.remote_nodes)

    def sendDiscoveryMessage(self, discovery_socket):
        #remote_network   = ("255.255.255.255", 5555)
        #remote_network   = ("172.16.0.255", 5555)
        #remote_network   = ("172.16.0.255", 5555)
        #remote_network   = ("10.140.243.255", 5555)
        remote_network        = ("192.168.1.255", 5555)
        values = (0xca, 0xfe, 0xba, 0xbe, 1, b'')
        packer = struct.Struct('B B B B I 96s') #https://docs.python.org/3/library/struct.html
        packed_data = packer.pack(*values)
        self.discovery_socket.sendto(packed_data, remote_network)
        print("sent to " + remote_network[0])

        
    def MainLoop( self ):
        while(self.killed == False):
            bytesAddressPair = ('',0)
            
            #discovery
            self.sendDiscoveryMessage(self.discovery_socket)
            try:
                bytesAddressPair = self.discovery_socket.recvfrom(self.buffer_size)
            except socket.timeout:
                print("timedout")
                time.sleep(1)
                continue
            self.updateIpmDb(bytesAddressPair[0], bytesAddressPair[1])
            time.sleep(1)

###
app = Application()
app.MainLoop()

print( "The app is terminated, exiting ..." )

