#!/usr/bin/env python
from coapthon.server.coap import CoAP
from CoAPServer.resources.NodeRegistration import NodeRegistration
from CoAPServer.resources.CoAPDiscovery import CoAPDiscovery
from CoAPServer.resources.Clock import Clock
from Utility.Log import Log

log_istance = Log(__name__, "EXP:Registration/Discovery")
log = log_istance.get_logger()

class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port), multicast=False)
        self.add_resource("/registration", NodeRegistration())
        self.add_resource("/discovery", CoAPDiscovery())
        self.add_resource("/clock", Clock())
        log.info("CoAP Server started on {}:{}".format(host, port))
    
    





