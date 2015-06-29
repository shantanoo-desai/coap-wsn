# coap-wsn
CoAP Server

Based on Project by Kiril Petrov (https://github.com/retfie)

Available resources:

<pre><b>CONFIG Parameters</b> : GET, POST</pre>
Configuring the IP sink address, Posting interval and Sink Path

<pre><b>GET config?param=ip</b></pre>
should return the sinks IP address
<pre><b>PUT config?param=ip (new sink address)</b></pre>
should update the mote to a new sink address
<pre>GET config?param=interval</pre>
should return a number with units of Seconds eg. 10 means 10 seconds
<pre>PUT config?param=interval (number)</pre>
should change the posting interval
<pre>GET config?param=path</pre>
should return <b> /SINK</b> as an idea of what the IP address is titled.
<pre><b>BATTERY</b> : GET</pre>
Returns battery value in form of xxxx mV
<pre><b>RPL info</b> : GET</pre>
Returns the Parent information when <pre>GET rplinfo/parents</pre>
if the Node has parent/s will return eg. 1

to access the parent information: use <pre>GET rplinfo/parents?index=0</pre>

<b>TIP</b>: when the returned information is eg. 1 use GET on index=0 a index=1 will return <pre>{}</pre>

For Multihop network
<pre>GET rplinfo/route</pre>
it should return values 1 or 2 according the Physical setup.
Similar to Parents info method use <pre>GET rplinfo/route?index=0 or 1 </pre> to return the Destination Address

if the Node is within the Border Router range it will return the Address of the BORDER ROUTER.

<pre><b>RADIO</b> : GET</pre>
will return the LQI and RSSI values according to the query index <b>p</b>
eg:
<pre> GET radio?p=lqi </pre> should return some value of LQI
<pre> GET radio?p=rssi</pre> should return some values of RSSI


To Access CoAP Server Resources:

Use of SMCP which has command line interface.

(Note: there seems to be some Problem at present when using the Copper Plugin in Mozilla Firefox)

for further info on SMCP : https://github.com/darconeous/smcp/

The CoAP server keeps posting the following :
<pre>{eui:aabbccddeeff, tmp=xxxx mC, count=yy, rssi =-abc dBm}</pre>
according to the 'interval' query in CONFIG parameter. 


By default the time interval is 10 seconds but that could be changed using the following line:

<pre>~$ smcpctl coap://[Sensor IPv6 address]:5683/config?param=interval 3600 </pre>


the above will configure the server to post the data every one hour (60 * 60  = 3600 seconds)
 
The Sink address is : **bbbb::101** is a Linux PC (Ubuntu) connected to the Sensor Network via the 6LBR border router solution by CETIC.

for further information (https://github.com/cetic/6lbr/wiki)

The sink is hence connected to the linux pc using a ethernet cable to a Raspberry Pi which runs the 6LBR on it and a Zolertia Z1 mote works as a SLIP-Radio which sniffs packets from the sensor network and then passes them to the Raspberry Pi

NOTE: Raspberry pi runs on the ROUTER mode and check if the SLIP radio is connected on the ttyUSB0 port **on the Raspberry Pi not on the Linux PC**
<pre>ls -l /dev/ttyUSB*</pre>

the important lines to establish with the Raspberry Pi 6LBR are following

<pre>sudo sysctl -w net.ipv6.conf.eth0.accept_ra=1
	sudo sysctl -w net.ipv6.conf.eth0.accept_ra_rt_info_max_plen=64

	sudo ip -6 addr add bbbb::101/64 dev eth0
	sudo route -A inet6 add aaaa::/64 gw bbbb::100
</pre>

to check connectivity ping the 6LBR
<pre> ping6 bbbb::101</pre>

Capture check: 
Use the most current Wireshark Version 1.12.3 (v1.12.3-0-gbb3e9a0 from master-1.12)
and capture packets on eth0 and observe the CoAP packets
