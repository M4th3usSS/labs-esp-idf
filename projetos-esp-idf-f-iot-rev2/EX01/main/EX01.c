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
    int cnt = 0;  

    if( DEBUG )
      ESP_LOGI( TAG, "task_blink run...\r\n" );
    
    /**
     * Inicializa a GPIO do ESP32
     */
    gpio_pad_select_gpio( LED_BUILDING ); 
    gpio_set_direction( LED_BUILDING, GPIO_MODE_OUTPUT );

    for(;;)
    {
      /**
       * Se cnt for par retorna 0, caso contrário retorna 1;
       */
      gpio_set_level( LED_BUILDING , cnt % 2 ); 
      cnt++;

      /**
       * Delay de 1 segundo
       */
      vTaskDelay( 1000/portTICK_PERIOD_MS );
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