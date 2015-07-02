# coap-wsn
CoAP Server

Based on Project by Kiril Petrov (https://github.com/retfie)

programmed using Erbium for CoAP used in Contiki-OS (https://www.contiki-os.org)

for current repositories on Contiki OS official repository link (https://github.com/contiki-os/contiki)


Available resources:

<pre><b>CONFIG Parameters</b> : GET, POST</pre>
Configuring the IP sink address, Posting interval and Sink Path

<pre><b>GET config?param=ip</b></pre>
should return the sinks IP address

<pre><b>POST config?param=ip (new sink address)</b></pre>
should update the mote to a new sink address

<pre><b> GET config?param=interval </b></pre>
should return a number with units of Seconds eg. 10 means 10 seconds

<pre><b> POST config?param=interval (number) </b></pre>
should change the posting interval

<pre><b> GET config?param=path </b></pre>
should return <b> /SINK</b> as an idea of what the IP address is titled.

<pre><b> POST config?param=path (name-of-path)</b></pre>
should change the path name to the mentioned one.


<pre><b>BATTERY</b> : GET</pre>
Returns battery value in form of xxxx mV
<h2>Routing Information using rplinfo</h2>

<pre><b>RPL info</b> : GET</pre>

Returns the Parent information when <pre>GET rplinfo/parents</pre>
if the Node has parent/s will return eg. 1

to access the parent information: use <pre>GET rplinfo/parents?index=0</pre>

<b>TIP</b>: when the returned information is eg. 1 use GET on index=0 as index=1 will return. 
 <pre>{}</pre>

For Multihop network
<pre>GET rplinfo/route</pre>
it should return values 1 or 2 according the Physical setup.
Hence if there are two available parents in a multihop network, the <b>GET</b> will return '2' which means to obtain the information use the <b> GET </b> with index = 0 or 1

Similar to Parents info method use <pre>GET rplinfo/route?index=0 or 1 </pre> to return the Destination Address

if the Node is within the Border Router range it will return the Address of the BORDER ROUTER.

<pre><b>RADIO</b> : GET</pre>
will return the LQI and RSSI values according to the query index <b>param</b>
eg:
<pre> GET radio?param=lqi </pre> should return some value of LQI
<pre> GET radio?param=rssi</pre> should return some values of RSSI


To Access CoAP Server Resources:

Use of SMCP which has command line interface.

(<b>Note</b>: there seems to be some Problem at present when using the Copper Plugin in Mozilla Firefox).


for further info on SMCP : https://github.com/darconeous/smcp/

The CoAP server keeps posting the following :
<pre>{eui:aabbccddeeff, tmp=xxxx mC, count=yy}</pre>
according to the 'interval' query in CONFIG parameter. 


By default the time interval is 10 seconds but that could be changed using the following line:

<pre>~$ smcpctl coap://[Sensor IPv6 address]:5683/config?param=interval 3600 </pre>


the above will configure the server to post the data every one hour (60 * 60  = 3600 seconds).
 
The Sink address is : <b>bbbb::101</b> is a Linux PC (Ubuntu) connected to the Sensor Network via the 6LBR border router solution by CETIC.

for further information (https://github.com/cetic/6lbr/wiki).

The sink is hence connected to the linux pc using a ethernet cable to a Raspberry Pi which runs the 6LBR on it and a Zolertia Z1 mote works as a SLIP-Radio which sniffs packets from the sensor network and then passes them to the Raspberry Pi.

NOTE: Raspberry pi runs on the ROUTER mode and check if the SLIP radio is connected on the ttyUSB0 port <b>on the Raspberry Pi not on the Linux PC</b>
<pre>ls -l /dev/ttyUSB*</pre>

the important lines to establish with the Raspberry Pi 6LBR are following

<pre>sudo sysctl -w net.ipv6.conf.eth0.accept_ra=1
	sudo sysctl -w net.ipv6.conf.eth0.accept_ra_rt_info_max_plen=64

	sudo ip -6 addr add bbbb::101/64 dev eth0
	sudo route -A inet6 add aaaa::/64 gw bbbb::100
</pre>

to check connectivity ping the 6LBR
<pre> ping6 bbbb::100</pre> 
also use a Web Browser to access the Border Router's webpage.
<pre>
		http://[bbbb::100]
</pre>

Capture check: 
Use the most current Wireshark Version 1.12.3 (v1.12.3-0-gbb3e9a0 from master-1.12)
and capture packets on eth0 and observe the CoAP packets.

OUTPUT USING SMCPCTL in Ubuntu Terminal:
<pre>
~$ smcpctl coap://[aaaa::c30c:0:0:7b]:5683/
Listening on port 61616.
coap://[aaaa::c30c:0:0:7b]:5683/> get .well-known/core
</.well-known/core>;ct=40,</config>;title="Config Parameters"; rt= "Data",</battig>;title="Config Parameters"; rt= "Data",</battery>;title="Battery status";rt="Battery",</radio>;title="RADIO: ?param=lqi|rssi";rt="RadioSensor",</rplinfo/parents>;title="PARENT INFO"; rt = "Data",</rplinfo/routes>;title="RPL ROUTE INFO"; rt="Data"
coap://[aaaa::c30c:0:0:7b]:5683/> get config?param=interval
60
coap://[aaaa::c30c:0:0:7b]:5683/> get config?param=ip
bbbb::101
coap://[aaaa::c30c:0:0:7b]:5683/> get config?param=path
/SINK
coap://[aaaa::c30c:0:0:7b]:5683/> get battery
1993 mV
coap://[aaaa::c30c:0:0:7b]:5683/> get radio?param=lqi
-84
coap://[aaaa::c30c:0:0:7b]:5683/> get radio?param=rssi
101
coap://[aaaa::c30c:0:0:7b]:5683/> get rplinfo/parent
get: Result code = 132 (NOT_FOUND)
coap://[aaaa::c30c:0:0:7b]:5683/> get rplinfo/parents
1
coap://[aaaa::c30c:0:0:7b]:5683/> get rplinfo/parents?index=0
{"eui":"c30c0000000010b7","pref":true,"etx":256}
coap://[aaaa::c30c:0:0:7b]:5683/> 

</pre>

Using SMCP a simple coap-server is made on the linux pc which listens to the CoAP port of 5683 and responds to the incoming COAP_POST from the CoAP server on Zolertia Z1 and responds with an ACK to the node back and the con_ok in the node is set to 1 which assures the posting message was received by the SINK.

when the following command is executed in the folder smcp/src/examples
(Will upload the file and complete folder soon! )
<pre>
	./coap-server
</pre>

the output is shown below:
<pre>
	Listening on port 5683
{"eui":"c10c00000000007b","temp":"25471 mC","count":1}
{"eui":"c10c00000000007b","temp":"25471 mC","count":2}
{"eui":"c10c00000000007b","temp":"25471 mC","count":3}
{"eui":"c10c00000000007b","temp":"25471 mC","count":4}
{"eui":"c10c00000000007b","temp":"25471 mC","count":5}
{"eui":"c10c00000000007b","temp":"25471 mC","count":6}
{"eui":"c10c00000000007b","temp":"25471 mC","count":7}
{"eui":"c10c00000000007b","temp":"25471 mC","count":8}
{"eui":"c10c00000000007b","temp":"25471 mC","count":9}
{"eui":"c10c00000000007b","temp":"25471 mC","count":10}
{"eui":"c10c00000000007b","temp":"25471 mC","count":11}
{"eui":"c10c00000000007b","temp":"25471 mC","count":12}
{"eui":"c10c00000000007b","temp":"25471 mC","count":13}
{"eui":"c10c00000000007b","temp":"25471 mC","count":14}
{"eui":"c10c00000000007b","temp":"25471 mC","count":15}
{"eui":"c10c00000000007b","temp":"25471 mC","count":16}
{"eui":"c10c00000000007b","temp":"25471 mC","count":17}
{"eui":"c10c00000000007b","temp":"25471 mC","count":18}

</pre>

Hence the Port number 5683 is now busy at listening to what the server is sending and giving a RESPONSE back. 
The response needs to be sent back to the Sensor node because of a CON (Confirmable Type) POST. The server anticipates an Acknowlegdement which the Linux PC running the smcp coap-server provides.


<h2>TO DO LIST</h2>
1. Possiblity: while printing data keep printing the same content to a .txt or .json file..

2. need to use this data printed on the gnome terminal for a simple UI.