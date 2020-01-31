# Comunicación serie RS-485 con Raspberry Pi y Gateway MQTT
**Trabajo para la asignatura Diseño Electrónico Avanzado del Máster en Ingeniería Mecatrónica de la UPV.**

Desarrollo de un protocolo de comunicación entre 3 Raspberry Pi sobre un bus serie **RS-485** multipunto con programas de esclavos y maestro. Se incluye un programa alternativo que convierte la aplicación maestro en un **Gateway MQTT**. De manera que se conecta a un aplicación web en **Nodejs**, que tiene embebido un **Broker MQTT** y almacena los mensajes recibidos en una base de datos NOSQL, **MongoDB**.

![alt text](https://i.gyazo.com/f95a6b22beb546eba2f6d527603ba8a3.png)

## Protocolo de comunicación
El protocolo utilizado tiene las siguientes directrices:
![alt text](https://i.gyazo.com/0b5d97e394ed66b070af2de1efc92634.png)

En la presentación se explica detalladamente el proyecto.
