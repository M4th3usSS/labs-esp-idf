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

#define LED_BUIDING		2 
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<LED_BUIDING)

#define BUTTON_1		17 
#define BUTTON_2		18 
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
static const char * TAG = "main: ";
volatile int cnt=0;


/**
 * ISR (função de callback) da Interrupção Externa de GPIOs;
 * Cada vez que Button_1 ou Button_2 for pressionado, esta função de IRQ será chamada
 */
static void IRAM_ATTR gpio_isr_handler( void* arg )
{
	/**
	 * Verifica qual foi o botão que acionou a interrupção;
	 */
	if( BUTTON_1 == (uint32_t) arg )
	{
		/**
		 * Caso o button_1 estiver pressionado o led building será acionado;
		 */
		if( gpio_get_level( (uint32_t) arg ) == 0 ) 
		{
			gpio_set_level( LED_BUIDING, cnt % 2 );		
		} 		

	} else 

	/**
	 * Caso a interrupção tenha sido gerada por button_2, então...
	 */
	if( BUTTON_2 == (uint32_t) arg )
	{
		if( gpio_get_level( (uint32_t) arg ) == 0 ) 
		{
			/**
			 * Inverte novamente o estado lógico do led building;
			 */
			gpio_set_level( LED_BUIDING, cnt % 2 );	
		} 		

	}   

	cnt++;	
}

/**
 * Task responsável pelas configurações das GPIOs do ESP32.
 */
void task_blink( void *pvParameter )
{

    if( DEBUG )
      ESP_LOGI( TAG, "task_blink run...\r\n" );

    gpio_config_t io_conf; //Cria a variável descritora para o drive GPIO.
	
	/**
	 * Configura o descritor de GPIOs do Led;
	 */
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_OUTPUT;  //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //Informa quais os pinos que serão configurados pelo drive. 
    gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
	/*
	GPIO_INTR_DISABLE = 0,     /!< Disable GPIO interrupt                             
    GPIO_INTR_POSEDGE = 1,     /! GPIO interrupt type : rising edge                  
    GPIO_INTR_NEGEDGE = 2,     /!< GPIO interrupt type : falling edge                 
    GPIO_INTR_ANYEDGE = 3,     /!< GPIO interrupt type : both rising and falling edge 
	*/
	//Configura o descritor das Inputs (botões)
	
    io_conf.intr_type = GPIO_INTR_NEGEDGE; //interrupção externa da(s) GPIO(s) habilitada e configurada para disparo na descida.
    io_conf.mode = GPIO_MODE_INPUT;  //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; //Informa quais os pinos que serão configurados no drive. 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; //ou  GPIO_PULLDOWN_ENABLE
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  //ou GPIO_PULLUP_DISABLE
    gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
	//Habilita a interrupção externa da(s) GPIO's. 
	//Ao utilizar a função gpio_install_isr_service todas as interrupções de GPIO do descritor vão chamar a mesma 
	//interrupção. A função de callback que será chamada para cada interrupção é definida em gpio_isr_handler_add. 
	//O flag ESP_INTR_FLAG_DEFAULT tem a ver com a alocação do vetor de interrupção, que neste caso o valor Zero (0) 
	//informa para alocar no setor de interrupção não compartilhado de nível 1, 2 ou 3. 
    gpio_install_isr_service(0);

	//Registra a interrupção externa de BUTTON_1
    gpio_isr_handler_add( BUTTON_1, gpio_isr_handler, (void*) BUTTON_1 ); 
	//idem para BUTTON_2
    gpio_isr_handler_add( BUTTON_2, gpio_isr_handler, (void*) BUTTON_2 ); 
	
    
    for(;;) 
    {
	    //Aguardando o acionamento das interrupções externas das GPIOs;
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
