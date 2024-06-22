from coapthon.client.helperclient import HelperClient
from Utility.DBAccess import DBAccess
from configparser import ConfigParser
from datetime import datetime
import coapthon.defines as defines
import json
import re
from Utility.Log import Log

class CoAPObserver:
    def __init__(self, ip, resource):
        self.client = HelperClient(server=(ip, 5683))
        self.resource = resource
        self.date_format = "%Y-%m-%dT%H:Z"
        self.log = Log("CoAPObserver", resource).get_logger()
        self.last_response = None

    def check_and_update(self, type, data, db):
        search_query = "SELECT COUNT(*) FROM solar_production WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s AND {} IS NULL".format(type)
        insert_query = "INSERT INTO solar_production ({}) VALUES (%s)".format(type)
        update_query = "UPDATE solar_production SET {} = %s WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s".format(type)
        index = 1
        if type == "predicted":
            index = 2    
        #Check if the sampled data is already present
        time = datetime.strptime(data[index]["t"], self.date_format)
        val = (time.year, time.month, time.day, time.hour)
        result = db.query(search_query, val, True)
        #If the data is present, update it
        if result[0][0] != 0:
            val = (data[index]["v"], time.year, time.month, time.day, time.hour)
            db.query(update_query, val, False)
        else:# Otherwise insert the data
            val = (data[index]["v"])
            db.query(insert_query, val, False)

    def callback_observe(self, response):
        self.log.info("Callback Observe {}".format(self.resource))
        configur = ConfigParser()
        configur.read('./CoAPServer/config.ini')
        database = DBAccess(
            host = configur.get('mysql', 'host'),
            user = configur.get('mysql', 'user'),
            password = configur.get('mysql', 'password'),
            database = configur.get('mysql', 'database'),
            log=self.log)
        if database.connect() is None:
            return None
        
        if response is None or response.code != defines.Codes.CONTENT.number:
            self.log.error("Response is None or not content")
            return None

        try:
            data = json.loads(response.payload)
        except Exception as e:
            self.log.error("BAD JSON:", e)
            return None

        if data[1]['v'] == -1:
            self.log.info("Data recieved is -1, skipping...")
            return None

        if self.resource == 'solar_energy':
            
            self.check_and_update("sampled", data, database)
            self.check_and_update("predicted", data, database)
            self.log.info("Updated solar energy production")

        elif self.resource == 'temperature_HVAC':
            self.query = "INSERT INTO temperature ( temperature, active_HVAC) VALUES (%s, %s)"
            val = (data[1]["v"], data[2]["v"])
            database.query(self.query, val, False)
            self.log.info("Updated temperature")
        
        self.last_response = response

    def start(self):
        self.client.observe(self.resource, self.callback_observe)
    
    def stop(self):
        self.client.cancel_observing(self.last_response)
        self.client.stop()