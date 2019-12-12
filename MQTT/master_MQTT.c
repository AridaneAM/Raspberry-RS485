/* Fichero: master.c
 * Compilar con:
 * gcc -Wall serial.c -o 
 * Añadir en la opcion del compilador: serial.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include <wiringPi.h>
#include "MQTTClient_publish.h"

//TIMEOUT (ms) para la recepción de datos
#define timeout_usec 100000  

//Macros de trama
#define HOLA          0x00
#define LEDR          0x10
#define LEDA          0x12
#define S1            0x30
#define S2            0x32
#define CPU_USAGE     0x40
#define CPU_TEMP      0x42

//Declaración de ficheros
FILE *config_file;
FILE *log_file;

//ID de dispositivo
int ID;
// estado de raspberry esclava
char state;

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Enviar trama por serie
void send_msg(char ID_slave, char com, char par);
// Recibir trama por serie
char receive_msg(char com);
// Busqueda de raspberrys esclavas disponibles
void RPI_search(void); 
// Comprobar estado de raspberry
char checkStatus(char ID_slave, char com);
// Comprobación checksum
int checksum(char *buffer);

////////////////////////////////////////////////////////////
// Declaracion de Variables Globales ...
int serial_fd; // descriptor del puerto serie
int timeout_ocurred=1; //variable para guadar el retorno de la lectura 

char buffer_in[7]; //Buffer para almacenar trama recibida
char buffer_out[7]; //Buffer para almacenar trama a enviar

unsigned int previous_millis1, previous_millis2, current_millis; //Variables para medir el tiempo de ejecución del programa
unsigned int interval1 = 100, interval2 = 5000; //Intervalos de tiempo para cada acción

// Array para almacenar IDs de raspberrys encontradas
char RPI_IDs[31];
// Cuenta de cantidad de esclavos
int count_RPI = 0;

// Mensaje de envio MQTT
char mensaje_MQTT[30];
char *topic_log = "log"; 
char *topic_temp1 = "temp1";
char *topic_temp2 = "temp2";
char *topic_usage1 = "usage1";
char *topic_usage2 = "usage2";
char *topic_btn = "topicbtn";

////////////////////////////////////////////////////////////
// ******** Programa PRINCIPAL ****************************
int main (int argc, char* argv[]){

    // Inicializa la libreria wiringPi
	wiringPiSetup();

    // Inicio de conexión MQTT
	init_MQTT();

    // Abre el puerto serie
    serial_fd=serial_open("/dev/ttyUSB0", B9600); 
    if (serial_fd==-1) {    // Error en apertura?
        printf ("Error abriendo el puerto serie\n");
        fflush(stdout);
        exit(0);
    }

    //Abrir fichero "config.txt" y crear "log.txt"
    config_file = fopen("config.txt", "r");
    if (config_file == NULL) {
        printf("No existe fichero de configuración\n");
        fflush(stdout);
        exit(0);
    }    
    log_file = fopen("log.txt", "w");

    //Leer número identificador de "config.txt"
    fscanf(config_file, "%d", &ID);
    //Cerrar fichero config
    fclose(config_file);

    //Detectar qué raspberry hay conectada con comando Hola
    RPI_search();
    //Mostrar listado en pantalla 
    printf("Se han encontrado %d raspberrys con IDs: ", count_RPI);
    for (int i = 0; i < count_RPI; i++) {
        printf("%d, ", (int)RPI_IDs[i]);
    }
    printf("\b\b \b\n");																/////////////////
    fflush(stdout);
    //Iniciar contador de tiempo de ejecución
    previous_millis1 = millis();
    previous_millis2 = millis();

    while(1) {
        //Obtener tiempo actual de programa
        current_millis = millis();

        //Cada 100 ms deberá leer el estado de los pulsadores de las dos tarjetas elegidas.
        if((current_millis - previous_millis1) >= interval1) {
            //Si S1 de una tarjeta está pulsado, encender la luz amarilla de la otra tarjeta, y si no está pulsado, apagar.
            //Igual, el pulsador S2 de una tarjeta actua sobre luz roja de la otra.
            if(checkStatus(RPI_IDs[0], S1)) {
                // Enviar mensaje a la otra raspberry
                send_msg(RPI_IDs[1], LEDA, 1);
                // Esperar confirmación
                receive_msg(LEDA);
                // Enviar mensaje por MQTT
                sprintf(mensaje_MQTT, "%d", 1);
                send_string_MQTT(mensaje_MQTT, topic_btn);
                
            } else {
                send_msg(RPI_IDs[1], LEDA, 0);
                receive_msg(LEDA);
                sprintf(mensaje_MQTT, "%d", 2);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            }
            if(checkStatus(RPI_IDs[0], S2)) {
                send_msg(RPI_IDs[1], LEDR, 1);
                receive_msg(LEDR);
                sprintf(mensaje_MQTT, "%d", 3);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            } else {
                send_msg(RPI_IDs[1], LEDR, 0);
                receive_msg(LEDR);
                sprintf(mensaje_MQTT, "%d", 4);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            }
            if(checkStatus(RPI_IDs[1], S1)) {
                send_msg(RPI_IDs[0], LEDA, 1);
                receive_msg(LEDA);
                sprintf(mensaje_MQTT, "%d", 5);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            } else {
                send_msg(RPI_IDs[0], LEDA, 0);
                receive_msg(LEDA);
                sprintf(mensaje_MQTT, "%d", 6);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            }
            if(checkStatus(RPI_IDs[1], S2)) {
                send_msg(RPI_IDs[0], LEDR, 1);
                receive_msg(LEDR);
                sprintf(mensaje_MQTT, "%d", 7);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            } else {
                send_msg(RPI_IDs[0], LEDR, 0);
                receive_msg(LEDR);
                sprintf(mensaje_MQTT, "%d", 8);
                send_string_MQTT(mensaje_MQTT, topic_btn);
            }
            previous_millis1 = millis();
        }

        //cada 5 segundos deberá mostrar en pantalla el nivel de uso y la temperatura de las CPU de las dos tarjetas esclavas elegidas.
        if((current_millis - previous_millis2) >= interval2) {
            state = checkStatus(RPI_IDs[0], CPU_USAGE);
            if(state!= -1) printf("Uso de CPU de raspberry con ID %d: %d %%\n", (int)RPI_IDs[0], (int)state);
            sprintf(mensaje_MQTT, "%d", state);
            send_string_MQTT(mensaje_MQTT, topic_usage1);
            state = checkStatus(RPI_IDs[0], CPU_TEMP);
            if(state!= -1) printf("Temperatura de raspberry con ID %d: %d ºC\n", (int)RPI_IDs[0], (int)state);
            sprintf(mensaje_MQTT, "%d", state);
            send_string_MQTT(mensaje_MQTT, topic_temp1);
            state = checkStatus(RPI_IDs[1], CPU_USAGE);
            if(state!= -1) printf("Uso de CPU de raspberry con ID %d: %d %%\n", (int)RPI_IDs[1], (int)state);
            sprintf(mensaje_MQTT, "%d", state);
            send_string_MQTT(mensaje_MQTT, topic_usage2);
            state = checkStatus(RPI_IDs[1], CPU_TEMP);
            if(state!= -1) printf("Temperatura de raspberry con ID %d: %d ºC\n", (int)RPI_IDs[1], (int)state);
            sprintf(mensaje_MQTT, "%d", state);
            send_string_MQTT(mensaje_MQTT, topic_temp2);

            previous_millis2 = millis();
        }
    }

    //cerrar puerto serie
    serial_close(serial_fd);
    //cerrar fichero log
	fclose(log_file);

    return 0;
}


/////////////////////////////////////////////////////////
// **************** FUNCIONES ///////////////////////////
// Enviar trama por serie
void send_msg(char ID_slave, char com, char par) {
    buffer_out[0] = ID;
    buffer_out[1] = ID_slave;
    buffer_out[2] = com;
    buffer_out[3] = 0;
    buffer_out[4] = par;
    buffer_out[5] = ((int)buffer_out[0] + (int)buffer_out[1] + (int)buffer_out[2] + (int)buffer_out[3] + (int)buffer_out[4]) % 256;
    buffer_out[6] = '\0';

	serial_send(serial_fd, buffer_out,6);
}

// Recibir trama por serie
/* (Códigos de error:)
** 255: Timeout en recepción de datos
** 254: Checksum de la trama incorrecto
** 253: Comando del mensaje recibido no coincide con el esperado
*/
char receive_msg(char com) {
    //Recibir respuesta
    timeout_ocurred = serial_read(serial_fd, buffer_in, 6, timeout_usec);
    //Si llega respuesta antes del timeout
    if(timeout_ocurred != 0) {
        //Guardar en "log.txt" la trama recibida
        fprintf(log_file, "%02X, %02X, %02X, %02X, %02X, %02X\n", buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5]);
        fflush(log_file);
        //Enviar log por mqtt
        sprintf(mensaje_MQTT, "%02X, %02X, %02X, %02X, %02X, %02X\n", buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5]);
	    send_string_MQTT(mensaje_MQTT, topic_log);
        //Comprobar checksum de la trama
        if(checksum(buffer_in)) {
            // Comprobar comando recibido
            if(buffer_in[2] == com + 1){
                // Devolver estado
                return buffer_in[4];
            } else {
                printf("Comando de mensaje enviado (%02X) no coincide con el mensaje recibido (%02X)\n", com, buffer_in[2] - 1);
                fflush(stdout);
                return 253;
            }
        } else {
            printf("Checksum de la trama recibida incorrecto\n");
            fflush(stdout);
            return 254;
        }
    } else {
        printf("Se ha producido un timeout en la espera del mensaje\n");
        fflush(stdout);
        return 255;
    }
}

// Busqueda de raspberrys esclavas disponibles
void RPI_search(void) {
    //Busqueda de hasta 30 raspberrys
    for(int i = 1; i < 30; i++) {
        // Enviar mensaje a la raspberry
        send_msg(i, HOLA, 0x00);
        // Recibir respuesta
        if(receive_msg(HOLA) == 0) {
            // Guardar en RP_IDs[] el ID de la raspberry
            RPI_IDs[count_RPI] = i;
            // Contar el nº de rasberrys encontradas
            count_RPI++;
        }
    }
}

// Comprobar estado de raspberry
char checkStatus(char ID_slave, char com) {
    // Comprobar estado
    send_msg(ID_slave, com, 0x00);
    //Recibir respuesta
    return receive_msg(com);
}

// Comprobación de checksum
int checksum(char *buff) {
    int cks;
    cks = ((int)buff[0] + (int)buff[1] + (int)buff[2] + (int)buff[3] + (int)buff[4]) % 256;
    
    return (cks == (int)buff[5]) ? 1 : 0;
}
