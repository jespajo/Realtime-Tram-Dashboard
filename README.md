# Realtime-Tram-Dashboard

The Tram data server (server.py) publishes messages over a custom protocol. 

These messages are either:

1. Tram Passenger Count updates (MSGTYPE=**PASSENGER_COUNT**)
2. Tram Location updates (MSGTYPE=**LOCATION**)

It publishes these messages over a continuous byte stream, over TCP.

Each message begins with a '**MSGTYPE**' content, and all messages are made up in the format of [**CONTENT_LENGTH**][**CONTENT**]:

For example, a raw Location update message looks like this:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**7**MSGTYPE**8**LOCATION**7**TRAM_ID**7**TRAMABC**5**VALUE**4**CITY
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The first byte, '**7**', is the length of the content '**MSGTYPE**'.\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;After the last byte of '**MSGTYPE**', you will find another byte, '**8**'.\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'**8**' is the length of the next content, '**LOCATION**'.\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;After the last byte of '**LOCATION**', you will find another byte, '**7**', the length of the next content '**TRAM_ID**', and so on.\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Parsing the stream in this way will yield a message of:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;MSGTYPE => **LOCATION*8
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TRAM_ID => *8TRAMABC**
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VALUE => **CITY**

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Meaning, this is a *location* message that tells us **TRAMABC** is in the **CITY**.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Once you encounter a content of 'MSGTYPE' again, this means we are in a new message, and finished parsing the current message

The task is to read from the TCP socket, and display a **realtime** updating dashboard (console) of all trams (as you will get messages for multiple trams, indicated by *TRAM_ID*), their current location and passenger count.

Sample console app output (updates whenever a new message updates the location or passenger count):

    Tram 1:
        Location: Williams Street
        Passenger Count: 50

    Tram 2:
        Location: Flinders Street
        Passenger Count: 22

To start the server to consume from, please install python, and run **python3 server.py**

Feel free to modify the code in *tram_dashboard.c*, which already implements a TCP socket consumer and dumps the content to a string & byte array