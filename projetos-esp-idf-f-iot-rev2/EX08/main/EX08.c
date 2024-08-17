/* --------------------------------------------------------------------------
  Autor: Prof° Fernando Simplicio;
  Hardware: NodeMCU ESP32
  Espressif SDK-IDF: v3.2
  Curso: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
 *  --------------------------------------------------------------------------

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/**
 * Lib C
 */
#include <stdio.h>
/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
/**
 * WiFi
 */
#include "esp_wifi.h"
/**
 * Callback do WiFi
 */
#include "esp_event_loop.h"
/**
 * Logs
 */
#include "esp_system.h"
#include "esp_log.h"
/**
 * NVS
 */
#include "nvs_flash.h"
/**
 * Lwip
 */
#include "lwip/err.h"
#include "lwip/sys.h"

/**
 * Definições;
 */
#define TRUE          	1 
#define DEBUG           1

#define WIFI_SSID       "FSimplicio"
#define WIFI_PASSWORD   "fsimpliciokzz5"

/**
 * Protótipos
 */
static esp_err_t event_handler( void *ctx, system_event_t *event );
void task_ip( void *pvParameter );
void wifi_init_sta( void );
void app_main( void );

/**
 * Variáveis
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "main: ";

/**
 * Função de callback chamada quando é alterado o estado da
 * conexão WiFi;
 */
static esp_err_t event_handler( void *ctx, system_event_t *event )
{
    switch( event->event_id ) 
    {
		case SYSTEM_EVENT_STA_START:

		    if( DEBUG )
		    	ESP_LOGI(TAG, "Vou tentar Conectar ao WiFi!\r\n");
			/*
				O WiFi do ESP32 foi configurado com sucesso. 
				Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
			*/
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:

		    if( DEBUG )
		    	ESP_LOGI(TAG, "Opa, estou Online!\r\n");
		    /*
				Se chegou aqui é porque a rede WiFi foi aceita pelo roteador e um IP foi 
				recebido e gravado no ESP32.
			*/
			if( DEBUG )
				ESP_LOGI(TAG, "got ip:%s",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
			/*
			   O bit WiFi_CONNECTED_BIT é setado pelo EventGroup pois precisamos avisar as demais Tasks
			   que a rede WiFi foi devidamente configurada. 
			*/
			xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:

		    if( DEBUG )
		    	ESP_LOGI(TAG, "Ops, rede offline! Vamos tentar reconectar...\r\n");
		    /*
				Se chegou aqui foi devido a falha de conexão com a rede WiFi.
				Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP32.
			*/
			esp_wifi_connect();
			
			/*
				É necessário apagar o bit WIFI_CONNECTED_BIT para avisar as demais Tasks que a conexão WiFi
				está offline no momento. 
			*/
			xEventGroupClearBits( wifi_event_group, WIFI_CONNECTED_BIT );
			break;
		default:
			break;
    }
    return ESP_OK;
}

/**
 * Configura o WiFi do ESP32 em modo Station, ou seja, 
 * passa a operar como um cliente, porém com operando com o IP fixo;
 */
void wifi_init_sta( void )
{
    tcpip_adapter_init();
	
	//--------------------------------------------------------------
	/*
		Desliga o serviço dhcp do stack tcp e configura o ESP32 com IP fixo.
	*/
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	tcpip_adapter_ip_info_t ipInfo;

	/**
	 * Configura IP, Gateway e Máscara da Rede; 
	 */
	IP4_ADDR(&ipInfo.ip, 10,0,0,45);
	IP4_ADDR(&ipInfo.gw, 10,0,0,1);
	IP4_ADDR(&ipInfo.netmask, 255,0,0,0);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    //--------------------------------------------------------------
	
    ESP_ERROR_CHECK( esp_event_loop_init( event_handler, NULL ) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    if( DEBUG )
    {
	    ESP_LOGI( TAG, "wifi_init_sta finished." );
	    ESP_LOGI( TAG, "connect to ap SSID:%s password:%s",WIFI_SSID, WIFI_PASSWORD);    	
    }

}

void task_ip( void *pvParameter )
{
    if( DEBUG )
      ESP_LOGI( TAG, "task_ip run...\r\n" );
	
    for(;;) {
        
		/*
		   Enquanto a rede WiFi não for configurada, esta Task permanecerá bloqueada pelo EventGroup.
		*/
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);	
        
		/*
			Se chegou aqui é porque a rede WiFi foi configurada e já podemos enviar dados via TCP.
			O código a seguir imprime o valor do IP do ESP32.
		*/
		tcpip_adapter_ip_info_t ip_info;
	    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));  

    	if( DEBUG )
      		ESP_LOGI( TAG, "IP :  %s\n", ip4addr_ntoa(&ip_info.ip) );
		
      	/**
      	 * A cada intervalo de 5s será impresso na saída do console o 
      	 * endereço IP recebido pelo ESP32;
      	 */
		vTaskDelay( 5000/portTICK_PERIOD_MS );
    }
}

/**
 * Função de entrada da aplicação;
 */
void app_main( void )
{	
    /*
		Inicialização da NVS necessária para resgatar os valores de calibração do PHY; 
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
	   somente após o aceite de conexão e a liberação do IP pelo roteador da rede (nos casos de IPs dinâmicos).
	*/
	wifi_event_group = xEventGroupCreate();
	
	/*
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	/*
	   Task responsável em imprimir o valor do IP recebido pelo roteador da rede. 
	*/
    if(xTaskCreate( task_ip, "task_ip", 2048, NULL, 5, NULL )!= pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar Task_IP.\n" );	
		return;		
	}
}
