from coapthon.resources.resource import Resource
import json
from jsonschema import validate, ValidationError
from Utility.DBAccess import DBAccess
import coapthon.defines as defines
from configparser import ConfigParser
from CoAPServer.CoAPObserver import CoAPObserver
from Utility.Log import Log

class NodeRegistration(Resource):

    def __init__(self, name="Node Registration"):
        super(NodeRegistration, self).__init__(name)
        self.payload = "Registered"
        self.json_register_schema = {
                    "node" : {"type" : "string"},
                    "resource" : {"type" : "string"}
                    }   

    def render_POST(self, request):
        res = NodeRegistration()
        res.location_query = request.uri_query
        log = Log("NodeRegistration", request.payload).get_logger()
        #Verify the JSON payload
        try:
            node_info = json.loads(request.payload)
            validate(instance=node_info, schema=self.json_register_schema)
        except ValidationError as e:
            log.error("JSON is not valid: ", e.message)
            log.error("\t - BAD JSON:", node_info)
            return None
        except:
            log.error("[ERROR] BAD PAYLOAD:", request.payload)
            return None
        
        log.info("Node Registration POST: {}".format(node_info["node"]))
        # Insert the node information into the database
        query = "REPLACE INTO nodes (ip, name ,resource, status) VALUES (%s, %s, %s, %s)"
        val = (request.source[0], node_info["node"], str(node_info["resource"]), 'ACTIVE')
        
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
        
        database.query(query, val, False)
        # Close the database connection
        database.close()
        log.info("Inserted node information into the database")
        # Start observing the resource
        log.info("Starting observation of the resource: {} from {}".format(node_info["resource"], request.source[0]))
        observer = CoAPObserver(request.source[0], node_info["resource"])
        observer.start()

        return res