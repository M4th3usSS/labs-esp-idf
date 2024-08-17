/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

// Error library
#include "esp_err.h"

#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

static void echo_task()
{
    /* Configuração do descritor da UART */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
	/* Define qual a UART a ser configurada. Neste caso, a UART_NUM_1 */ 
	/* No ESP32 há três UART, sendo a UART_NUM_0 utilizada para o debug, enquanto a UART_NUM_1 e UART_NUM_2 
	   estão livres para uso geral. 
	*/
    uart_param_config(UART_NUM_1, &uart_config);
	
	/* Definimos quais os pinos que serão utilizados para controle da UART_NUN_1 */
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
	
	/* Define o buffer de recepção (RX) com 1024 bytes, e mantem o buffer de transmissão em zero. Neste caso, 
	   a task permanecerá bloqueada até que o conteúdo (charactere) seja transmitido. O três últimos parametros
	   estão relacionados a interrupção (evento) da UART.*/
	   
    uart_driver_install(UART_NUM_1, BUF_SIZE * 1, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        
		/* Leitura da UART */
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
		/* Envia pela UART os mesmos  bytes que foram recebidos */
        uart_write_bytes(UART_NUM_1, (const char *) data, len);
		
		/* Imprime no console os bytes recebidos pela UART */
		if(len>0)
		{
			printf("%.*s", len, data);
			fflush(stdout); 
		}
    }
}

void app_main()
{
    xTaskCreate(echo_task, "uart_echo_task", 4098, NULL, 1, NULL);
}
