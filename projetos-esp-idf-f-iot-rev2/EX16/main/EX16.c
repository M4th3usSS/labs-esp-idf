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
 * Definições Gerais
 */
#define DEBUG 1
#define CONFIG_WIFI_SSID "FSimplicio"
#define CONFIG_WIFI_PASSWORD "fsimpliciokzz5"

/**
 * Variáveis
 */
static const char *TAG = "mqtt";
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/**
 * Ao definir os dois comandos a seguir o arquivo mqtt_geniot_io_pem será carregado
 * como uma constante na memória flash do ESP32; 
 * O arquivo mqtt_geniot_io_pem contém o certificado TLS de acesso a plataforma geniot.io;
 */
extern const uint8_t mqtt_geniot_io_pem_start[] asm("_binary_mqtt_geniot_io_pem_start");
extern const uint8_t mqtt_geniot_io_pem_end[]   asm("_binary_mqtt_geniot_io_pem_end");

/**
 * Protótipos
 */
static esp_err_t wifi_event_handler( void *ctx, system_event_t *event );
static void wifi_init_sta( void );
static esp_err_t mqtt_event_handler( esp_mqtt_event_handle_t event );
static void mqtt_app_start( void );
void app_main( void );

/**
 * Função de callback chamada pelo stack WiFi;
 */
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) 
    {
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

/**
 * Configura o WiFi do ESP32 em modo Station (client);
 */
static void wifi_init_sta( void )
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ));
    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ));
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ));
    ESP_ERROR_CHECK( esp_wifi_start() );

    if( DEBUG )
        ESP_LOGI( TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******" );
}


static esp_err_t mqtt_event_handler( esp_mqtt_event_handle_t event )
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) 
    {
        
        /*
         * Este case foi adicionado a partir da versão 3.2 do SDK-IDF;
         * e é necessário ser adicionado devido as configurações de error do compilador;
         */
          case MQTT_EVENT_BEFORE_CONNECT: 

            if( DEBUG )
                ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            
          break;
          
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            /**
             * Assim que o ESP32 estabelecer conexão com o broker MQTT
             * é feita a assinatura do tópico "temperatura" com o QoS 0;
             */
            msg_id = esp_mqtt_client_subscribe(client, "temperatura", 0); 
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

            /**
             * Logo após a assinatura é publicado o valor 34.56 no tópico 
             * temperatura com QoS 0; 
             * Onde:
             * id = esp_mqtt_client_publish(Handle_client, topico, string_valor, string_size, QoS, retenção);
             */
            msg_id = esp_mqtt_client_publish(client, "temperatura", "34.56", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            /**
             * Todas as mensagens recebidas chegam aqui! 
             * Portanto, é necessário verificar qual o tópico é dono da mensagem.
             */
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }

    return ESP_OK;
}


/**
 * Configura o stack MQTT;
 */
static void mqtt_app_start(void)
{

    const esp_mqtt_client_config_t mqtt_cfg = 
    {
		.uri = "mqtt://mqtt.geniot.io:1883",
        .event_handle = mqtt_event_handler,
		.username = "7f33f3ce94f774494feb8f1843511509",
		.password = "7f33f3ce94f774494feb8f1843511509",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
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
    nvs_flash_erase();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    wifi_event_group = xEventGroupCreate();

    /**
     * Inicializa o Wifi do ESP32 em modo Station;
     */
    wifi_init_sta();

    /**
     * Aguarda a conexão via WiFi do ESP32 ao roteador;
     */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    /**
     * Inicializa o stack MQTT; 
     */
    mqtt_app_start();

}
