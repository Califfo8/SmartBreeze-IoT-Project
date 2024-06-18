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

class CoAPDiscovery(Resource):

    def __init__(self, name="CoAP Discovery"):
        super(CoAPDiscovery, self).__init__(name, observable=False)
        self.payload = ""    

    def render_GET(self, request):
        res = CoAPDiscovery()
        res.location_query = request.uri_query
        #Verify the JSON payload
        print("[UP] Requested ip resource: {}".format(request.payload))

        # Search the resource in the database
        query = "SELECT COUNT(*) FROM nodes WHERE resource = %s"
        val = (request.payload,)
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
        except Exception as e:
            print("[ERROR] Could not execute the count query: {}".format(e))
            mydb.close()
            return None
        # take the result of the query and close the 
        node_ip = mycursor.fetchall()
        # Check if the resource was found
        if node_ip[0][0] <=0:
            print("[ERROR] Resource not found")
            return None
        # take the result of the query and close the connection
        query = "SELECT ip FROM nodes WHERE resource = %s"
        try:
            # Execute the SQL query
            mycursor.execute(query, val)
        except Exception as e:
            print("[ERROR] Could not execute the select query: {}".format(e))
            mydb.close()
            return None
        
        # return the result of the query
        node_ip = mycursor.fetchall()
        mydb.close()

        print("[UP] Resource found at: {}, sending to request".format(node_ip[0][0]))
        res.payload = node_ip[0][0]
        return res