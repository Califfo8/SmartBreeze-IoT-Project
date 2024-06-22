from CoAPServer.CoAPServer import CoAPServer

if __name__ == '__main__':
    ip = "fd00::1"
    port = 5683
    server = CoAPServer(ip, port)
    
    try:
        server.listen(10)
    except KeyboardInterrupt:
        print("Server Shutdown")
        server.close()
        print("Exiting...")
