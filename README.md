# Realtime-Tram-Dashboard

The Tram data server (server.py) publishes messages over a custom protocol. 

These messages are either:

1. Tram Passenger Count updates (MSGTYPE=**PASSENGER_COUNT**)
2. Tram Location updates (MSGTYPE=**LOCATION**)

It publishes these messages over a continuous byte stream, over TCP.

Each message begins with a '**MSGTYPE**' content, and all messages are made up in the format of [**CONTENT_LENGTH**][**CONTENT**]:

For example, a raw Location update message looks like this:

- `\x07MSGTYPE\x08LOCATION\x07TRAM_ID\x07TRAMABC\x05VALUE\x04CITY`
    - The first byte, `\x07`, is the length of the content `MSGTYPE`. 
    - After the last byte of '`MSGTYPE`, you will find another byte, `\x08`.
    - `\x08` is the length of the next content, `LOCATION`. 
    - After the last byte of `LOCATION`, you will find another byte, `\x07`, the length of the next content `TRAM_ID`, and so on.
- Parsing the stream in this way will yield a message of:
    - MSGTYPE => **LOCATION**
    - TRAM_ID => **TRAMABC**
    - VALUE => **CITY**
- Meaning, this is a *location* message that tells us **TRAMABC** is in the **CITY**.
- Once you encounter a content of `MSGTYPE` again, this means we are in a new message, and finished parsing the current message

The task is to read from the TCP socket, and display a **realtime** updating dashboard (console) of all trams (as you will get messages for multiple trams, indicated by *TRAM_ID*), their current location and passenger count.

Sample console app output (updates whenever a new message updates the location or passenger count):

    Tram 1:
        Location: Williams Street
        Passenger Count: 50

    Tram 2:
        Location: Flinders Street
        Passenger Count: 22

To start the server to consume from, please install python, and run **python3 server.py 8081**

Feel free to modify the code in *tram_dashboard.c*, which already implements a TCP socket consumer and dumps the content to a string & byte array
