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

//TIMEOUT (ms) para la recepción de datos
#define timeout_usec 1000000000  

//Macros de trama
#define Hola          0x00;
#define LEDR          0x10;
#define LEDA          0x12;
#define S1            0x30;
#define S2            0x32;
#define CPU_usage     0x40;
#define CPU_temp      0x42;

//Declaración de ficheros
FILE *config_file;
FILE *log_file;

//ID de dispositivo
char ID;
// estado de raspberry esclava
char state;

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Busqueda de raspberrys esclavas disponibles
void RPI_search(void); 
// Enviar trama por serie
void send_msg(char ID_slave, char com, char par);
// Comprobar estado de raspberry
void checkStatus(char ID, char com);
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
char RPI_IDs[21];
// Cuenta de cantidad de esclavos
char count_RPI;

////////////////////////////////////////////////////////////
// ******** Programa PRINCIPAL ****************************
int main (int argc, char* argv[]){

    // Inicializa la libreria wiringPi
	wiringPiSetup();

    // Abre el puerto serie
    serial_fd=serial_open("/dev/ttyS0", B9600); 
    if (serial_fd==-1) {    // Error en apertura?
        printf ("Error abriendo el puerto serie\n");
        fflush(stdout);
        exit(0);
    }

    //Abrir fichero "config.txt" y crear "log.txt"
    config_file = fopen("config.txt", "r");
    if (config_file == NULL) {
        printf("No exite fichero de configuración\n");
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
    printf("Se han encontrado %d raspberrys con IDs: ", (int)count_RPI);
    for (char i = 0; i < count_RPI; i++) {
        printf("%d, ", (int)RPI_IDs[i]);
    }
    fflush(stdout);
    //Elegir 2 de ellas

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
                //Enviar mensaje a la otra raspberry
                send_msg(RPI_IDs[1], LEDA, 1);
                //Enviar mensaje por MQTT (usar sprintf)
                
            } else {
                send_msg(RPI_IDs[1], LEDA, 0);
            }
            if(checkStatus(RPI_IDs[0], S2)) {
                send_msg(RPI_IDs[1], LEDR, 1);
            } else {
                send_msg(RPI_IDs[1], LEDR, 0);
            }
            if(checkStatus(RPI_IDs[1], S1)) {
                send_msg(RPI_IDs[0], LEDA, 1);
            } else {
                send_msg(RPI_IDs[0], LEDA, 0);
            }
            if(checkStatus(RPI_IDs[1], S2)) {
                send_msg(RPI_IDs[0], LEDR, 1);
            } else {
                send_msg(RPI_IDs[0], LEDR, 0);
            }

            previous_millis1 = millis();
        }


        //cada 5 segundos deberá mostrar en pantalla el nivel de uso y la temperatura de las CPU de las dos tarjetas esclavas elegidas.
        if((current_millis - previous_millis2) >= interval2) {
            state = checkStatus(RPI_IDs[0], CPU_usage);
            printf("Uso de CPU de raspberry con ID %d: %d %%\n", (int)RPI_IDs[0], (int)state);
            state = checkStatus(RPI_IDs[0], CPU_temp);
            printf("Temperatura de raspberry con ID %d: %d ºC\n", (int)RPI_IDs[0], (int)state);
            state = checkStatus(RPI_IDs[1], CPU_usage);
            printf("Uso de CPU de raspberry con ID %d: %d %%\n", (int)RPI_IDs[0], (int)state);
            state = checkStatus(RPI_IDs[1], CPU_temp);
            printf("Temperatura de raspberry con ID %d: %d ºC\n", (int)RPI_IDs[0], (int)state);

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

// Busqueda de raspberrys esclavas disponibles
void RPI_search(void) {
    //Busqueda de hasta 20 raspberrys
    for(char i = 0; i < 20, i++) {
        // Enviar mensaje a la raspberry
        send_msg(i, 0x00, 0x00);
        //Recibir respuesta
        timeout_ocurred = serial_read(serial_fd, buffer_in, 6, timeout_usec);
        //Si llega respuesta antes del timeout
        if(timeout_ocurred != 0) {
            //Guardar en "log.txt" la trama recibida
            fprintf(log_file, "%02X, %02X, %02X, %02X, %02X, %02X\n", buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5]);
			fflush(log_file);
            if(checksum(buffer_in)) {
                //Guardar en RP_IDs[] el ID de la raspberry
                RPI_IDs[count_RPI] = i;
                //Contar el nº de rasberrys encontradas
                count_RPI++;
            }
        }
    }
}

// Enviar trama por serie
void send_msg(char ID_slave, char com, char par) {
    buffer_out[0] = ID;
    buffer_out[1] = ID_slave;
    buffer_out[2] = com;
    buffer_out[3] = 0;
    buffer_out[4] = par;
    buffer_out[5] = ((int)buffer_out[0] + (int)buffer_out[1] + (int)buffer_out[2] + (int)buffer_out[3] + (int)buffer_out[4]) % 256;
    buffer_out[6] = '\0';

	serial_send(serial_fd, &buffer_out],6);*/
}

// Comprobar estado de raspberry
char checkStatus(char ID, char com) {
    // Comprobar temperatura
    send_msg(ID, com, 0x00);
    //Recibir respuesta
    timeout_ocurred = serial_read(serial_fd, buffer_in, 6, timeout_usec);
    //Si llega respuesta antes del timeout
    if(timeout_ocurred != 0) {
        //Guardar en "log.txt" la trama recibida
        fprintf(log_file, "%02X, %02X, %02X, %02X, %02X, %02X\n", buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5]);
        fflush(log_file);
        //sprintf y enviar log por mqtt

        //Comprobar checksum de la trama
        if(checksum(buffer_in)) {
            // Devolver estado
            return buffer_in[4];
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

//Comprobación de checksum
int checksum(char *buff) {
    int cks;
    cks = ((int)buff[0] + (int)buff[1] + (int)buff[2] + (int)buff[3] + (int)buff[4]) % 256;
    
    return (cks == (int)buff[5]) ? 1 : 0;
}