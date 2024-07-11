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

class jsonSenML:
    def __init__(self):
        self._decimal_accuracy = 10000
        self._date_format = "%Y-%m-%dT%H:%MZ"
        self.json = None
        
    def initialize(self, json_str):
        try:
            self.json = json.loads(json_str)
        except json.JSONDecodeError as e:
            log.error("Failed to decode JSON: {}".format(e.msg))
            log.error("Bad JSON: {}".format(json_str))
            return True
        except Exception as e:
            log.error("BAD JSON:", e)
            return True
        
        # For every value received, check if is >self._decimal_accuracy, if so convert it to float
        for i in range(1, len(self.json)):
            if self.json[i]['v'] >= self._decimal_accuracy:
                self.json[i]['v'] = self.json[i]['v'] / self._decimal_accuracy
            if self.json[i]['v'] < 0:
                log.info("Data received is <0, skipping...")
                return True
        return False
    
    def get_datetime(self, index):
        if self.json is None:
            log.error("JSON is None")
            return None
        try:
            return datetime.strptime(self.json[index]['t'], self._date_format)
        except ValueError as e:
            log.error("Failed to parse the date: {}".format(e))
            return None

class CoAPObserver:
    def __init__(self, ip, resource):
        self.client = HelperClient(server=(ip, 5683))
        self.resource = resource
        self.last_response = None

    def check_and_update(self, type, data, db):
        search_query = "SELECT COUNT(*) FROM solar_production WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s"
        insert_query = "INSERT INTO solar_production (timestamp,{}) VALUES (%s,%s)".format(type)
        update_query = "UPDATE solar_production SET {} = %s WHERE YEAR(timestamp) = %s AND MONTH(timestamp) = %s AND DAY(timestamp) = %s AND HOUR(timestamp) = %s".format(type)
        index = 2
        if type == "predicted":
            index = 1    
        #Check if the sampled data is already present
        time = data.get_datetime(index)
        if time is None:
            log.error("Failed get the timestamp from the data")
            return True
        db_timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        val = (time.year, time.month, time.day, time.hour)
        result = db.query(search_query, val, True)
        if result is None:
            log.error("Failed to find the data in the database")
            return True
        
        #If the data is present, update it
        if result[0][0] != 0:
            val = (data.json[index]["v"], time.year, time.month, time.day, time.hour)
            ret = db.query(update_query, val, False)
        else:# Otherwise insert the data
            val = (db_timestamp, data.json[index]["v"])
            ret = db.query(insert_query, val, False)
        return False

    def callback_observe(self, response):
        # Set the logger for the DBAccess class
        log_istance.set_resource(self.resource)
        log.info("Callback Observe {}".format(self.resource))
        
        # Check if the response is valid
        if response is None or response.code != defines.Codes.CONTENT.number:
            log.error("Response is None or not content")
            return None

        # Read the configuration file for the database acces
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
        
        # Parse the response
        data = jsonSenML()
        if data.initialize(response.payload) is True:
            database.close()
            return None
        
        if self.resource == 'solar_energy':
            check = self.check_and_update("sampled", data, database)
            if check is True:
                database.close()
                return None
            self.check_and_update("predicted", data, database)
            log.info("Updated solar energy production")

        elif self.resource == 'temperature_HVAC':
            self.query = "INSERT INTO temperature (timestamp, temperature, active_HVAC) VALUES (%s,%s, %s)"
            time = data.get_datetime(1)
            db_timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            val = (db_timestamp, data.json[1]["v"], data.json[2]["v"])
            ret = database.query(self.query, val, False)
            if ret is None:
                log.error("Failed to insert temperature information into the database: {}".format(val))
                return None
            else:
                log.info("Updated temperature")
        
        self.last_response = response

    def start(self):
        self.client.observe(self.resource, self.callback_observe)
    
    def stop(self):
        self.client.cancel_observing(self.last_response)
        self.client.stop()