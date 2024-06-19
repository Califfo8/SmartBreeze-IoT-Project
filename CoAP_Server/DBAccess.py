import mysql.connector

class DBAccess():
    def __init__(self, host, user, password, database):
        self.host = host
        self.user = user
        self.password = password
        self.database = database

        self.mydb = None
        self.mycursor = None

    def connect(self):
        try:
            # Connect to the MySQL database
            self.mydb = mysql.connector.connect(
                host=self.host,
                user=self.user,
                password=self.password,
                database=self.database
            )
            # Create a cursor object to execute SQL queries
            self.mycursor = self.mydb.cursor()
        except Exception as e:
            print("[ERROR] Could not connect to the database: {}".format(e))
            return None
        return True
    
    def close(self):
        self.mydb.close()
        self.mydb = None
        self.mycursor = None
    
    def query(self, query, val, fetchall=False):
        """Query the database with the given query and values, return the result of the query if it is a SELECT query, otherwise return True if the query was successful."""
        if self.mydb is None:
            print("[ERROR] Database connection is not established")
            return None

        try:
            # Execute the SQL query
            self.mycursor.execute(query, val)
        except Exception as e:
            print("[ERROR] Could not execute the query: {}".format(e))
            self.mydb.close()
            return None

        # Check if the query is a SELECT query
        if fetchall:
            # take the result of the query and close the connection
            result = self.mycursor.fetchall()
        else:
            # commit the changes
            self.mydb.commit()
            result = True

        return result
