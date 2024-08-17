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
 * WiFi
 */
#include "esp_wifi.h"
/**
 * WiFi Callback
 */
#include "esp_event_loop.h"
/**
 * Log
 */
#include "esp_system.h"
#include "esp_log.h"
/**
 * NVS
 */
#include "nvs.h"
#include "nvs_flash.h"

/**
 * Definições Gerais
 */
#define DEBUG 1

#define BUTTON_0	GPIO_NUM_16 
#define BUTTON_1	GPIO_NUM_17

#define RELE_0		GPIO_NUM_2 
#define RELE_1		GPIO_NUM_19 

/**
 * Variáveis
 */
static char status_rel0=0; 
static char status_rel1=0; 
static const char *TAG = "main: ";

/**
 * Protótipos
 */
static void task_button( void *pvParameter );
void app_main( void ); 

/**
 * Task responsável pelos acionamentos dos relés e pelo
 * controle da NVS;
 */
static void task_button( void *pvParameter )
{
    int aux0 = 0; 
    int aux1 = 0;

	if( DEBUG )
		ESP_LOGI( TAG, "task_button run.\n" );
		
    for(;;) 
	{

	   /**
	    * Button_0 foi pressionado?
	    */
	   if(gpio_get_level( BUTTON_0 ) == 0 && aux0 == 0 )
		{ 
			/**
			 * Aguarda 80ms devido o bounce;
			 */
		  	vTaskDelay( 80/portTICK_PERIOD_MS );	

		  	/**
		  	 * Button_0 foi realmente pressionado? 
		  	 */
		  	if( gpio_get_level( BUTTON_0 ) == 0 && aux0 == 0 ) 
		  	{

		        /**
		         * Cada vez que button_0 for pressionado, a variável status_rel0
		         * será incrementada e vpino irá assumir valor 1 ou 0;
		         */
				int vpino = (status_rel0++) % 2;
				
				/**
				 * E assim, será invertido o estado lógico de RELE_0;
				 */
				gpio_set_level( RELE_0, vpino );
			 
			 	/**
			 	 * Salva na memória NVS na chave RL0 o valor de vpino;
			 	 */
				nvs_handle my_handle;
				esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
				if ( err != ESP_OK ) {					
						if( DEBUG )
							ESP_LOGI( TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
				} else {
							
					err = nvs_set_i32(my_handle, "RL0", vpino );
					if(err != ESP_OK) {
						if( DEBUG )
							ESP_LOGI( TAG, "\nError in nvs_set_str! (%04X)\n", err);			
					}
					err = nvs_commit(my_handle);
					if(err != ESP_OK) {
						if( DEBUG )
							ESP_LOGI( TAG, "\nError in commit! (%04X)\n", err);
						
					}
					nvs_close(my_handle);	
				}

		  		if( DEBUG )
					ESP_LOGI( TAG, "RELE_0 = %d.\r\n", vpino );

				aux0 = 1; 
		 	}
		}

		/**
		 * Button_0 foi solto?
		 */
		if( gpio_get_level( BUTTON_0 ) == 1 && aux0 == 1 )
		{
			/**
			 * Aguarda 80ms devido o bounce;
			 */
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			
			/**
			 * Realmente button_0 foi solto? 
			 */
			if( gpio_get_level( BUTTON_0 ) == 1 && aux0 == 1 )
			{
				aux0 = 0;
			}
		}	
		
        //---------------------------------------------------
		/**
		 * Button_1 pressionado?
		 */
	    if( gpio_get_level( BUTTON_1 ) == 0 && aux1 == 0 )
		{ 
			/**
			 * Aguarda 80ms devido o bounce;
			 */
			vTaskDelay( 80/portTICK_PERIOD_MS );	
			 
			/**
			 * Button_1 realmente pressionado?
			 */
			if( gpio_get_level( BUTTON_1 ) == 0 && aux1 == 0 ) 
			{   
			    
				int vpino = (status_rel1++) % 2;
				/**
				 * Altera o estado lógido de RELE_1;
				 */
				gpio_set_level( RELE_1, vpino );
							
				/**
				 * Salva na NVS o valor da GPIO que controla rele_1;
				 */
				nvs_handle my_handle;
				esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
				if ( err != ESP_OK ) {					
						if( DEBUG )
							ESP_LOGI( TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
				} else {					
					err = nvs_set_i32( my_handle, "RL1", vpino );
					if(err != ESP_OK) {
						if( DEBUG )
							ESP_LOGI( TAG, "\nError in nvs_set_str! (%04X)\n", err);			
					}
					err = nvs_commit( my_handle );
					if(err != ESP_OK) {
						if( DEBUG )
							ESP_LOGI( TAG, "\nError in commit! (%04X)\n", err);
						
					}
					nvs_close(my_handle);	
				}

				if( DEBUG )
					ESP_LOGI( TAG, "\nRELE_1 = %d\r\n", vpino );
				
				aux1 = 1; 
			}
		}

		/**
		 * Button_1 solto?
		 */
		if( gpio_get_level( BUTTON_1 ) == 1 && aux1 == 1 )
		{
			/**
			 * Aguarda 80ms devido o bounce;
			 */
		    vTaskDelay( 80/portTICK_PERIOD_MS );

			if(gpio_get_level( BUTTON_1 ) == 1 && aux1 == 1 )
			{
				aux1 = 0;
			}
		}	
	
		/**
		 * Bloqueia esta task para dar oportunidade de outras tasks de menor
		 * prioridade serem executadas;
		 */
        vTaskDelay( 100/portTICK_PERIOD_MS );
    }
}

/**
 * Inicio da Aplicação;
 */
void app_main( void )
{

    /*
		Inicialização da memória não volátil para armazenamento de dados (Non-volatile storage (NVS)).
		**Necessário para realização da calibração do PHY. 
	*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    /* Configura a GPIO 16 como entrada */ 
	gpio_pad_select_gpio( BUTTON_0 );	
    gpio_set_direction( BUTTON_0, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON_0, GPIO_PULLUP_ONLY ); 
	
    /* Configura a GPIO 17 como entrada */ 
	gpio_pad_select_gpio( BUTTON_1 );	
    gpio_set_direction( BUTTON_1, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON_1, GPIO_PULLUP_ONLY ); 
	
    /* Configura a GPIO 18 como saída */ 
	gpio_pad_select_gpio( RELE_0 );
    gpio_set_direction( RELE_0, GPIO_MODE_OUTPUT );

    /* Configura a GPIO 19 como saída */ 
	gpio_pad_select_gpio( RELE_1 );
    gpio_set_direction( RELE_1, GPIO_MODE_OUTPUT );
	
    int pin; 
	
	/**
	 * NVS é um setor da memória flash do ESP32 reservado para o armazenamento
	 * de dados não voláteis. 
	 * NVS funciona da seguinte forma: 
	 * 
	 * 1- Inicialmente é necessário abrir a seção NVS para ter acesso ao Handle;
	 */
    nvs_handle my_handle;
	esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
	if ( err != ESP_OK ) {					
			if( DEBUG )
				ESP_LOGI( TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
	} else {
	    /**
	     * Verifica se a chave "RL0" existe na seção storage. Caso exista seu valor
	     * é lido e armazenado na variável pin;
	     */
		err = nvs_get_i32( my_handle, "RL0", &pin );
		if( err != ESP_OK ) {							
			if( err == ESP_ERR_NVS_NOT_FOUND ) {
				if( DEBUG )
					ESP_LOGI( TAG, "\nKey RL0 not found\n" );
				/**
				 * Caso RL0 não exista na seção, então seta em nível 0 o RELE_0;
				 */
				gpio_set_level( RELE_0, 0 );
			} 							
		} else {
			if( DEBUG )
				ESP_LOGI( TAG, "\nGPIO RELE_0 is %d\n", pin );
			/**
			 * Caso RL0 exista, então o valor de pin é carregado em RELE_0;
			 */
			gpio_set_level(RELE_0, pin);
			status_rel0 = pin;
		}


	    /**
	     * Verifica se a chave "RL1" existe na seção storage. Caso exista seu valor
	     * é lido e armazenado na variável pin;
	     */

		err = nvs_get_i32(my_handle, "RL1", &pin);
		if( err != ESP_OK ) {							
			if( err == ESP_ERR_NVS_NOT_FOUND ) {
				if( DEBUG )
					ESP_LOGI( TAG, "\nKey RL1 not found\n");
				/**
				 * Caso RL0 não exista na seção, então seta em nível 0 o RELE_0;
				 */
				gpio_set_level(RELE_1, 0);
			} 							
		} else {
			if( DEBUG )
				ESP_LOGI( TAG, "\nGPIO RELE_1 is %d\n", pin );
			/**
			 * Caso RL0 exista, então o valor de pin é carregado em RELE_0;
			 */			
			gpio_set_level(RELE_1, pin);
			status_rel1 = pin;
		}

		/**
		 * É obrigatório fechar a seção em NVS;
		 */
		nvs_close(my_handle);		
	}
	
	/**
	 * Task responsável em ligar/desligar os relés por meio de botões. 
	 * A cada mudança de estado das GPIOs dos relés os valores dessas GPIOs são 
	 * salvos em NVS; Ao ligar o MCU os últimos valores das GPIOs são lidos da
	 * NVS e carregados novamente nas GPIOs que controlam os relés;
	 */
    if( xTaskCreate( task_button, "task_button", 4098, NULL, 1, NULL ) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_button.\r\n" );  
      return;   
    }

}
