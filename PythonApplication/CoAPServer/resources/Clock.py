from coapthon.resources.resource import Resource
from Utility.Log import Log
from datetime import datetime

class Clock(Resource):

    def __init__(self, name="CoAP Discovery"):
        super(Clock, self).__init__(name, observable=False)
        self.payload = datetime.now().strftime("%Y-%m-%dT%H:%MZ")         
    
    def render_GET(self, request):
        res = Clock()
        res.location_query = request.uri_query
        log = Log("Clock", request.payload).get_logger()
        res.payload = datetime.now().strftime("%Y-%m-%dT%H:%MZ")
        log.info("Request time by {} sending: {}".format(request.source[0], res.payload))
        return res