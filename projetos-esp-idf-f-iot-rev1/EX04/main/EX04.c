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

void Task_LED( void *pvParameter )
{
    int cnt=0;
	
	
    gpio_config_t io_conf; //Cria a variável descritora para o drive GPIO.
	
	//Configura o descritor de Outputs (Leds).
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_OUTPUT;  //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //Informa quais os pinos que serão configurados pelo drive. 
    gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
	
	//Configura o descritor das Inputs (botões)
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_INPUT;  //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; //Informa quais os pinos que serão configurados no drive. 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; //ou  GPIO_PULLDOWN_ENABLE
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  //ou GPIO_PULLUP_DISABLE
    gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
    printf("Pisca LED_1 e LED_2\n");
    
    while(1) {
		
		if( (gpio_get_level( BUTTON_1 ) == 0) || (gpio_get_level( BUTTON_2 ) == 0) ) {
			gpio_set_level( LED_1, cnt%2 );
			gpio_set_level( LED_2, cnt%2 );
			cnt++;	 
		}
		vTaskDelay( 300/portTICK_PERIOD_MS );
    }
}

void app_main()
{	
    xTaskCreate( Task_LED, "Task_LED", 2048, NULL, 2, NULL );
    printf( "Task_LED Iniciada com sucesso.\n" );
}
