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
#define timeout_usec 10000  

//Declaración de ficheros
FILE *config_file;
FILE *log_file;

//ID de dispositivo
char ID;

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Enviar una cadena de caracteres por el puerto serie
void serial_send_string(char * msg);

////////////////////////////////////////////////////////////
// Declaracion de Variables Globales ...
int serial_fd; // descriptor del puerto serie
int timeout_ocurred=1; //variable para guadar el retorno de la lectura 

char buffer_in[7]; //Buffer para almacenar trama recibida
char buffer_out[7]; //Buffer para almacenar trama a enviar

unsigned int previous_millis1, previous_millis2, current_millis; //Variables para medir el tiempo de ejecución del programa
unsigned int interval1 = 100, interval2 = 5000; //Intervalos de tiempo para cada acción
////////////////////////////////////////////////////////////
// ******** Programa PRINCIPAL ****************************
int main (int argc, char* argv[]){

    // Inicializa la libreria wiringPi
	wiringPiSetup();

    // Abre el puerto serie
    serial_fd=serial_open("/dev/ttyUSB0", B9600); 
    if (serial_fd==-1) {    // Error en apertura?
        printf ("Error abriendo el puerto serie\n");
        fflush(stdout);
        exit(0);
    }

    //Abrir fichero "config.txt"
    config_file = fopen("config.txt", "r");
    if (config_file == NULL) {
        printf("No exite fichero de configuración\n");
        fflush(stdout);
        exit(0);
    }    
    //Leer número identificador de "config.txt"
    fscanf(config_file, "%d", &ID);
    //Cerrar fichero config
    fclose(config_file);

    //Detectar qué raspberry hay conectada con comando Hola

    //Mostrar listado en pantalla 

    //Elegir 2 de ellas

    //Iniciar contador de tiempo de ejecución
    previous_millis1 = millis();
    previous_millis2 = millis();

    while(1) {
        //Obtener tiempo actual de programa
        current_millis = millis();

        //Cada 100 ms deberá leer edl estado de los pulsadores de las dos tarjetas elegidas.
        if((current_millis - previous_millis1) >= interval1) {
            //Si S1 de una tarjeta está pulsado, encenfder la luz amarilla de la otra tarjeta, y si no está pulsado, apagar.

            //Igual, el pulsador S2 de una tarjeta actua sobre luz roja de la otra.
            previous_millis1 = millis();
        }


        //cada 5 segundos deberá mostrar en pantalla el nivel de uso y la temperatura de las CPU de las dos tarjetas esclavas elegidas.
        if((current_millis - previous_millis2) >= interval2) {
            //Acción
            previous_millis2 = millis();
        }
    }
}


/////////////////////////////////////////////////////////
// **************** FUNCIONES ///////////////////////////

// Envia una cadena de caracteres terminada en '\0' por el puerto serie
void serial_send_string(char * msg){
    int i;
    i = 0;
    while (msg[i] != '\0'){
        // Transmitir dato (caracter)
        serial_send(serial_fd, &msg[i] ,1);
        i++;
    }
}