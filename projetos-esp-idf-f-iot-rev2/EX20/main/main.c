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
 * LWIP
 */
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

/**
 * Lib MQTT
 */
#include "mqtt_client.h"
 /**
  * Lib Wifimanager
  */
#include "http_server.h"
#include "wifi_manager.h"

/**
 * Definições Gerais
 */
#define DEBUG 1
#define BUTTON	GPIO_NUM_17 
#define TOPICO_TEMPERATURA "temperatura"

/**
 * Variáveis
 */
static esp_mqtt_client_handle_t client;
static const char *TAG = "main: ";
static QueueHandle_t Queue_Button;
extern const int WIFI_MANAGER_REQUEST_STA_CONNECT_BIT;
extern EventGroupHandle_t wifi_manager_event_group;
static EventGroupHandle_t mqtt_event_group;
const static int CONNECTED_BIT = BIT0;

static TaskHandle_t task_http_server = NULL;
static TaskHandle_t task_wifi_manager = NULL;

/**
 * Protótipos
 */
static void monitoring_task( void *pvParameter );
static esp_err_t mqtt_event_handler( esp_mqtt_event_handle_t event );
static void mqtt_app_start( void );
static void task_button( void *pvParameter );
void app_main( void );

/**
 * A cada 5s imprime a quantidade de bytes livre no Heap;
 */
static void monitoring_task( void *pvParameter )
{
	for(;;)
	{
		if( DEBUG )
			ESP_LOGI( TAG, "free heap: %d\n",esp_get_free_heap_size() );

		vTaskDelay( 5000 / portTICK_PERIOD_MS );
	}
}

/**
 * Função de callback do stack MQTT; 
 * Por meio deste callback é possível receber as notificações com os status
 * da conexão e dos tópicos assinados e publicados;
 */
static esp_err_t mqtt_event_handler( esp_mqtt_event_handle_t event )
{
    int msg_id;

    /**
     * Repare que a variável client, no qual armazena o Handle da conexão socket
     * do MQTT foi colocada como global; 
     */
    client = event->client;
    
    switch (event->event_id) 
    {

        case MQTT_EVENT_BEFORE_CONNECT: 
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;        
        /**
         * Evento chamado quando o ESP32 se conecta ao broker MQTT, ou seja, 
         * caso a conexão socket TCP, SSL/TSL e autenticação com o broker MQTT
         * tenha ocorrido com sucesso, este evento será chamado informando que 
         * o ESP32 está conectado ao broker;
         */
        case MQTT_EVENT_CONNECTED:

            if( DEBUG )
                ESP_LOGI( TAG, "MQTT_EVENT_CONNECTED" );

            /**
             * Assina o tópico umidade assim que o ESP32 se conectar ao broker MQTT;
             * onde, 
             * esp_mqtt_client_subscribe( Handle_client, TOPICO_STRING, QoS );
             */
            msg_id = esp_mqtt_client_subscribe( client, "umidade", 0 );

            /**
             * Se chegou aqui é porque o ESP32 está conectado ao Broker MQTT; 
             * A notificação é feita setando em nível 1 o bit CONNECTED_BIT da 
             * variável mqtt_event_group;
             */
            xEventGroupSetBits( mqtt_event_group, CONNECTED_BIT );
            break;
        /**
         * Evento chamado quando o ESP32 for desconectado do broker MQTT;
         */
        case MQTT_EVENT_DISCONNECTED:

            if( DEBUG )
                ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");   
            /**
             * Se chegou aqui é porque o ESP32 foi desconectado do broker MQTT;
             */
            xEventGroupClearBits(mqtt_event_group, CONNECTED_BIT);
            break;

        /**
         * O eventos seguintes são utilizados para notificar quando um tópico é
         * assinado pelo ESP32;
         */
        case MQTT_EVENT_SUBSCRIBED:
            break;
        
        /**
         * Quando a assinatura de um tópico é cancelada pelo ESP32, 
         * este evento será chamado;
         */
        case MQTT_EVENT_UNSUBSCRIBED:
            break;
        
        /**
         * Este evento será chamado quando um tópico for publicado pelo ESP32;
         */
        case MQTT_EVENT_PUBLISHED:
            
            if( DEBUG )
                ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        
        /**
         * Este evento será chamado quando uma mensagem chegar em algum tópico 
         * assinado pelo ESP32;
         */
        case MQTT_EVENT_DATA:

            if( DEBUG )
            {
                ESP_LOGI(TAG, "MQTT_EVENT_DATA"); 

                /**
                 * Sabendo o nome do tópico que publicou a mensagem é possível
                 * saber a quem data pertence;
                 */
                ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
                ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);               
            }       
            break;
        
        /**
         * Evento chamado quando ocorrer algum erro na troca de informação
         * entre o ESP32 e o Broker MQTT;
         */
        case MQTT_EVENT_ERROR:
            if( DEBUG )
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

/**
 * Configuração do stack MQTT; 
 */
static void mqtt_app_start( void )
{

    /**
     * Sem SSL/TLS
     */
    const esp_mqtt_client_config_t mqtt_cfg = 
    {
		.uri = "mqtt://mqtt.geniot.io:1883",
        .event_handle = mqtt_event_handler,
		.username = "7f33f3ce94f774494feb8f1843511509",
		.password = "7f33f3ce94f774494feb8f1843511509",
    };
   
    /**
     * Carrega configuração do descritor e inicializa stack MQTT;
     */
    esp_mqtt_client_handle_t client = esp_mqtt_client_init( &mqtt_cfg );
    esp_mqtt_client_start(client);
}

/**
 * Task responsável pela manipulação
 */
static void task_button( void *pvParameter )
{
    char str[20]; 
    int aux = 0;
	int counter = 0;
		
    /**
     * Configuração da GPIO BUTTON;
     */
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 

    for(;;) 
	{

		/**
		 * Enquanto o ESP32 estiver conectado a rede WiFi será realizada a leitura
		 * de button;
		 */
	    xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
		/**
		 * Botaõ pressionado?
		 */
		if( gpio_get_level( BUTTON ) == 0 && aux == 0 )
		{ 
		  	/**
		  	 * Aguarda 80 ms devido o bounce;
		  	 */
			vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
			/**
			 * Verifica se realmente button está pressionado;
			 */
			if( gpio_get_level( BUTTON ) == 0 && aux == 0 ) 
			{	


				if( DEBUG )
                	ESP_LOGI( TAG, "Button %d Pressionado .\r\n", BUTTON ); 

				sprintf( str, "{\"value\":%d}", counter );
				
				/**
				 * Publica em TOPICO_TEMPERATURA o valor de counter com QoS 0 sem retenção;
				 */
				esp_mqtt_client_publish( client, TOPICO_TEMPERATURA, str, strlen( str ), 0, 0 );	
				counter++;

				aux = 1; 
			}
		}

		/**
		 * Botão solto?
		 */
		if( gpio_get_level( BUTTON ) == 1 && aux == 1 )
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			
			if(gpio_get_level( BUTTON ) == 1 && aux == 1 )
			{
				aux = 0;
			}
		}	

		/**
		 * Contribui para as demais tasks de menor prioridade sejam escalonadas
		 * pelo scheduler;
		 */
		vTaskDelay( 100/portTICK_PERIOD_MS );	
    }
}


/**
 * Inicio da Aplicação;
 */
void app_main( void )
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
	if( xTaskCreate( http_server, "http_server", 2048, NULL, 5, &task_http_server ) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar http_server.\r\n" );  
      return;   
    }

	/* start the wifi manager task */
	if( xTaskCreate( wifi_manager, "wifi_manager", 4096, NULL, 4, &task_wifi_manager ) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar wifi_manager.\r\n" );  
      return;   
    }

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
	
	xEventGroupWaitBits( wifi_manager_event_group, WIFI_MANAGER_REQUEST_STA_CONNECT_BIT, false, true, portMAX_DELAY );
	
	/**
	 * Inicializa o stack MQTT;
	 */
	mqtt_app_start();
	
	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	if( xTaskCreate( task_button, "task_button", 4098, NULL, 2, NULL )  != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_button.\r\n" );  
      return;   
    }
	
	/**
	 * Task responsável pelo monitoramento do heap;
	 */
	if( xTaskCreatePinnedToCore( monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1)  != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar monitoring_task.\r\n" );  
      return;   
    }

}
