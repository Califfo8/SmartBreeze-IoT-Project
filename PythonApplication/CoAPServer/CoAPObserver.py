from coapthon.client.helperclient import HelperClient
from Utility.DBAccess import DBAccess
from configparser import ConfigParser
from datetime import datetime
import coapthon.defines as defines
import json
import re
from Utility.Log import Log
log_istance = Log(__name__, "Unknow")
log = log_istance.get_logger()

class CoAPObserver:
    def __init__(self, ip, resource):
        self.client = HelperClient(server=(ip, 5683))
        self.resource = resource
        self.date_format = "%Y-%m-%dT%H:Z"
        self.last_response = None

    def check_and_update(self, type, data, db):
        search_query = "SELECT COUNT(*) FROM solar_production WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s AND {} IS NULL".format(type)
        insert_query = "INSERT INTO solar_production (timestamp,{}) VALUES (%s,%s)".format(type)
        update_query = "UPDATE solar_production SET {} = %s WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s".format(type)
        index = 1
        if type == "predicted":
            index = 2    
        #Check if the sampled data is already present
        time = datetime.strptime(data[index]["t"], self.date_format)
        db_timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        val = (time.year, time.month, time.day, time.hour)
        result = db.query(search_query, val, True)
        if result is None:
            log.error("Failed to find the data in the database")
            return None
        
        #If the data is present, update it
        if result[0][0] != 0:
            val = (data[index]["v"], time.year, time.month, time.day, time.hour)
            ret = db.query(update_query, val, False)
        else:# Otherwise insert the data
            val = (db_timestamp, data[index]["v"])
            ret = db.query(insert_query, val, False)

    def callback_observe(self, response):
        log_istance.set_resource(self.resource)
        log.info("Callback Observe {}".format(self.resource))
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
        
        if response is None or response.code != defines.Codes.CONTENT.number:
            log.error("Response is None or not content")
            return None

        try:
            data = json.loads(response.payload)
        except json.JSONDecodeError as e:
            log.error("Failed to decode JSON: {}".format(e.msg))
            log.error("Bad JSON: {}".format(response.payload))
            return None
        except Exception as e:
            log.error("BAD JSON:", e)
            return None

        if data[1]['v'] == -1:
            log.info("Data recieved is -1, skipping...")
            return None

        if self.resource == 'solar_energy':
            
            self.check_and_update("sampled", data, database)
            self.check_and_update("predicted", data, database)
            log.info("Updated solar energy production")

        elif self.resource == 'temperature_HVAC':
            self.query = "INSERT INTO temperature (timestamp, temperature, active_HVAC) VALUES (%s,%s, %s)"
            time = datetime.strptime(data[1]["t"], self.date_format)
            db_timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            val = (db_timestamp, data[1]["v"], data[2]["v"])
            ret = database.query(self.query, val, False)
            if ret is None:
                log.error("Failed to insert temperature information into the database")
                return None
            log.info("Updated temperature")
        
        self.last_response = response

    def start(self):
        self.client.observe(self.resource, self.callback_observe)
    
    def stop(self):
        self.client.cancel_observing(self.last_response)
        self.client.stop()