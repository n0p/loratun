# Receives elm327app's UDP telemetry and pushes it to an influxdb database

import asyncio
import socket
import struct
import geohash
from influxdb import InfluxDBClient
debug = 1
measurement_name = "CANdata"
# ip, puerto, usuario, clave, bbdd
influx = InfluxDBClient(host='---IP---', use_udp=True, udp_port=8089, username='apiuser', password='apipass', database='apidb')
class CAN2Influx:
    def connection_made(self, transport):
        self.transport = transport
    def datagram_received(self, data, addr):
        message = ""
        # message = data.decode() # bytes to str
        hostname = socket.gethostbyaddr(str(addr[0]))
        if debug > 0: 
            print('Received %d bytes from: %r  %r' % (len(data), hostname[0], message))
        
        if data[0]==0x43 and data[1]==0x41 and data[2] == 0x52:
            if debug > 1: 
                print("Seems like a valid packet")
            
            nextbyte = 3
            load = rpm = spd = gpslat = gpslon = gpsspd = 0
            while nextbyte < len(data):
                if (nextbyte < len(data)) and data[nextbyte] == 0x04: # Engine load - 1 byte
                    nextbyte = nextbyte +1
                    load = struct.unpack_from("B",data[nextbyte:nextbyte+1])[0]
                    if debug > 2: 
                        print("Got load: %d" % (load))
                    nextbyte = nextbyte +1
                    
                if (nextbyte < len(data)) and data[nextbyte] == 0x0c: # Avg RPM - 2B
                    nextbyte = nextbyte +1
                    rpm = struct.unpack_from(">H",data[nextbyte:nextbyte+2])[0]
                    if debug > 2: 
                        print("Got RPM: %d" % rpm)
                    nextbyte = nextbyte +2
                    
                if (nextbyte < len(data)) and data[nextbyte] == 0x0d: # Avg spd - 1B kmh
                    nextbyte = nextbyte +1
                    spd = struct.unpack_from("B",data[nextbyte:nextbyte+1])[0]
                    if debug > 2: 
                        print("Got spd: %d" % spd)
                    nextbyte = nextbyte +1
                    
                if (nextbyte < len(data)) and data[nextbyte] == 0xf0: # GPS lat - 4B signed * 1E6
                    nextbyte = nextbyte +1
                    igpslat = struct.unpack_from(">l",data[nextbyte:nextbyte+4])[0]
                    gpslat = float(igpslat) / 1000000
                    if debug > 2: 
                        print("Got GPS Lat: %f" % gpslat)
                    nextbyte = nextbyte +4
                    
                if (nextbyte < len(data)) and data[nextbyte] == 0xf1: # GPS lon - 4B signed * 1E6
                    nextbyte = nextbyte +1
                    igpslon = struct.unpack_from(">l",data[nextbyte:nextbyte+4])[0]
                    gpslon = float(igpslon) / 1000000
                    if debug > 2: 
                        print("Got GPS Lon: %f" % gpslon)
                    nextbyte = nextbyte +4
                    
                if (nextbyte < len(data)) and data[nextbyte] == 0xfd: # GPS spd - 1B kmh
                    nextbyte = nextbyte +1
                    gpsspd = struct.unpack_from("B",data[nextbyte:nextbyte+1])[0]
                    if debug > 2: 
                        print("Got GPS spd: %d" % gpsspd)
                    nextbyte = nextbyte +1
            # end while
        if ( gpslat != 0 and gpslon != 0):
            pktgeohash = geohash.encode(gpslat, gpslon)
            if debug > 2: 
                print("geohash is: %r" % pktgeohash)
            # build measurement with geohash
            json_body = [
                {
                    "measurement": measurement_name,
                    "tags": {
                        "host": str(hostname[0]),
                        "geohash": str(pktgeohash)
                    },
                    "fields": {
                        "OBDSpeed": spd,
                        "OBDRPM": rpm,
                        "OBDLoad": load,
                        "GPSspd": gpsspd,
                        "GPSlat": gpslat,
                        "GPSlon": gpslon
                    }
                }
            ]
        else:
            # build measurement without geohash
            json_body = [
                {
                    "measurement": measurement_name,
                    "tags": {
                        "host": str(hostname[0])
                    },
                    "fields": {
                        "OBDSpeed": spd,
                        "OBDRPM": rpm,
                        "OBDLoad": load
                    }
                }
            ]
            #end else
        if debug > 0: 
            print (json_body)
        influx.write_points(json_body)
        # end packet
    # end def
loop = asyncio.get_event_loop()
print("Starting UDP server")
# One protocol instance will be created to serve all client requests
listen = loop.create_datagram_endpoint(
    CAN2Influx, local_addr=('::', 43690))
transport, protocol = loop.run_until_complete(listen)
try:
    loop.run_forever()
except KeyboardInterrupt:
    pass
transport.close()
loop.close()
