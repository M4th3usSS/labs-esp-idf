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
#include "driver/gpio.h"
 
void Task_LED( void *pvParameter ); 

#define LED_1	16 
#define LED_2	17    
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_1) | (1ULL<<LED_2))

#define BUTTON_1	18 
#define BUTTON_2	19 
#define GPIO_INPUT_PIN_SEL  ((1ULL<<BUTTON_1) | (1ULL<<BUTTON_2))

#define ESP_INTR_FLAG_DEFAULT 0

volatile int cnt_1=0;
volatile int cnt_2=0;

/* Cada vez que Button_1 ou Button_2 for pressionado, esta função de IRQ será chamada*/
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //Identifica qual o botão que foi pressionado
    if( BUTTON_1 == (uint32_t) arg )
	{
		//Caso o botão 1 estiver pressionado, faz a leitura e o acionamento do led.
		 if( gpio_get_level( (uint32_t) arg ) == 0) {
			gpio_set_level( LED_1, cnt_1%2 );
				
		} 		
	
	} else 
    if( BUTTON_2 == (uint32_t) arg )
	{
		 if( gpio_get_level( (uint32_t) arg ) == 0) {
			gpio_set_level( LED_2, cnt_2%2 );	
		} 		
	
	}   
	
	cnt_1++;
	cnt_2++;	
}
 
void Task_LED( void *pvParameter )
{
	gpio_config_t io_conf = {
		.intr_type = GPIO_PIN_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = GPIO_OUTPUT_PIN_SEL
	};	
	gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
	gpio_config_t io_conf2 = {
		.intr_type = GPIO_INTR_NEGEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = GPIO_INPUT_PIN_SEL,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE
	};
    gpio_config(&io_conf2); //Configura a(s) GPIO's conforme configuração do descritor.

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add( BUTTON_1, gpio_isr_handler, (void*) BUTTON_1 ); 
    gpio_isr_handler_add( BUTTON_2, gpio_isr_handler, (void*) BUTTON_2 ); 
	
    printf("Pisca LED_1 e LED_2\n");
    
    while(1) {		  
	    //Aguardando a interrupção das GPIOs serem geradas...
		vTaskDelay( 300/portTICK_PERIOD_MS );
    }
}
 
void app_main()
{	
    xTaskCreate( Task_LED, "Task_LED", 2048, NULL, 2, NULL );
    printf( "Task_LED Iniciada com sucesso.\n" );
}