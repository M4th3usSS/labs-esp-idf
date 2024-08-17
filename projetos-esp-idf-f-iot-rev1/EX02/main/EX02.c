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

#define LED		GPIO_NUM_16 
#define BUTTON	GPIO_NUM_17 

/*
	-----------------------------------------------
	GPIO_MODE_OUTPUT       //Saída
	GPIO_MODE_INPUT        //Entrada
	GPIO_MODE_INPUT_OUTPUT //Dreno Aberto
	-----------------------------------------------
    GPIO_PULLUP_ONLY,               // Pad pull up            
    GPIO_PULLDOWN_ONLY,             // Pad pull down          
    GPIO_PULLUP_PULLDOWN,           // Pad pull up + pull down
    GPIO_FLOATING,                  // Pad floating  
*/
void Task_LED( void *pvParameter )
{
    int cnt=0;
	
    /* Configura a GPIO 16 como saída */ 
	gpio_pad_select_gpio( LED );
    gpio_set_direction( LED, GPIO_MODE_OUTPUT );
		
    /* Configura a GPIO 17 como saída */ 
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 
	
    printf("Pisca LED com GPIO 16\n");
    
    while(1) {
	
	  if(gpio_get_level( BUTTON ) == 0)
      { 	  
		gpio_set_level( LED, cnt%2 );
		cnt++;	
	  }
        
      vTaskDelay( 300/portTICK_PERIOD_MS );
    }
}

void app_main()
{	
    xTaskCreate( Task_LED, "blink_task", 1024, NULL, 5, NULL );
    printf( "Task_LED Iniciada com sucesso.\n" );
}
