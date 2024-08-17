/* --------------------------------------------------------------------------
  Autor: Prof° Fernando Simplicio;
  Hardware: NodeMCU ESP32
  Espressif SDK-IDF: v3.2
  Curso: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
 *  --------------------------------------------------------------------------

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/**
 * Standard C lib
 */
#include <stdio.h>

/**
 * FreeRTOs
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * Drivers
 */
#include "driver/gpio.h"

/**
 * Logs 
 */
#include "esp_log.h"


/**
 * Definições;
 */
#define TRUE          	1 
#define DEBUG         	1

#define LED_1 	2    
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<LED_1)

#define BUTTON_1	17 
#define BUTTON_2	18 
#define GPIO_INPUT_PIN_SEL  ((1ULL<<BUTTON_1) | (1ULL<<BUTTON_2))

/**
 * Protótipos
 */
static void IRAM_ATTR gpio_isr_handler( void *arg );
void task_blink( void *pvParameter );
void app_main( void );

/**
 * Variáveis Globais
 */
volatile int cnt_1 = 0;
volatile int cnt_2 = 0;
static const char * TAG = "main: ";

/**
 * Cada vez que Button_1 ou Button_2 for pressionado, esta função de IRQ será chamada;
 * Devido ao bounce, provavelmente irá ocorrer inúmeras interrupções externas sucessivas;
 * Porém, neste caso, não há ao que recorrer, pois a interrupção está sendo gerada 
 * por intermédio de um botão;
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    /**
     * Verifica qual foi o botão pressionado;
     */
    if( BUTTON_1 == (uint32_t) arg )
	{
		//Caso o botão 1 estiver pressionado, faz a leitura e o acionamento do led.
		if( gpio_get_level( (uint32_t) arg ) == 0) 
		{
			gpio_set_level( LED_1, cnt_1 % 2 );
		} 		
	
	} else 
    if( BUTTON_2 == (uint32_t) arg )
	{
		if( gpio_get_level( (uint32_t) arg ) == 0 ) 
		{
			gpio_set_level( LED_1, cnt_2 % 2 );	
		} 		
	
	}   
	
	cnt_1++;
	cnt_2++;	
}
 
void task_blink( void *pvParameter )
{
    if( DEBUG )
      ESP_LOGI( TAG, "task_blink run...\r\n" );

  	/**
  	 * Configura o descritor da GPIO led; 
  	 */
	gpio_config_t io_conf = {
		.intr_type = GPIO_PIN_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = GPIO_OUTPUT_PIN_SEL
	};	
	gpio_config( &io_conf ); 
	
	/**
	 * Configura o descritor da GPIO button;
	 */
	gpio_config_t io_conf2 = {
		.intr_type = GPIO_INTR_NEGEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = GPIO_INPUT_PIN_SEL,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE
	};
    gpio_config( &io_conf2 ); 

    /**
     * Habilita a chave geral das interrupções das GPIO's; 
     */
    gpio_install_isr_service(0);

    /**
     * Registra a função de callback da ISR (Interrupção Externa);
     * O número da GPIO é passada como parâmetro para a função de callback, 
     * pois assim, é possível saber qual foi o botão pressionado;
     */
    gpio_isr_handler_add( BUTTON_1, gpio_isr_handler, (void*) BUTTON_1 ); 
    gpio_isr_handler_add( BUTTON_2, gpio_isr_handler, (void*) BUTTON_2 ); 
	
    /**
     * Toda task normalmente possuem um loop sem saída;
     */
    for(;;) 
    {
	    //Aguardando o acionamento das interrupções externas das GPIOs;
		vTaskDelay( 300/portTICK_PERIOD_MS );
    }

    /**
     * Espera-se que este comando nunca seja executado; 
     */
    vTaskDelete( NULL ); 
}

/**
 * Função Principal da Aplicação;
 * Chamada logo após a execução do bootloader do ESP32;
 */
void app_main( void )
{	
    /**
     * Cria a task responsável em interver o estado do led building;
     */
    if( (xTaskCreate( task_blink, "task_blink", 2048, NULL, 1, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_blink.\r\n" );  
      return;   
    }
}