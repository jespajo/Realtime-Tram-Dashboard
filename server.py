#! /usr/bin/python
import socket,os
from random import randrange
import time
import sys

class Util:
    # Helpers
    def get_random_elem(arr):
        return arr[randrange(0, len(arr))]

    def string_to_bytes(s):
        return str.encode(s)

    def add_field_to_byte_array(b, fieldValue):
        fieldLenByte = len(fieldValue)
        fieldValueInBytes = Util.string_to_bytes(fieldValue)
        b.append(fieldLenByte)
        for i in range(len(fieldValueInBytes)):
            b.append(fieldValueInBytes[i])

class MockDataProvider:
    TRAMS = ['Tram 1', 'Tram 2', 'Tram 3', 'Tram 4']
    LOCATIONS = ['Flinders Uni', 'Southbank', 'Williams Street', 'Central', 'Bourke Street Mall']

    # Data providers
    def get_tram_location():
        b = bytearray()
        Util.add_field_to_byte_array(b, "MSGTYPE")
        Util.add_field_to_byte_array(b, "LOCATION")
        Util.add_field_to_byte_array(b, "TRAM_ID")
        Util.add_field_to_byte_array(b, Util.get_random_elem(MockDataProvider.TRAMS))
        Util.add_field_to_byte_array(b, "VALUE")
        Util.add_field_to_byte_array(b, Util.get_random_elem(MockDataProvider.LOCATIONS))
        return b

    def get_tram_passenger_count():
        b = bytearray()
        Util.add_field_to_byte_array(b, "MSGTYPE")
        Util.add_field_to_byte_array(b, "PASSENGER_COUNT")
        Util.add_field_to_byte_array(b, "TRAM_ID")
        Util.add_field_to_byte_array(b, Util.get_random_elem(MockDataProvider.TRAMS))
        Util.add_field_to_byte_array(b, "VALUE")
        Util.add_field_to_byte_array(b, str(randrange(5, 200)))
        return b



while (True):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  
    sock.bind(('127.0.0.1', int(sys.argv[1])))
    sock.listen(5)

    connection, address = sock.accept()  
    try:
        print("Connection established")
        while (True):
            time.sleep(2)
            buf = None
            if (randrange(2) == 0):
                buf = MockDataProvider.get_tram_location()
            else:
                buf = MockDataProvider.get_tram_passenger_count()
            #print(buf)
            connection.send(buf)
    except Exception as e:
        print('Error with sending data')
        print(e)
    connection.close()