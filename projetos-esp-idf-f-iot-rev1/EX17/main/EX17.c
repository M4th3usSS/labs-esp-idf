/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#define BUTTON	GPIO_NUM_17 
#define TOPICO_TEMPERATURA "temperatura"

static const char *TAG = "MQTTS";

static EventGroupHandle_t wifi_event_group;
static EventGroupHandle_t mqtt_event_group;

const static int CONNECTED_BIT = BIT0;

QueueHandle_t Queue_Button;


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

#define CONFIG_WIFI_SSID "FSimplicio"
#define CONFIG_WIFI_PASSWORD "fsimpliciokzz5"

static void wifi_init(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
}

extern const uint8_t iot_eclipse_org_pem_start[] asm("_binary_iot_eclipse_org_pem_start");
extern const uint8_t iot_eclipse_org_pem_end[]   asm("_binary_iot_eclipse_org_pem_end");

esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    int msg_id;
    client = event->client;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
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
   /* 
    //Conexão sem TLS/SSL
    const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = "mqtt://mqtt.cientistadofuturo.com.br:1883",
        .event_handle = mqtt_event_handler,
		.username = "user1",
		.password = "321",
    };
   */
   //Conexão com TLS/SSL
    const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = "mqtts://mqtt.cientistadofuturo.com.br:8883",
        .event_handle = mqtt_event_handler,
        .cert_pem = (const char *)iot_eclipse_org_pem_start,
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
	
	/*
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação TCP quando a rede WiFi estiver configurada, ou seja, 
	   somente após o aceite de conexão e a liberação do IP pelo roteador da rede.
	*/
	wifi_event_group = xEventGroupCreate();
	
	/*
	  O ESP32 está conectado ao broker MQTT? Para sabermos disso, precisamos dos event_group do FreeRTOS.
	*/
	mqtt_event_group = xEventGroupCreate();
	/*
	  Configura a rede WiFi.
	*/
	wifi_init();
	
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
    mqtt_app_start();

	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	xTaskCreate( Task_Button, "TaskButton", 4098, NULL, 1, NULL );
	
}
