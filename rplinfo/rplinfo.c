
/*code based on Mariano Alvira's project rplinfo on github.com and
 updates based on Kiril Petrov's application*/

#include <string.h>
#include <stdlib.h>

#include "contiki.h"
#include "contiki-net.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "net/rpl/rpl.h"

#include "rest-engine.h"
#include "er-coap-engine.h"

#include "rplinfo.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"


/*to add IP Address*/

uint16_t
ipaddr_add(const uip_ipaddr_t *addr, char *buf) // code in uip.h
{
	uint16_t a,n;
	int i,f; //counters
	n=0;
	for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i+= 2)
	{
		a = (addr->u8[i] << 8) + addr->u8[i+1];
	
	if (a == 0 && f >= 0)
		{
			if(f++ == 0)
			{
				n+= sprintf(&buf[n], "::"); // double colons for consecutive zeros
			}
		}else
		{
			if (f > 0)
			{
				f= -1;
			}else if (i > 0)
			{
				n+= sprintf(&buf[n], ":"); // no consecutive zeros with single colon

			}
			n += sprintf(&buf[n], "%x", a);
		}
	}
	return n;
}

/*Creating routing message*/

uint16_t 
create_route_msg(char *buf, uip_ds6_route_t *r)
{
	uint8_t n = 0;
	n += sprintf(&(buf[n]), "{\"dest\":\""); // sprintf for buffer prints
	
	n += ipaddr_add(&r->ipaddr, &(buf[n])); // push IP address in Buffer

	n += sprintf(&(buf[n]), "\",\"next\":\"");

	n += ipaddr_add(uip_ds6_route_nexthop(r), &(buf[n])); // Next hop address in Buffer

	n += sprintf(&(buf[n]), "\"}");

	buf[n] = 0; //clear present n value of buffer

	PRINTF("buf: %s\n",buf); 	// show the present thing in the whole buffer
	
	return n;
}

/*define Handlers for CoAP Resources*/

static void routes_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*Define resource
	Should have 6 argument values..

*/

RESOURCE(routes,
		"title=\"RPL ROUTE INFO\"; rt=\"Data\"",
		routes_handler,
		routes_handler,
		NULL,
		NULL);

/*develop the handler*/
void
routes_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	int32_t strpos = 0;
	uip_ds6_route_t *r;
	volatile uint8_t i;

	size_t len = 0;
	uint8_t index;   //>>>>>>>>>>>>>>>>>>index used while experimenting eg. : GET rplinfo/routes?index=1
	const char *pstr;
	uint8_t count;

	/*Count the number of the routes and return them*/
	count=uip_ds6_route_num_routes();

	if((len = REST.get_query_variable(request, "index", &pstr))) // Parse for the index variable and compare value
	{
		index=(uint8_t)atoi(pstr);

		if(index >= count)
		{
			strpos = snprintf((char *)buffer,preferred_size, "{}"); 
			/*
			 if index is greater or same as count value return empty brackets
			 */
		}else
		{
			/*seek route entry and return it*/
			i = 0;
			for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r), i++)
			{
				if (i == index)
				{
					break;
				}
			}
			strpos = create_route_msg((char *)buffer, r);
		}

		REST.set_header_content_type(response, REST.type.APPLICATION_JSON); // Header will be in JSON format...
	}else
	{
		// index not provided
		strpos += snprintf((char *)buffer, preferred_size, "%d", count);
	}

	//*offset = -1;
	REST.set_response_payload(response, (char *)buffer, strpos); // Setting the payload of response.

}

/*
	ROUTE MESSAGE:
		At this point output ----> {"dest":"some IPv6 address","next": "next hop address"}
*/


/*
	creating Parent Message

*/

uint16_t
create_parent_msg(char *buf,rpl_parent_t *parent, uint8_t preferred)
{
	uint8_t n = 0;
	uip_ds6_nbr_t *nbr;

	uip_ipaddr_t * addr = rpl_get_parent_ipaddr(parent);

	n += sprintf(&(buf[n]), "{\"eui\":\"%04x%04x%04x%04x\",",
					UIP_HTONS(addr->u16[4]),
					UIP_HTONS(addr->u16[5]),
					UIP_HTONS(addr->u16[6]),
					UIP_HTONS(addr->u16[7]));
	
	n += sprintf(&(buf[n]), "\"pref\":");

	if(preferred == 1)
	{
		n += sprintf(&(buf[n]), "true,");
	}else
	{
		n += sprintf(&(buf[n]), "false,");
	}

	nbr = rpl_get_nbr(parent); // Estimated Retransmission part (ETX) value

	n += sprintf(&(buf[n]), "\"etx\":%d}",nbr->link_metric);

	buf[n] = 0; // clear next buffer value
	PRINTF("buf: %s\n", buf); // print the buffer value
	return n;
}

/*
	ROUTE MESSAGE :
		at this point parent msg: ----> {"eui":"xxxxxxxxxxx","pref": true/false,"etx":000}

*/

/*Define the Parent Handler and the Resource*/
static void parents_handler(void *request, void *response, uint8_t *buffer ,uint16_t preferred_size, int32_t *offset);


RESOURCE(parents,
		"title=\"PARENT INFO\"; rt = \"Data\"",
		parents_handler,
		parents_handler,
		NULL,
		NULL);

static volatile uint8_t cur_neigh_entry;
static volatile uint8_t entry_char_skip;

void
parents_handler(void *request, void *response, uint8_t *buffer ,uint16_t preferred_size, int32_t *offset)
{
	int32_t strpos = 0;
	volatile uint8_t i;
	rpl_dag_t *dag;
	rpl_parent_t *parent;

	size_t len = 0;
	uint8_t index;
	const char *pstr;
	uint8_t count;

	dag = rpl_get_any_dag();
	
	if(dag != NULL) 
	{

		/* count the number of routes and return the total */
		count = 0;
		for (parent = dag->preferred_parent, i = 0; parent != NULL; parent = parent->next) 
		{
			count++;
		}

		if ((len = REST.get_query_variable(request, "index", &pstr))) 
		{

			index = (uint8_t)atoi(pstr);

			if (index >= count) 
			{
				strpos = snprintf((char *)buffer, preferred_size, "{}");
			} else 

			{
				/* seek to the route entry and return it */
				i = 0;
				for (parent = dag->preferred_parent, i = 0; parent != NULL; parent = parent->next, i++) 
				{
					if (i == index) // if the preferred parent is the actual parent itself
					{
						break;
					}
				}

				if (parent == dag->preferred_parent) 
				{
					strpos = create_parent_msg((char *)buffer, parent, 1); // if preferred parent then 1 or else 0
				} else 
				{
					strpos = create_parent_msg((char *)buffer, parent, 0);
				}
			}

			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);

		} else 
		{ /* index not provided */
			strpos += snprintf((char *)buffer, preferred_size, "%d", count);
		}

	} else
	{ /* no DAG */
		strpos += snprintf((char *)buffer, preferred_size, "{\"err\": \"no DAG\"}");
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
	}

	//*offset = -1;
	REST.set_response_payload(response, buffer, strpos);

}

/*defined in rplinfo.h*/
void
rplinfo_activate_resources(void) 
{
  rest_activate_resource(&parents, "rplinfo/parents");
  rest_activate_resource(&routes, "rplinfo/routes");

}


