#CoAP Server

Programmed using [Erbium](http://people.inf.ethz.ch/mkovatsc/erbium.php) for [CoAP](coap.technology) used in [Contiki-OS](https://www.contiki-os.org)

## Available resources:

### CONFIG Parameters: GET, POST
Configuring the IP sink address, Posting interval and Sink Path

`GET config?param=ip`
should return the sinks IP address

`POST config?param=ip (new sink address)`
should update the mote to a new sink address

`GET config?param=interval`
should return a number with units of Seconds eg. 10 means 10 seconds

`POST config?param=interval (number)`
should change the posting interval

`GET config?param=path`
should return __/SINK__ as an idea of what the IP address is titled.

`POST config?param=path (name-of-path)`
should change the path name to the mentioned one.


### __BATTERY__: GET
Returns battery value in form of xxxx mV

## Routing Information using rplinfo

### __RPL info__: GET

Returns the Parent information when `GET rplinfo/parents`
if the Node has parent/s will return eg. 1

to access the parent information: use `GET rplinfo/parents?index=0`

__TIP__: when the returned information is eg. 1 use GET on index=0 as index=1 will return. 
 `{}`

### For Multihop network

`GET rplinfo/route`

it should return values 1 or 2 according the Physical setup.
Hence if there are two available parents in a multihop network, the __GET__ will return '2' which means to obtain the information use the __GET__ with index = 0 or 1

Similar to Parents info method use `GET rplinfo/route?index=0 or 1` to return the Destination Address

```
coap://[aaaa::c30c:0:0:1c8]:5683/> get rplinfo/routes?index=0
{"Dest":"aaaa::c30c:0:0:7b","Next":"fe80::c30c:0:0:7b"}
```

if the Node is within the Border Router range it will return the Address of the BORDER ROUTER.

### __RADIO__ : GET
will return the LQI and RSSI values according to the query index __param__
e.g.:

    GET radio?param=lqi </pre> should return some value of LQI
    GET radio?param=rssi</pre> should return some values of RSSI

__NEW__: Adding the LPM (Low Transmit Power Module) for testing Multi-Hop networks.

To Access CoAP Server Resources:

Use of SMCP which has command line interface.



for further info on [SMCP](https://github.com/darconeous/smcp/)

The CoAP server keeps posting the following :

`{"eui":"aabbccddeeff", "count"=yy,"tmp102:jj.kkkk C"}`

according to the 'interval' query in CONFIG parameter. 


By default the time interval is 30 seconds but that could be changed using the following line:

`~$ smcpctl coap://[Sensor IPv6 address]:5683/config?param=interval 3600`


the above will configure the server to post the data every one hour (60 * 60  = 3600 seconds).
 
The Sink address is : `bbbb::101` is a Linux PC (Ubuntu) connected to the Sensor Network via the 6LBR border router solution by __CETIC__.

for further information [6LBR-Wiki](https://github.com/cetic/6lbr/wiki).

The sink is hence connected to the linux pc using a ethernet cable to a Raspberry Pi which runs the 6LBR on it and a Zolertia Z1 mote works as a SLIP-Radio which sniffs packets from the sensor network and then passes them to the Raspberry Pi.

__NOTE__: Raspberry pi runs on the ROUTER mode and check if the SLIP radio is connected on the `ttyUSB0` port *on the Raspberry Pi not on the Linux PC*

`ls -l /dev/ttyUSB*`

the important lines to establish with the Raspberry Pi 6LBR are following

```
	sudo sysctl -w net.ipv6.conf.eth0.accept_ra=1
	sudo sysctl -w net.ipv6.conf.eth0.accept_ra_rt_info_max_plen=64

	sudo ip -6 addr add bbbb::101/64 dev eth0
	sudo route -A inet6 add aaaa::/64 gw bbbb::100
```

to check connectivity ping the 6LBR

    ping6 bbbb::100
    
also use a Web Browser to access the Border Router's webpage.

    http://[bbbb::100]

### Capture check:

Use the most current *Wireshark Version 1.12.3 (v1.12.3-0-gbb3e9a0 from master-1.12)*
and capture packets on eth0 and observe the CoAP packets.

OUTPUT AT THE TERMINAL FOR SENSOR: (using `make login`)

```
	Rime started with address 193.12.0.0.0.0.1.200
	MAC c1:0c:00:00:00:00:01:c8 Ref ID: 4979
	Contiki-2.6-2343-g06f7acf started. Node id is set to 456.
	CSMA ContikiMAC, channel check rate 8 Hz, radio channel 26
	Tentative link-local IPv6 address fe80:0000:0000:0000:c30c:0000:0000:01c8
	Starting 'CoAP Server'
	Button SensorSensor configuration: 
	Magic number: 5448
	Version:0001
	SINK PATH:/SINK 
	Post Interval: 30 
	IP Address: [bbbb:0000:0000:0000:0000:0000:0000:0101]
	---- post count = 0
	buf:{"eui":"c10c0000000001c8","count":0"tmp102":" 0.0000 C"}
	---- post count = 1
	buf:{"eui":"c10c0000000001c8","count":1"tmp102":" 30.7500 C"}
```

__Note__: the initial __0.0000 C__ is due to initialization of the sensor. can be considered Garbage.

OUTPUT USING SMCPCTL in Ubuntu Terminal:

```
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
coap://[aaaa::c30c:0:0:1c8]:5683/> get rplinfo/routes?index=0
{"Dest":"aaaa::c30c:0:0:7b","Next":"fe80::c30c:0:0:7b"}
```

Using __SMCP__ a simple coap-server is made on the linux pc which listens to the CoAP port of __5683__ and responds to the incoming `COAP_POST` from the CoAP server on Zolertia Z1 and responds with an ACK to the node back and the `con_ok` in the node is set to 1 which assures the posting message was received by the __SINK__

when the following command is executed in the folder `smcp/src/examples`
(Will upload the file and complete folder soon! )

    ./coap-server

the output is shown below:
```
	Listening on port 5683
{"eui":"c10c00000000007b","count":0"tmp102":" 0.0000 C"}
{"eui":"c10c0000000001c8","count":0"tmp102":" 0.0000 C"}
{"eui":"c10c00000000007b","count":1"tmp102":" 29.4375 C"}
{"eui":"c10c00000000007b","count":1"tmp102":" 29.4375 C"}
{"eui":"c10c0000000001c8","count":1"tmp102":" 30.0625 C"}
{"eui":"c10c00000000007b","count":1"tmp102":" 29.4375 C"}
{"eui":"c10c00000000007b","count":1"tmp102":" 29.4375 C"}
{"eui":"c10c0000000001c8","count":2"tmp102":" 30.1250 C"}

```

Hence the Port number 5683 is now busy at listening to what the server is sending and giving a RESPONSE back. 
The response needs to be sent back to the Sensor node because of a __CON (Confirmable Type) POST__. The server anticipates an Acknowlegdement which the Linux PC running the smcp coap-server provides.


## RECENT CHANGES
1. added LPM (low transmit power) for checking multi-hop

2. changed the temperature acquiring by using TMP-102 (in-built) sensor

3. added the serial output of sensor

4. added route example for multi-hop scheme

5. will save the incoming post in a `data-collect.txt` in folder `smcp/src/examples`
