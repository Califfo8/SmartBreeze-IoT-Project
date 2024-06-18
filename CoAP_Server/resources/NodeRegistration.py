from coapthon.resources.resource import Resource
import json
from jsonschema import validate, ValidationError
import mysql.connector
import coapthon.defines as defines
class DBAccess():
    def __init__(self):
        self.host = "localhost"
        self.user = "root"
        self.password = "PASSWORD"
        self.database = "SmartBreezeDB"

SBDB = DBAccess()
json_register_schema = {
    "node" : {"type" : "string"},
    "resource" : {"type" : "string"}
}
class NodeRegistration(Resource):

    def __init__(self, name="Node Registration"):
        super(NodeRegistration, self).__init__(name)
        self.payload = "Registered"    

    def render_POST(self, request):
        res = NodeRegistration()
        res.location_query = request.uri_query
        #Verify the JSON payload
        try:
            node_info = json.loads(request.payload)
            validate(instance=node_info, schema=json_register_schema)
        except ValidationError as e:
            print("[ERROR] JSON is not valid: ", e.message)
            print("\t - BAD JSON:", node_info)
            return None
        except:
            print("[ERROR] BAD PAYLOAD:", request.payload)
            return None
        
        print("[UP] Node Registration POST: {}".format(node_info["node"]))
        # Insert the node information into the database
        query = "REPLACE INTO nodes (ip, name ,resource, status) VALUES (%s, %s, %s, %s)"
        val = (request.source[0], node_info["node"], str(node_info["resource"]), 'ACTIVE')
        try:
            # Connect to the MySQL database
            mydb = mysql.connector.connect(
                host=SBDB.host,
                user=SBDB.user,
                password=SBDB.password,
                database=SBDB.database
            )
        except:
            print("[ERROR] Could not connect to the database")
            return None

        try:
            # Create a cursor object to execute SQL queries
            mycursor = mydb.cursor()
            # Execute the SQL query
            mycursor.execute(query, val)
        except:
            print("[ERROR] Could not execute the query")
            mydb.close()
            return None
        # Close the database connection
        mydb.commit()
        mydb.close()
        print("[UP] Inserted node information into the database")

        return res