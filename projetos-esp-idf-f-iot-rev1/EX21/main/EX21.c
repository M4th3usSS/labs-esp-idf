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
#include "nvs_flash.h"

#define BUTTON_0	GPIO_NUM_16 
#define BUTTON_1	GPIO_NUM_17

#define RELE_0		GPIO_NUM_18 
#define RELE_1		GPIO_NUM_19 

#define DEBUG 1

static char status_rel0=0; 
static char status_rel1=0; 

void Task_Button ( void *pvParameter )
{
    int aux0 = 0, aux1 = 0;

	if( DEBUG )
		printf( "Task_Button Iniciada com sucesso.\n" );
		
    for(;;) 
	{
	   //---------------------------------------------------
	    /* GPIO RELÉ_0 */
	   if(gpio_get_level( BUTTON_0 ) == 0 && aux0 == 0)
		{ 
		  vTaskDelay( 80/portTICK_PERIOD_MS );		  
		  if(gpio_get_level( BUTTON_0 ) == 0) //realmente o botão 0 está pressionado?
		  {
		        status_rel0++;
				int vpino = status_rel0%2;
						
				gpio_set_level( RELE_0, vpino );
				
				if( DEBUG )
					printf( "\nRELE_0 = %d\n", vpino );	    

				/*-------------------------------------------*/
				nvs_handle my_handle;
				esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
				if ( err != ESP_OK ) {					
						if( DEBUG )
							printf( "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
				} else {
					//RELE_0					
					err = nvs_set_i32(my_handle, "RL0", vpino);
					if(err != ESP_OK) {
						if( DEBUG )
							printf("\nError in nvs_set_str! (%04X)\n", err);			
					}
					err = nvs_commit(my_handle);
					if(err != ESP_OK) {
						if( DEBUG )
							printf("\nError in commit! (%04X)\n", err);
						
					}
					nvs_close(my_handle);	
				}
				/*-------------------------------------------*/
				
			aux0 = 1; 
		  }
		}

		if(gpio_get_level( BUTTON_0 ) == 1 && aux0 == 1)
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			if(gpio_get_level( BUTTON_0 ) == 1 )
			{
				aux0 = 0;
			}
		}	
		
        //---------------------------------------------------
		/* GPIO RELÉ_1 */
	   if(gpio_get_level( BUTTON_1 ) == 0 && aux1 == 0)
		{ 
			/* Delay para tratamento do bounce da tecla*/
			vTaskDelay( 80/portTICK_PERIOD_MS );	
			  
			if(gpio_get_level( BUTTON_1 ) == 0) 
			{   
			    status_rel1++;
				int vpino = status_rel1%2;
				
				
				gpio_set_level( RELE_1, vpino );
				if( DEBUG )
					printf( "\nRELE_1 = %d\n", vpino );
				
				/*-------------------------------------------*/
				nvs_handle my_handle;
				esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
				if ( err != ESP_OK ) {					
						if( DEBUG )
							printf( "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
				} else {
					//RELE_0					
					err = nvs_set_i32(my_handle, "RL1", vpino);
					if(err != ESP_OK) {
						if( DEBUG )
							printf("\nError in nvs_set_str! (%04X)\n", err);			
					}
					err = nvs_commit(my_handle);
					if(err != ESP_OK) {
						if( DEBUG )
							printf("\nError in commit! (%04X)\n", err);
						
					}
					nvs_close(my_handle);	
				}
				/*-------------------------------------------*/
				
				aux1 = 1; 
			}
		}

		if(gpio_get_level( BUTTON_1 ) == 1 && aux1 == 1)
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			if(gpio_get_level( BUTTON_1 ) == 1 )
			{
				aux1 = 0;
			}
		}	
        //---------------------------------------------------
	
        vTaskDelay( 300/portTICK_PERIOD_MS );
    }
}

void app_main()
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
	
    nvs_handle my_handle;
	esp_err_t err = nvs_open( "storage", NVS_READWRITE, &my_handle );
	if ( err != ESP_OK ) {					
			if( DEBUG )
				printf( "Error (%s) opening NVS handle!\n", esp_err_to_name(err) );			
	} else {
	    //RELE_0					
		err = nvs_get_i32(my_handle, "RL0", &pin);
		if( err != ESP_OK ) {							
			if( err == ESP_ERR_NVS_NOT_FOUND ) {
				if( DEBUG )
					printf( "\nKey RL0 not found\n" );
				gpio_set_level(RELE_0, 0);
			} 							
		} else {
			if( DEBUG )
				printf( "\nGPIO RL0 is %d\n", pin );
			gpio_set_level(RELE_0, pin);
			status_rel0 = pin;
		}
		//RELE_1
		err = nvs_get_i32(my_handle, "RL1", &pin);
		if( err != ESP_OK ) {							
			if( err == ESP_ERR_NVS_NOT_FOUND ) {
				if( DEBUG )
					printf( "\nKey RL1 not found\n");
				gpio_set_level(RELE_1, 0);
			} 							
		} else {
			if( DEBUG )
				printf( "\nGPIO RL1 is %d\n", pin );
			gpio_set_level(RELE_1, pin);
			status_rel1 = pin;
		}
		nvs_close(my_handle);		
	}
	
    xTaskCreate( Task_Button, "Task_Button", 4098, NULL, 1, NULL );

}
