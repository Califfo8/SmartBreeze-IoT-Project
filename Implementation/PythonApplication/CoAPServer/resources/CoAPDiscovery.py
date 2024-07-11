from coapthon.resources.resource import Resource
from coapthon.client.helperclient import HelperClient
from coapthon import defines
import coapthon.defines as defines
from configparser import ConfigParser
from Utility.DBAccess import DBAccess
from Utility.Log import Log

class CoAPDiscovery(Resource):

    def __init__(self, name="CoAP Discovery"):
        super(CoAPDiscovery, self).__init__(name, observable=False)
        self.payload = ""

    def check_resource(self, host, port, resource):
        """Check if the resource is available in the node and if the node is active"""
        client = HelperClient(server=(host, port))
        response = client.get(resource)
        client.stop()
        if response is None or response.code != defines.Codes.CONTENT.number:
            return False
        else:
            return True
            
    
    def render_GET(self, request):
        res = CoAPDiscovery()
        res.location_query = request.uri_query
        log = Log("CoAPObserver", request.payload).get_logger()
        #Verify the JSON payload
        log.info("Requested ip resource: {}".format(request.payload))

        # Search the resource in the database
        configur = ConfigParser()
        configur.read('./CoAPServer/config.ini')
        database = DBAccess(
            host = configur.get('mysql', 'host'),
            user = configur.get('mysql', 'user'),
            password = configur.get('mysql', 'password'),
            database = configur.get('mysql', 'database'),
            log=log)
        
        if database.connect() is None:
            return None
        
        query = "SELECT COUNT(*) FROM nodes WHERE resource = %s"
        val = (request.payload,)

        node_ip = database.query(query, val, True)
        if node_ip is None:
            database.close()
            return None
        
        elif node_ip[0][0] <=0:
            log.error("Resource not found:{}".format(node_ip[0][0]))
            return None
        # If the resource is found, return the ip address
        query = "SELECT ip FROM nodes WHERE resource = %s"
        node_ip = database.query(query, val, True)
        if node_ip is None:
            database.close()
            return None
        
        # Ping the node to check if it is active
        log.info("Checking resource: {} at {}:{}: ".format(request.payload, request.payload, 5683))
        if not self.check_resource(node_ip[0][0], 5683, request.payload):
            log.info("Resource not found: node or resource is inactive. Updating the database")
            query = "UPDATE nodes SET Status ='DEACTIVATED'  WHERE resource = %s"
            node_ip = database.query(query, val, True)
            database.close()
            return None
        
        # Close the database connection
        database.close()

        log.info("Resource found at: {}, sending to request".format(node_ip[0][0]))
        res.payload = node_ip[0][0]
        
        return res