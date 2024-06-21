#!/usr/bin/env python
from coapthon.server.coap import CoAP
from CoAPServer.resources.NodeRegistration import NodeRegistration
from CoAPServer.resources.CoAPDiscovery import CoAPDiscovery

class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port), multicast=False)
        self.add_resource("/registration", NodeRegistration())
        self.add_resource("/discovery", CoAPDiscovery())
        print("CoAP Server started on {}:{}".format(host, port))
    
    





