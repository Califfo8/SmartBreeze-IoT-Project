from coapthon.resources.resource import Resource
from coapthon.client.helperclient import HelperClient
from coapthon import defines
import json
from jsonschema import validate, ValidationError
from DBAccess import DBAccess
import coapthon.defines as defines

class CoAPDiscovery(Resource):

    def __init__(self, name="CoAP Discovery"):
        super(CoAPDiscovery, self).__init__(name, observable=False)
        self.payload = ""    
    
    def check_resource(self, host, port, resource):
        """Check if the resource is available in the node and if the node is active"""
        print("\t Checking resource: {} at {}:{}".format(resource, host, port))
        client = HelperClient(server=(host, port))
        response = client.get(resource)
        client.stop()
        print("\t\t Response: {}".format(response.payload))
        if response is None or response.code != defines.Codes.CONTENT.number:
            return False
        else:
            return True
            
    
    def render_GET(self, request):
        res = CoAPDiscovery()
        res.location_query = request.uri_query
        #Verify the JSON payload
        print("[UP] Requested ip resource: {}".format(request.payload))

        # Search the resource in the database
        database = DBAccess(
            host="localhost",
            user="root",
            password="PASSWORD",
            database="SmartBreezeDB")
        
        if database.connect() is None:
            return None
        
        query = "SELECT COUNT(*) FROM nodes WHERE resource = %s AND status = 'ACTIVE'"
        val = (request.payload,)
        node_ip = database.query(query, val, True)
        
        if node_ip is None:
            return None
        elif node_ip[0][0] <=0:
            print("\t Resource not found:{}".format(node_ip[0][0]))
            return None
        # If the resource is found, return the ip address
        query = "SELECT ip FROM nodes WHERE resource = %s AND status = 'ACTIVE'"
        node_ip = database.query(query, val, True)
        
        # Ping the node to check if it is active
        if not self.check_resource(node_ip[0][0], 5683, request.payload):
            print("\t Resource not found: node or resource is inactive. Updating the database")
            query = "UPDATE nodes SET Status ='DEACTIVATED'  WHERE resource = %s"
            return None
        # Close the database connection
        database.close()

        print("\t Resource found at: {}, sending to request".format(node_ip[0][0]))
        res.payload = node_ip[0][0]
        return res