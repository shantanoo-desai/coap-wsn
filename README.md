# coap-wsn
CoAP Server

Based on Project by Kiril Petrov (https://github.com/retfie)

Available resources:

<pre><b>CONFIG Parameters</b> : GET, POST</pre>
Configuring the IP sink address, Posting interval and Sink Path
<pre><b>BATTERY</b> : GET</pre>
Returns battery value in form of xxxx mV
<pre><b>RPL info</b> : GET</pre>
Returns the Parent information when <pre>GET rplinfo/parents</pre>
if the Node has parent/s will return eg. 1

to access the parent information: use <pre>GET rplinfo/parents?index=0</pre>

<u>TIP</u> when the returned information is eg. 1 use GET on index=0 a index=1 will return <pre>{}</pre>

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
