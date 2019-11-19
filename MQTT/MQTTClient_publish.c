#include "MQTTClient_publish.h"

char *username = "RPI_master";
char *password = "raspberry";
MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

int rc;

void init_MQTT() {
    MQTTClient_create(&client, ADDRESS, CLIENTID,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = username;
    conn_opts.password = password;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
}

int send_string_MQTT(char *msg, char *topic) {
    pubmsg.payload = msg;
    pubmsg.payloadlen = (int)strlen(msg);

    MQTTClient_publishMessage(client, topic, &pubmsg, &token);

    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    return rc;
}
