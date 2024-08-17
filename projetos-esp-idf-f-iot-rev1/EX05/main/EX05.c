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

volatile int cnt=0;

/* Cada vez que Button_1 ou Button_2 for pressionado, esta função de IRQ será chamada*/
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //Identifica qual o botão que foi pressionado
    if( BUTTON_1 == (uint32_t) arg )
	{
		//Caso o botão 1 estiver pressionado, faz a leitura e o acionamento do led.
		 if( gpio_get_level( (uint32_t) arg ) == 0) {
			gpio_set_level( LED_1, cnt%2 );
				
		} 		
	
	} else 
    if( BUTTON_2 == (uint32_t) arg )
	{
		 if( gpio_get_level( (uint32_t) arg ) == 0) {
			gpio_set_level( LED_2, cnt%2 );	
		} 		
	
	}   
	
	cnt++;	
}

void Task_LED( void *pvParameter )
{
    gpio_config_t io_conf; //Cria a variável descritora para o drive GPIO.
	
	//Configura o descritor de Outputs (Leds).
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
	//O flag ESP_INTR_FLAG_DEFAULT tem a ver com a alocação do vetor de interrupção, que neste caso o valor Zero (0) informa para alocar no setor de interrupção não compartilhado de nível 1, 2 ou 3. 
	
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	//Registra a interrupção externa de BUTTON_1
    gpio_isr_handler_add(BUTTON_1, gpio_isr_handler, (void*) BUTTON_1); 
	//idem para BUTTON_2
    gpio_isr_handler_add(BUTTON_2, gpio_isr_handler, (void*) BUTTON_2); 
	
    printf("Interrupcao das GPIOs configurada.\n");
    
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
