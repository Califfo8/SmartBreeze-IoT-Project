from coapthon.resources.resource import Resource
import json
from jsonschema import validate, ValidationError
from Utility.DBAccess import DBAccess
import coapthon.defines as defines
from configparser import ConfigParser
from CoAPServer.CoAPObserver import CoAPObserver
from coapthon.messages.response import Response
from Utility.Log import Log

log_istance = Log(__name__, "Unknow")
log = log_istance.get_logger()

class NodeRegistration(Resource):

    def __init__(self, name="Node Registration"):
        super(NodeRegistration, self).__init__(name, observable=False)
        self.payload = "Registered"
        
    def render_POST(self, request):
        res = NodeRegistration()
        json_register_schema = {
                    "node" : {"type" : "string"},
                    "resource" : {"type" : "string"},
                    "settings" : {"type" : "string"}
                    }   

        res.location_query = request.uri_query
        log.info("Node Registration POST request received from: {}".format(request.source[0]))
        #Verify the JSON payload
        try:
            node_info = json.loads(request.payload)
            validate(instance=node_info, schema=json_register_schema)
        except ValidationError as e:
            log.error("JSON is not valid: ", e.message)
            log.error("\t - BAD JSON:", node_info)
            return None
        except json.JSONDecodeError as e:
            log.error("Failed to decode JSON: {}".format(e.msg))
            log.error("Bad JSON: {}".format(request.payload))
            return None
        except Exception as e:
            log.error("BAD PAYLOAD:", e)
            return None
        log_istance.set_resource(node_info["node"])
        log.info("Node Registration POST source recognized")
        # Insert the node information into the database
        query = "REPLACE INTO nodes (ip, name ,resource, settings) VALUES (%s, %s, %s, %s)"
        val = (request.source[0], node_info["node"], str(node_info["resource"]), str(node_info["settings"]))
        
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
        
        ret = database.query(query, val, False)
        # Close the database connection
        database.close()
        # Check if the query was successful
        if ret is None:
            log.error("Failed to insert node information into the database")
            return None
        
        log.info("Inserted node information into the database")
        # Start observing the resource
        log.info("Starting observation of the resource: {} from {}".format(node_info["resource"], request.source[0]))
        observer = CoAPObserver(request.source[0], node_info["resource"])
        observer.start()
        log_istance.set_resource("Unknow")
        return res