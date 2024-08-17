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
  * Definições
  */
#define TRUE          1 
#define DEBUG         1
#define LED_BUILDING  GPIO_NUM_2 
#define LED_2         GPIO_NUM_18  
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_BUILDING) | (1ULL<<LED_2))

/**
 * Protótipos
 */
void task_blink( void *pvParameter );
void app_main( void );

/**
 * Variáveis Globais
 */
static const char * TAG = "main: ";
 

/**
 * Task Responsável em Inverter o estado de um pino do ESP32
 */
void task_blink( void *pvParameter )
{
    int cnt=0;

    if( DEBUG )
      ESP_LOGI( TAG, "task_blink run...\r\n" );		

    /**
     * Declara a variável descritora para o drive de GPIO;
     */
    gpio_config_t io_conf; 

    /**
     * Desabilita o recurso de interrupção externa das GPIO's a serem configuradas;
     */
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; 

    /**
     * Configura todas as GPIO's a serem configuradas como saída;
     */
    io_conf.mode = GPIO_MODE_OUTPUT;  

    /**
     * Informa quais as GPIO's que serão configuradas; 
     */
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; 

    /*	
      Pull-Up e Pull-Down somente são aplicados quando os pinos forem configurados como entrada
      Portanto, os comandos a seguir permanecem comentados.
    */ 
    //io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    //io_conf.pull_up_en = GPIO_PULLUP_DISABLE;  

    /**
     * Configura as GPIO's conforme configuração do descritor. 
     */
    gpio_config( &io_conf ); 
	
    
    for(;;) 
    {  
  		gpio_set_level( LED_BUILDING, cnt % 2 );
  		gpio_set_level( LED_2, cnt % 2 );
  		cnt++;	 

  		vTaskDelay( 300/portTICK_PERIOD_MS );
    }
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