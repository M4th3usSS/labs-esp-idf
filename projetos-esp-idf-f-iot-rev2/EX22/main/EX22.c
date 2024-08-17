/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  Espressif SDK-IDF: v3.2
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
/**
 * Lib C
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * Log
 */
#include "esp_system.h"
#include "esp_log.h"

/**
 * Uart
 */
#include "driver/uart.h"

// Error library
#include "esp_err.h"

/**
 * Definições
 */
#define DEBUG 1
#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUFFER_UART_SIZE 1024

/**
 * Variáveis
 */
static const char *TAG = "main: ";

/**
 * Protótipos
 */
static void echo_task( void *pvParameter );
void app_main( void ); 

/**
 * task responsável pela inicialização da Uart1 do ESP32; 
 */
static void echo_task( void *pvParameter )
{
    /* Configuração do descritor da UART */
    uart_config_t uart_config = 
    {
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
    uart_param_config( UART_NUM_1, &uart_config );
	
	/* Definimos quais os pinos que serão utilizados para controle da UART_NUN_1 */
    uart_set_pin( UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS );
	
	/* Define o buffer de recepção (RX) com 1024 bytes, e mantem o buffer de transmissão em zero. Neste caso, 
	   a task permanecerá bloqueada até que o conteúdo (charactere) seja transmitido. O três últimos parametros
	   estão relacionados a interrupção (evento) da UART.*/
	   
    uart_driver_install( UART_NUM_1, BUFFER_UART_SIZE, 0, 0, NULL, 0 );

    /**
     * Reserva buffer Uart para recepção serial;
     */
    unsigned char * buffer_uart = (unsigned char *) pvPortMalloc( BUFFER_UART_SIZE );
    /**
     * A alocação dinâmica foi realizada com sucesso?
     */
    if( buffer_uart == NULL )
    {
        if( DEBUG )
          ESP_LOGI( TAG, "error - nao foi possivel alocar buffer_uart.\r\n" );

        vTaskDelete( NULL );
        return;
    }

    for(;;) 
    {
        
		    /* Leitura da UART */
        int len = uart_read_bytes( UART_NUM_1, buffer_uart, BUFFER_UART_SIZE, 20/portTICK_RATE_MS );
        
		    /* Envia pela UART os mesmos  bytes que foram recebidos */
        uart_write_bytes( UART_NUM_1, (const char *) buffer_uart, len );
		
		    /* Imprime no console os bytes recebidos pela UART */
    		if( len > 0 )
    		{
            if( DEBUG )
              ESP_LOGI( TAG, "%.*s", len, buffer_uart );
    			  fflush( stdout ); 
    		}
    }

    /**
     * O programa não deveria executar as linhas seguintes...
     */
    vPortFree( buffer_uart );
    vTaskDelete( NULL );
}

/**
 * Início da Aplicação;
 */
void app_main( void )
{
    if( xTaskCreate( echo_task, "uart_echo_task", 4098, NULL, 1, NULL) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar echo_task.\r\n" );  
      return;   
    }
}
