/*
Copyright (c) 2017 Tony Pottier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file main.c
@author Tony Pottier
@brief Entry point for the ESP32 application.
@see https://idyl.io
@see https://github.com/tonyp7/esp32-wifi-manager


  Modificado por: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/


*/



#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "http_server.h"
#include "wifi_manager.h"
#include "mqtt_client.h"


#define BUTTON	GPIO_NUM_17 
#define TOPICO_TEMPERATURA "temperatura"

static const char *TAG = "MQTTS";
QueueHandle_t Queue_Button;

extern const int WIFI_MANAGER_REQUEST_STA_CONNECT_BIT;
extern EventGroupHandle_t wifi_manager_event_group;

static EventGroupHandle_t mqtt_event_group;

const static int CONNECTED_BIT = BIT0;

static TaskHandle_t task_http_server = NULL;
static TaskHandle_t task_wifi_manager = NULL;

/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code!
 */
void monitoring_task(void *pvParameter)
{
	for(;;){
		printf("free heap: %d\n",esp_get_free_heap_size());
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    int msg_id;
    client = event->client;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			msg_id = esp_mqtt_client_subscribe(client, "button/io", 0);
			xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");	
			xEventGroupClearBits(mqtt_event_group, CONNECTED_BIT);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            break;
			
        case MQTT_EVENT_UNSUBSCRIBED:
            break;
			
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
			
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);	

            break;
			
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
   
    //Conexão sem TLS/SSL
    const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = "mqtt://mqtt.cientistadofuturo.com.br:1883",
        .event_handle = mqtt_event_handler,
		.username = "user1",
		.password = "321",
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void Task_Button ( void *pvParameter )
{
    char res[10]; 
    volatile int aux=0;
	uint32_t counter = 0;
		
    /* Configura a GPIO 17 como saída */ 
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 
	
    printf("Leitura da GPIO BUTTON\n");
    
	
	
    for(;;) 
	{
	
		if(gpio_get_level( BUTTON ) == 0 && aux == 0)
		{ 
		  /* Delay para tratamento do bounce da tecla*/
		  vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
		  if(gpio_get_level( BUTTON ) == 0) 
		  {		
			
            xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
			
			
			sprintf(res, "%d", counter);
			/*
			  link (dica de leitura)
			  https://www.hivemq.com/blog/mqtt-essentials-part-8-retained-messages
			  Sintaxe:
			  int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, 
										  const char *topic, 
										  const char *data, 
										  int len, 
										  int qos, 
										  int retain)
			*/
			esp_mqtt_client_publish(client, TOPICO_TEMPERATURA, res, strlen(res), 0, 0);	
			counter++;
			
			aux = 1; 
		  }
		}

		if(gpio_get_level( BUTTON ) == 1 && aux == 1)
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			if(gpio_get_level( BUTTON ) == 1 )
			{
				aux = 0;
			}
		}	

		vTaskDelay( 10/portTICK_PERIOD_MS );	
    }
}

void app_main()
{


	/* disable the default wifi logging */
	esp_log_level_set("wifi", ESP_LOG_NONE);

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
	

	/* start the HTTP Server task */
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &task_http_server);

	/* start the wifi manager task */
	xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 4, &task_wifi_manager);

    /*
	  O ESP32 está conectado ao broker MQTT? Para sabermos disso, precisamos dos event_group do FreeRTOS.
	*/
	mqtt_event_group = xEventGroupCreate();
	
	/*
	  Antes de conectar ao broker mqtt é necessário verificar se ocorreu a conexão com a rede wifi. 
	  A sinalização da rede Wifi, ou seja, a conexão e a confirmação de "subida" do ESP32
	  como estação é feita na folha "wifi_manager.c" na linha 465 pelo event_group:
	  xEventGroupSetBits(wifi_manager_event_group, WIFI_MANAGER_REQUEST_STA_CONNECT_BIT);
	  
	  Portanto, precisamos referenciar as variáveis wifi_manager_event_group e WIFI_MANAGER_REQUEST_STA_CONNECT_BIT
	  nesta folha main.c. 
	  
	  Veja que no início do programa foram referenciadas as variáveis. 
	  
	  extern const int WIFI_MANAGER_REQUEST_STA_CONNECT_BIT;
	  extern EventGroupHandle_t wifi_manager_event_group;
	  
	
	*/
	
	xEventGroupWaitBits(wifi_manager_event_group, WIFI_MANAGER_REQUEST_STA_CONNECT_BIT, false, true, portMAX_DELAY);
	mqtt_app_start();
	
	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	xTaskCreate( Task_Button, "TaskButton", 4098, NULL, 5, NULL );
	
	
	/* your code should go here. In debug mode we create a simple task on core 2 that monitors free heap memory */
#if WIFI_MANAGER_DEBUG
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
#endif


}
