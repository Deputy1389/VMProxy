Here is a proxy that allows you to sign into one vm of unix from another and use normally. 

Start sproxy on ServerVM. It should listen on TCP port SPort.

sproxy SPort

Start the cproxy on ClientVM to listen on TCP port 5200, and specify the sproxy:

cproxy CPort ServerIP SPort

On ClientVM, telnet to cproxy:

telnet localhost CPort

Where CPort and SPort are a chosen port number and SIP is the IP of the serverVM