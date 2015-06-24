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

