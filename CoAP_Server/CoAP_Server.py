#!/usr/bin/env python

import getopt
import sys
import json
from coapthon.server.coap import CoAP
from coapthon import defines
from resources.NodeRegistration import NodeRegistration
from resources.CoAPDiscovery import CoAPDiscovery
class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port), multicast=False)
        self.add_resource("/registration", NodeRegistration())
        self.add_resource("/discovery", CoAPDiscovery())
        print("CoAP Server started on {}:{}".format(host, port))

if __name__ == '__main__':
    ip = "fd00::1"
    port = 5683
    server = CoAPServer(ip, port)

    try:
        print("CoAP Server Started")
        server.listen(10)
    except KeyboardInterrupt:
        print("Server Shutdown")
        server.close()
        print("Exiting...")



