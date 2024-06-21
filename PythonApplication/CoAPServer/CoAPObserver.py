from coapthon.client.helperclient import HelperClient
from Utility.DBAccess import DBAccess
from configparser import ConfigParser
from datetime import datetime
import json
class CoAPObserver:
    def __init__(self, ip, resource):
        self.uri = "coap://[{}]:5683/{}".format(ip, resource)
        self.client = HelperClient(server=(ip, 5683))
        self.resource = resource
        self.date_format = "%Y-%m-%dT%H:Z"
        
    def callback(self, response):
        configur = ConfigParser()
        configur.read('./CoAPServer/config.ini')
        database = DBAccess(
            host = configur.get('mysql', 'host'),
            user = configur.get('mysql', 'user'),
            password = configur.get('mysql', 'password'),
            database = configur.get('mysql', 'database'))
        if database.connect() is None:
            return None
        
        try:
            data = json.loads(response.payload)
        except:
            print("[ERROR] BAD JSON:", response.payload)
            return None
        print("[UP] Received data from {}: {}".format(self.uri, data))

        if self.resource == 'solar_energy':
            # Insert the sampled data
            query = "INSERT INTO solar_production (sampled, predicted) VALUES (%f, %s)"
            val = ( data[1]["v"], "NULL")
            database.query(query, val, False)

            # Update the predicted data
            predic_time = datetime.strptime(data[2]["t"], self.date_format)
            query = "SELECT COUNT(*) FROM solar_production WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s"
            val = (predic_time.year, predic_time.month, predic_time.day, predic_time.hour)

            query = "UPDATE INTO solar_production (predicted) VALUES (%s) WHERE timestamp = %s"
            val = (data[2]["v"], data[2]["t"])
            database.query(query, val, False)

        elif self.resource == 'temperature_HVAC':
            self.query = "INSERT INTO temperature ( temperature, active_HVAC) VALUES (%f, %d)"
            val = (data[1]["v"], data[2]["v"])
            database.query(self.query, val, False)

    def start(self):
        self.client.observe(self.uri, self.callback)
    
    def stop(self):
        self.client.cancel_observing(self.uri)
        self.client.stop()
        print("[DOWN] Stopped observing {}".format(self.uri))