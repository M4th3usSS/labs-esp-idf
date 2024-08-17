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
 
#define LED_1	16  //ou GPIO_NUM_16
#define LED_2	17  //ou GPIO_NUM_17
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_1) | (1ULL<<LED_2))
 
void Task_LED( void *pvParameter )
{
    int cnt=0;
		
    gpio_config_t io_conf; //Declara a variável descritora do drive de GPIO.
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //Desabilita o recurso de interrupção neste descritor.
    io_conf.mode = GPIO_MODE_OUTPUT;  //Configura como saída. 
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //Informa quais os pinos que serão configurados pelo drive. 
	
	/*	
		Pull-Up e Pull-Down somente são aplicados quando os pinos forem configurados como entrada
		Portanto, os comandos a seguir permanecem comentados.
	*/ 
    //io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; //ou  GPIO_PULLDOWN_ENABLE
    //io_conf.pull_up_en = GPIO_PULLUP_DISABLE;  //ou GPIO_PULLUP_DISABLE
	
    gpio_config(&io_conf); //Configura a(s) GPIO's conforme configuração do descritor. 
	
    printf("Pisca LED_1 e LED_2\n");
    
    while(1) {
		  
		gpio_set_level( LED_1, cnt%2 );
		gpio_set_level( LED_2, cnt%2 );
		cnt++;	 
		vTaskDelay( 300/portTICK_PERIOD_MS );
    }
}
 
void app_main()
{	
    xTaskCreate( Task_LED, "Task_LED", 2048, NULL, 2, NULL );
    printf( "Task_LED Iniciada com sucesso.\n" );
}