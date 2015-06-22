all: coap-server coap-client

WITH_TMP102_SENSOR=0
WITH_BUTTON_SENSOR=1

CONTIKI= ../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# Contiki IPv6 configuration
WITH_UIP6=1
UIP_CONF_IPV6=1
# IPv6 make config disappeared completely
CFLAGS += -DUIP_CONF_IPV6=1
# variable for Makefile.include
CFLAGS += -DUIP_CONF_IPV6_RPL=1

SMALL=1

# REST Engine shall use Erbium CoAP implementation
APPS += er-coap
APPS += rest-engine

ifeq ($(WITH_TMP102_SENSOR),1)
CFLAGS += -DWITH_TMP102_SENSOR=1
endif

ifeq ($(WITH_BUTTON_SENSOR),1)
CFLAGS += -DWITH_BUTTON_SENSOR=1
endif

PROJECTDIRS += rplinfo
PROJECT_SOURCEFILES += rplinfo.c

include $(CONTIKI)/Makefile.include
