from configparser import ConfigParser
from Utility.DBAccess import DBAccess
from Utility.Log import Log
from tabulate import tabulate
from coapthon.client.helperclient import HelperClient
import os
import json

log = Log(__name__, "User").get_logger()

class Node:
    def __init__(self, node_tuple, id):
        self.id = id
        self.ip = node_tuple[0]
        self.name = node_tuple[1]
        self.settings = json.loads(node_tuple[2])
        self.modified = False
        self._keys = list(self.settings.keys())

    def print_settings(self):
        print(f"\n[{self.id}-{self.name.upper()}]")
        i = 0
        for key, value in self.settings.items():
            print(f"\t[{i}] {key}: {value}")
            i += 1
    
    def change_setting(self, parameter_id, new_value):
        self.settings[self._keys[parameter_id]] = new_value
        self.modified = True

    def __str__(self):
        return f"IP: {self.ip}, Name: {self.name}, Settings: {self.settings}"


class UserApplication:
    def __init__(self):
        configur = ConfigParser()
        configur.read('./CoAPServer/config.ini')
        
        self.db = DBAccess(
            host = configur.get('mysql', 'host'),
            user = configur.get('mysql', 'user'),
            password = configur.get('mysql', 'password'),
            database = configur.get('mysql', 'database'),
            log=log)

    def start(self):
        
        while True:
            self.print_menu()
            command = input("Command: ")
            if command == "1":
                self.nodes_settings()
            elif command == "2":
                self.list_nodes()
            elif command == "3":
                break
            else:
                print("Invalid command")

    def print_menu(self):
        self.clear()
        print("||======================MENU=====================================||")
        print("||Enter the corresponding command number:                        ||")
        print("|| [1] Nodes settings                                            ||")
        print("|| [2] List nodes                                                ||")
        print("|| [3] Exit                                                      ||")
        print("||===============================================================||")

    def setting_menu(self, nodes):
        self.clear()
        print("||======================NODES SETTINGS============================||")
        for node in nodes:
            node.print_settings()           
        print("[s] save changes")
        print("[q] back")
        input_cmd = input("Insert the new value of the chosen parameter (format: [node_id] [parameter_id] [new_value]) or [q] to go back: ")
        return input_cmd

    def save_settings(self, nodes):
        print("Saving settings...")
        # Update settings in the database
        for node in nodes:
            if node.modified:
                self.db.query(query="UPDATE nodes SET settings = %s WHERE ip = %s", val=(json.dumps(node.settings), node.ip))
        # Update settings in the nodes
        print("Sending the new settings: ", end="")
        for node in nodes:
            if node.modified:
                client = HelperClient(server=(node.ip, 5683))
                print(f"\n \t{node.name} - {node.ip}")
                client.post("settings", json.dumps(node.settings))
                client.stop()
        print("Settings saved!")
        input("Press any key to continue...")

    def nodes_settings(self):
        self.db.connect()
        sql_nodes = self.db.query(query="SELECT ip,name,settings FROM nodes",val=None, fetchall=True)
        nodes = []
        i=0
        for node in sql_nodes:
            nodes.append(Node(node, i))
            i += 1
        
        input_cmd = ""
        while input_cmd != "q":
            input_cmd = self.setting_menu(nodes)
            
            if input_cmd == "s":
                self.save_settings(nodes)
            elif input_cmd == "q":
                break
            else:
                
                try:
                    input_cmd = input_cmd.split(" ")
                    if len(input_cmd) != 3:
                        print("Invalid command")
                        continue
                    node_id = int(input_cmd[0])
                    parameter_id = int(input_cmd[1])
                    new_value = float(input_cmd[2])
                except:
                    print("Invalid command")
                
                nodes[node_id].change_setting(parameter_id, new_value)
        self.db.close()          
         
    def list_nodes(self):
        self.clear()
        self.db.connect()
        nodes = self.db.query(query="SELECT * FROM nodes",val=None, fetchall=True)
        self.db.close()

        intestazioni = ["Ip", "Name", "Resource", "Parameters Setting"]
        print(tabulate(nodes, headers=intestazioni, tablefmt="psql"))
        input("Press any key to return to menu...")
    
    def clear(self):
        os.system('cls' if os.name == 'nt' else 'clear')
            