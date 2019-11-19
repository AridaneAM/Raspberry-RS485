//Compile		gcc -Wall -c "%f" MQTTClient_publish.c
//Build 		gcc -Wall -o "%e" "%f" -lpaho-mqtt3c MQTTClient_publish.c

#include <stdio.h>
#include "MQTTClient_publish.h"
#include <wiringPi.h>

char mensaje_MQTT[30];
char *tema = "topicprueba";

char buffer_out[7]; 

unsigned int tiempo;

int main() {
	tiempo = millis();
	buffer_out[0] = 0x01;
    buffer_out[1] = 0x02;
    buffer_out[2] = 0x23;
    buffer_out[3] = 0x04;
    buffer_out[4] = 0x05;
    buffer_out[5] = 0x06;
    buffer_out[6] = '\0';
	

	
	init_MQTT();
	
	sprintf(mensaje_MQTT, "%02X, %02X, %02X, %02X, %02X, %02X\n", buffer_out[0], buffer_out[1], buffer_out[2], buffer_out[3], buffer_out[4], buffer_out[5]);
	send_string_MQTT(mensaje_MQTT, tema);
	



	return 0;
}
