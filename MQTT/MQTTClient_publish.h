#ifndef MQTTClient_publish_h
#define MQTTClient_publish_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS     "ws://rpi-gateway.herokuapp.com:80/"
#define CLIENTID    "RPI_master"
#define QOS         0
#define TIMEOUT     10000L

void init_MQTT();
int send_string_MQTT(char *msg, char *topic);

#endif
