/* Fichero: slave.c
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
//GPIO
#define S1 8
#define S2 9
#define RED1 13
#define YEL1 14

//Declaración de ficheros
FILE *temperature_file;
FILE *cpu_usage_file;
FILE *config_file;
FILE *log_file;

//ID de dispositivo
char ID;

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Enviar una cadena de caracteres por el puerto serie
void serial_send_string(char * msg);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Lectura de temperatura de CPU
char CPU_temp (void);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Lectura de carga de CPU
char CPU_usage (void);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Lectura de pulsador
char button_S1_read (void);
char button_S2_read (void);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Cambiar estado LED
void change_LEDR (int LED, char state);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Comprobación checksum
int checksum(char *buff);

////////////////////////////////////////////////////////////
// Declaracion de la funcion prototipo...
// Enviar mensaje a maestro
void send_reply(char tx, char rx, char cmd, char data_1, char data_2);

////////////////////////////////////////////////////////////
// Declaracion de Variables Globales ...
int serial_fd; // descriptor del puerto serie
int timeout_ocurred=1; //variable para guadar el retorno de la lectura 

char buffer_in[7]; //Buffer para almacenar trama recibida
char buffer_out[7]; //Buffer para almacenar trama a enviar

////////////////////////////////////////////////////////////
// ******** Programa PRINCIPAL ****************************
int main (int argc, char* argv[]){

    // Inicializa la libreria wiringPi
	wiringPiSetup();

    // Configura direccion de cada pin
	pinMode(YEL1, OUTPUT);
	pinMode(RED1, OUTPUT);
	pinMode(S1, INPUT);
    pinMode(S2, INPUT);
	
	// Apaga todos los LEDs
	digitalWrite(YEL1,LOW);
	digitalWrite(RED1,LOW);
	
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
        printf("No exite fichero de configuración\n");
        fflush(stdout);
        exit(0);
    }    

    log_file = fopen("log.txt", "w");
    //Leer número identificador de "config.txt"
    fscanf(config_file, "%d", &ID);

    //Cerrar fichero config
    fclose(config_file);

    while(1){
        //Recibir trama de 6 bytes por puerto serie y almacenar en buffer
        timeout_ocurred = serial_read(serial_fd, buffer_in, 6, timeout_usec);
        if (timeout_ocurred!=0) { // Si no hubo timeout ...
            //Guardar en "log.txt" la trama recibida
            fprintf(log_file, "%X %X %X %X %X %X\n", buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5], buffer_in[6]);

            //Comprobar checksum de la trama (si no es correcto descartarla, borra buffer y poner en escucha de nuevo)
            if(checksum(buffer_in)){
                //Si el checksum es correcto, comprobar dirección de destino es la de este esclavo (si no es correcto descartarla, borra buffer y poner en escucha de nuevo)
                if(buffer_in[1] == ID) {
                    
                    //Si la dirección es la correcta, procesar trama (comando y datos si el comando lo indica)
                    switch (buffer_in[2])
                    {
                    case 0x00: // Hola
                        send_reply(ID, buffer_in[0], 0x00, 0x00, 0x00);
                        break;
                    case 0x10: // cambiar estado LED rojo
                        change_LED(RED1, buffer_in[4]);
                        send_reply(ID, buffer_in[0], 0x10, 0x00, 0x00);
                        break;
                    case 0x12: // cambiar estado LED amarillo
                        change_LED(YEL1, buffer_in[4]);
                        send_reply(ID, buffer_in[0], 0x12, 0x00, 0x00);
                        break;
                    case 0x30: // Leer estado de pulsador S1
                        send_reply(ID, buffer_in[0], 0x30, 0x00, button_S1_read());
                        break;    
                    case 0x32: // Leer estado de pulsador S2
                        send_reply(ID, buffer_in[0], 0x32, 0x00, button_S2_read());
                        break; 
                    case 0x40: // Leer carga de CPU (%)
                        send_reply(ID, buffer_in[0], 0x40, 0x00, CPU_usage());
                        break;
                    case 0x42: //Leer temperatura de CPU (ºC)
                        send_reply(ID, buffer_in[0], 0x42, 0x00, CPU_temp());
                        break;
                    default:
                        break;
                    }
                }
            }
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

// Lectura de temperatura de CPU
char CPU_temp (void) {
    float T;

    temperature_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temperature_file == NULL) {
        T = 0;
    } else {
        fscanf(temperature_file, "%f", &T);
        T /= 1000;
    }
    fclose(temperature_file);
    return (char)T;  
}

// Lectura de carga de CPU
char CPU_usage (void) {
    float load;

    cpu_usage_file = fopen("/proc/loadavg", "r");
    if(cpu_usage_file == NULL) {
        load = 0;
    } else {
        fscanf(cpu_usage_file, "%f", &load);
        load *= 100;
    }
    fclose(cpu_usage_file);
    return (char)load;
}

// Lectura de pulsador
char button_S1_read (void) {
    (digitalRead(S1) == 0) ? return 1 : return 0;
}

char button_S2_read (void) {
    (digitalRead(S2) == 0) ? return 1 : return 0;
}

// Cambiar estado LED
void change_LEDR (int LED, char state) {
    digitalWrite(LED, state);
}

//Comprobación de checksum
int checksum(char *buff) {
    char cks;
    cks = (buff[0] + buff[1] + buff[2] + buff[3] + buff[4]) % 256;
    (cks == buff[5]) ? return 1 : return 0;
}

// Enviar mensaje a maestro
void send_reply(char tx, char rx, char cmd, char data_1, char data_2) {

    buffer_out[0] = tx;
    buffer_out[1] = rx;
    buffer_out[2] = cmd + 1;
    buffer_out[3] = data_1;
    buffer_out[4] = data_2;
    buffer_out[5] = buffer_out[0] + buffer_out[1] + buffer_out[2] + buffer_out[3] + buffer_out[4]) % 256;
    buffer_out[6] = '\0';

    serial_send_string(buffer_out);
}

/////////////////////////////////////////////////////////
///////// FIN DEL CODIGO ////////////////////////////////
/////////////////////////////////////////////////////////