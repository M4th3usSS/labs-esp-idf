#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

/*
SSID e Password de sua rede WiFi local.
*/
#define WIFI_SSID 	 "ESP32-AP"
#define WIFI_PASS 	 "esp32pwd" //no mínimo 8 caracteres (WPA2)

/**
 * 0-> SSID não oculto na rede.	
 * 1-> SSID oculto na rede.
 */
#define AP_SSID_HIDDEN 			   0

#define AP_IP 		"192.168.0.49"
#define AP_GATEWAY 	"192.168.0.1"
#define AP_MASCARA  "255.255.255.0"

/*Porta que será aberta pelo Servidor TCP*/
#define PORT_NUMBER 9765 
#define MAX_NUMERO_CLIENTES 5

/**
 * Definições Gerais
 */
#define DEBUG 0

#endif