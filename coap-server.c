#include <string.h>
#include <stdlib.h>

#include "contiki.h"
#include "contiki-net.h"

#include "net/rpl/rpl.h"
#include "dev/radio.h"

#if PLATFORM_HAS_BUTTON
#include "dev/button-sensor.h"
#endif

#if WITH_TMP102_SENSOR
#include "dev/se95-sensor.h"
#endif

#if PLATFORM_HAS_BATTERY
#include "dev/battery-sensor.h"
#endif

#if PLATFORM_HAS_RADIO
#include "dev/radio-sensor.h"
#endif

#include "rest-engine.h"
#include "er-coap-engine.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*default path to post */
#define DEFAULT_SINK_PATH "/SINK"
/*default time to wait between posts*/
#define DEFAULT_POST_INTERVAL 10

#define REMOTE_PORT		UIP_HTONS(COAP_DEFAULT_PORT)

extern void rplinfo_activate_resources(void); // RPL parents and paths

PROCESS(coap_server, "CoAP Server");
AUTOSTART_PROCESSES(&coap_server);

/*flag to check connection*/

static uint8_t con_ok;

/*counters*/
static int post_count;
static int uptime_count;

static rpl_dag_t *dag; // DAG 

char buf[256]; // Buffer

/*timer declaration*/
static struct etimer et_read_sensors; 
/*radio value capture*/
static radio_value_t radio_value;
/*new event declaration*/
static process_event_t ev_new_interval;

/*max length of hostnames and paths*/
#define SINK_MAXLEN 31

/*Version*/
#define SENSOR_VERSION 1
#define SENSOR_MAGIC 0x5448


/*Sensor Confirguration structure*/

typedef struct
{
	uint16_t magic;  // magic number 0x5448
	uint16_t version; // version:1
	char sink_path[SINK_MAXLEN+1]; // path to post to ..
	uint16_t post_interval; // time between posts
	uip_ipaddr_t sink_addr; // sink IP address
}SENSORConfig;

// declare the sensor config struct

static SENSORConfig sensor_cfg;

/*Set default configuration values*/
void 
sensor_config_default(SENSORConfig *c)
{
		int i;
		c->magic = SENSOR_MAGIC;
		c->version = SENSOR_VERSION;
		strncpy(c->sink_path, DEFAULT_SINK_PATH, SINK_MAXLEN);
		c-> post_interval=DEFAULT_POST_INTERVAL;

		/*>>>>>>>>>>>>>>>>>>SINK DEFAULT ADDRESS<<<<<<<<<<<<<<<<<<*/
		c->sink_addr.u16[0] = UIP_HTONS(0xbbbb);
		for (i = 1; i < 7; i++)
		{
		  c-> sink_addr.u16[i] =0;
		} 
		c->sink_addr.u16[7] = UIP_HTONS(0x0101);
}

void 
sensor_config_print(void)
{
	PRINTF("Sensor configuration: \n");
	PRINTF("Magic number: %04x\n",sensor_cfg.magic);
	PRINTF("Version:%04x\n", sensor_cfg.version);
	PRINTF("SINK PATH:%s \n",sensor_cfg.sink_path);
	PRINTF("Post Interval: %d \n", sensor_cfg.post_interval);
	PRINTF("IP Address: ");
	PRINT6ADDR(&sensor_cfg.sink_addr);
	PRINTF("\n");
}

/*IP Address Printing*/
int
ipaddr_sprintf(char *s, uip_ipaddr_t *addr) // code in uip.h
{
	uint16_t a;
	unsigned int i; //counters
	int n=0;
	int f;
	for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i+= 2)
	{
		a = (addr->u8[i] << 8) + addr->u8[i+1];
	
	if (a == 0 && f >= 0)
		{
			if(f++ == 0)
			{
				n+= sprintf(&s[n], "::"); // double colons for consecutive zeros
			}
		}else
		{
			if (f > 0)
			{
				f= -1;
			}else if (i > 0)
			{
				n+= sprintf(&s[n], ":"); // no consecutive zeros with single colon

			}
			n += sprintf(&s[n], "%x", a);
		}
	}
	return n;
}

/*Handlers for Config CoAP*/

static void config_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void config_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(config,
		"title=\"Config Parameters\"; rt= \"Data\"",
		config_get_handler,
		config_post_handler,
		NULL,
		NULL);

PROCESS_NAME(read_sensors);

/*Defining the Handler for CoAP*/

static void
config_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	const char *pstr;  // where the user data will be
	size_t len = 0; 	// length comparer
	uint8_t n = 0; 		// where manipulation of the data will occur
	if((len = REST.get_query_variable(request, "param", &pstr)))
	{
		if (strncmp(pstr, "interval", len) == 0)
		{
			/*if length of the word "interval" and the pstr match perfectly*/
			n = sprintf((char *)buffer, "%d", sensor_cfg.post_interval); 
			/* push the data of the specified post interval on buffer */
		}else if (strncmp(pstr, "path", len) == 0)
		{
			/*if length of word "path" exactly matches the pstr perfectly*/	
			strncpy((char *)buffer, sensor_cfg.sink_path, SINK_MAXLEN);
			n = strlen(sensor_cfg.sink_path);
			/*copy the the sink path on the buffer*/
		}else if (strncmp(pstr, "ip", len) == 0)
		{
			/*if the word "ip" matches pstr perfectly*/
			n = ipaddr_sprintf((char *)buffer,&sensor_cfg.sink_addr);
			/*copy the ip address of the sink to the buffer*/
		}else
		{
			goto bad; // go to bad request
		}

		/*set coap header type and also payload type of response*/
		REST.set_header_content_type(response, REST.type.TEXT_PLAIN); //basically plain text
		REST.set_response_payload(response, (uint8_t *)buffer,n); // set the payload
	}else
	{
		goto bad;	// go the bad request
	}
	return;

	bad:
	REST.set_response_status(response, REST.status.BAD_REQUEST);
}

static void 
config_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	void *param = NULL; //null pointer
	uip_ipaddr_t *new_addr; // to post new IP address
	const char *pstr;  // whatever input comes in stays in this pointer
	size_t len = 0;
	const uint8_t *new;  // for New request payload

	if((len = REST.get_query_variable(request, "param", &pstr )))
	{
		if(strncmp(pstr, "interval", len) == 0)
		{
			/*post the new posting interval if the word "interval" and pstr match perfectly*/
			param = &sensor_cfg.post_interval;
		}else if (strncmp(pstr, "path", len) == 0)
		{
			/*post the new sink path if the word "path" and pstr match perfectly*/
			param = sensor_cfg.sink_path;
		}else if (strncmp(pstr, "ip", len) == 0)
		{
			/* post the new ip address if the word "ip" and pstr match perfectly*/
			new_addr = &sensor_cfg.sink_addr;
		}else
		{
			goto bad_post;
		}
	}else
	{
		goto bad_post;
	}

	/*manipulating the request payload for POST resource*/

	REST.get_request_payload(request, &new);
	if (strncmp(pstr, "interval", len) == 0)
	{
		strncpy(param, (const char *)new, SINK_MAXLEN);
	}else if (strncmp(pstr, "ip", len) == 0)
	{
		uiplib_ipaddrconv((const char *)new, new_addr);
		PRINT6ADDR(new_addr);
	}else
	{
		*(uint16_t *)param = (uint16_t)atoi((const char *)new); // ???
	}

	sensor_config_print();

	if(strncmp(pstr, "interval", len) == 0)
	{
		/*send new_interval_event to schedule post with new time interval*/
		process_post(&coap_server, ev_new_interval, NULL);

	}else if (strncmp(pstr, "path", len) == 0 || strncmp (pstr, "ip", len) == 0)
	{
		process_start(&read_sensors, NULL);
	}
	return;

	bad_post:
	REST.set_response_status(response, REST.status.BAD_REQUEST);
}

/*CoAP resources for Battery and Radio sensor
*/
#if PLATFORM_HAS_BATTERY
static void battery_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* A simple getter example. Returns the reading from battery sensor with a simple etag */
RESOURCE(res_battery,
         "title=\"Battery status\";rt=\"Battery\"",
         battery_get_handler,
         NULL,
         NULL,
         NULL);

static void
battery_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  int battery = battery_sensor.value(0);

  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%d", battery);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == REST.type.APPLICATION_JSON) {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'battery':%d}", battery);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }
}

#endif /* PLATFORM_HAS_BATTERY */

/*if platform has radio sensor */

#if PLATFORM_HAS_RADIO
static void radio_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* A simple getter example. Returns the reading of the rssi/lqi from radio sensor */
RESOURCE(res_radio,
         "title=\"RADIO: ?p=lqi|rssi\";rt=\"RadioSensor\"",
         radio_get_handler,
         NULL,
         NULL,
         NULL);

static void
radio_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t len = 0;
  const char *p = NULL;
  uint8_t param = 0;
  int success = 1;

  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if((len = REST.get_query_variable(request, "p", &p))) {
    if(strncmp(p, "lqi", len) == 0) {
      param = RADIO_SENSOR_LAST_VALUE;
    } else if(strncmp(p, "rssi", len) == 0) {
      param = RADIO_SENSOR_LAST_PACKET;
    } else {
      success = 0;
    }
  } else {
    success = 0;
  } if(success) {
    if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
      REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
      snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%d", radio_sensor.value(param));

      REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
    } else if(accept == REST.type.APPLICATION_JSON) {
      REST.set_header_content_type(response, REST.type.APPLICATION_JSON);

      if(param == RADIO_SENSOR_LAST_VALUE) {
        snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'lqi':%d}", radio_sensor.value(param));
      } else if(param == RADIO_SENSOR_LAST_PACKET) {
        snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'rssi':%d}", radio_sensor.value(param));
      }
      REST.set_response_payload(response, buffer, strlen((char *)buffer));
    } else {
      REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
      const char *msg = "Supporting content-types text/plain and application/json";
      REST.set_response_payload(response, msg, strlen(msg));
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}
#endif /* PLATFORM_HAS_RADIO */


/* This function is will be passed to COAP_BLOCKING_REQUEST() to handle responses. 
	from the er-example-client.c
*/
void
client_chunk_handler(void *response)
{
  const uint8_t *chunk;
  coap_packet_t *const coap_pkt = (coap_packet_t *) response;

  int len = coap_get_payload(response, &chunk);
  printf("|%.*s", len, (char *)chunk);

  if (coap_pkt->type == COAP_TYPE_ACK) 
  {
  	/*if the received signal is CoAP ACK then set flag for OK Connection */
    post_count = 0;
    PRINTF("got ACK\n");
    con_ok = 1;
  } else
    con_ok = 0;
}
/*checking the Connection status*/
PROCESS(do_post, "Post Results");
PROCESS_THREAD(do_post, ev, data)
{
	PROCESS_BEGIN();
	static coap_packet_t request[1]; /* This way the packet can be treated as pointer as usual. from coap-client example */
	
	coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0); // non-confirmable message
	coap_set_header_uri_path(request, sensor_cfg.sink_path);
	coap_set_payload(request, buf, strlen(buf)); // main buffer declared at first

	post_count++;
	COAP_BLOCKING_REQUEST(&sensor_cfg.sink_addr, REMOTE_PORT, request, client_chunk_handler);

	if (erbium_status_code)
		PRINTF("status %u: %s \n", erbium_status_code, coap_error_message);
	if(con_ok == 0)
		PRINTF("CON failed \n");
	else
		PRINTF("CoAP Done \n");

	PROCESS_END();
}

#if WITH_BUTTON_SENSOR
PROCESS(button_post, "Posting via Button");
PROCESS_THREAD(button_post, ev, data)
{
	uint8_t n = 0;
	linkaddr_t *addr;

	PROCESS_BEGIN();

	addr = &linkaddr_node_addr; // get the link local address in ..
	n += snprintf(&(buf[n]), "{\"eui\": \"%02x%02x%02x%02x%02x%02x%02x%02x\",\"text\":\"%s\"}",
				  addr->u8[0],
				  addr->u8[1],
				  addr->u8[2],
				  addr->u8[3],
				  addr->u8[4],
				  addr->u8[5],
				  addr->u8[6],
				  addr->u8[7],
				  "Button Pressed"  /*get the EUI from the Link Local Address and say that Button is pressed*/
				  );
	buf[n] = 0;
	PRINTF("buf: %s\n", buf);

	if (dag != NULL)
	{	/*if DAG is already set do the posting procedure for connectivity check*/
		process_start(&do_post, NULL);
	}
	PROCESS_END();
}
#endif

PROCESS(read_sensors, "Read Sensors");
PROCESS_THREAD(read_sensors,ev,data)
{
	uint8_t n = 0;
#if WITH_TMP102_SENSOR
	uint8_t m = 0;
	char temp_buf[30]; // buffer for temperature values
#endif

	int16_t value;
	linkaddr_t *addr;
	PROCESS_BEGIN();

	addr = &linkaddr_node_addr;
	n += sprintf(&(buf[n]),"{\"eui\":\"%02x%02x%02x%02x%02x%02x%02x%02x\"",
		     addr->u8[0],
		     addr->u8[1],
		     addr->u8[2],
		     addr->u8[3],
		     addr->u8[4],
		     addr->u8[5],
		     addr->u8[6],
		     addr->u8[7]);
	n += sprintf(&(buf[n]), ",\"temp\":\" %d mC\"", 25000 + ((value >> 4)-1422) * 1000 / 42); // will give xxxx mC for temp
	n += sprintf(&(buf[n]),",\"count\": %d", uptime_count);

#if WITH_TMP102_SENSOR
	value = se95_sensor.value(0);
	if(value & (1 << 12))
		value = (~value + 1) * 0.03125 * 1000 * -1;
	else
		value *= 0.03125 * 1000;

	m = sprintf(temp_buf, "\"tmp102\":\"%d mC\"", value);

	strncat(&(buf[n]), temp_buf, m);

	n += m;
#endif

	if (NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &radio_value) == RADIO_RESULT_OK)
		n += sprintf(&(buf[n]), "\"rssi\":\"%d dBm\"", radio_value);

	n += sprintf(&(buf[n]), "}");

	buf[n] = 0;
	PRINTF("buf:%s\n", buf);

	if(dag != NULL)
	{
			/*if the DAG is not empty do connectivity check*/
		process_start(&do_post, NULL); 
	}
	
	PROCESS_END();
}

PROCESS_THREAD(coap_server, ev, data)
{
	PROCESS_BEGIN();

	ev_new_interval = process_alloc_event(); // allocate the new interval event

	/*Initialize the rest engine*/

	rest_init_engine();
	rest_activate_resource(&config, "config");
	rest_activate_resource(&res_battery, "battery");
	rest_activate_resource(&res_radio, "radio");
	rplinfo_activate_resources();

#if WITH_TMP102_SENSOR
	SENSORS_ACTIVATE(se95_sensor);
	PRINTF("TMP102 Sensor");
#endif

#if WITH_BUTTON_SENSOR
	SENSORS_ACTIVATE(button_sensor);
	PRINTF("Button Sensor");
#endif

	/*set default message*/

	sensor_config_default(&sensor_cfg);
	/*print configuration*/
	sensor_config_print();

	etimer_set(&et_read_sensors, 5 * CLOCK_SECOND);

	// initialize
	dag = NULL;
	post_count = 0;
	uptime_count = 0;

	while(1)
	{
		PROCESS_WAIT_EVENT();

#if WITH_BUTTON_SENSOR

		if(ev == sensors_event && data == &button_sensor)
		{
			printf("====> Button Pressed\n" );
			process_start(&button_post, NULL);
		}
#endif

		if (ev == ev_new_interval)
		{
				/*if new post interval is set.. make the timer reset to the new value*/
			etimer_set(&et_read_sensors, sensor_cfg.post_interval * CLOCK_SECOND);
			etimer_restart(&et_read_sensors); // restart the sensor reading again
			PRINTF("New Interval for read_sensor is set\n");
		}

		if(etimer_expired(&et_read_sensors))
		{
			PRINTF("---- post count = %d\n", post_count);

			if (dag == NULL)
			{
				/* if DAG is not set then take any DAG and join in*/
				dag = rpl_get_any_dag();

				if(dag != NULL)
				{
					PRINTF("Joined DAG.. Posting to");
					PRINT6ADDR(&sensor_cfg.sink_addr);
					PRINTF("\n");
				}
				post_count ++;
			}

			etimer_set(&et_read_sensors, sensor_cfg.post_interval * CLOCK_SECOND);
			process_start(&read_sensors, NULL);
			uptime_count++;
		}
	}
	PROCESS_END();
}
