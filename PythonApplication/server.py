from CoAPServer.CoAPServer import CoAPServer


ip = "fd00::1"
port = 5683
server = CoAPServer(ip, port)
try:
    server.listen(10)
except KeyboardInterrupt:
    print("Server Shutdown")
    server.close()
    print("Exiting...")
