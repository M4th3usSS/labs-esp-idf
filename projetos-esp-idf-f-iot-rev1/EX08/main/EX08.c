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
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

void Task_IP ( void *pvParam );
/*
 Informe o SSID e Password de sua rede WiFi local.
*/

//#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD

#define EXAMPLE_ESP_WIFI_SSID 	"FSimplicio"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "WIFI: ";

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador (no caso de Ip dinâmico).
  ref: http://esp32.info/docs/esp_idf/html/dc/df5/esp__wifi_8h_source.html
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
		case SYSTEM_EVENT_STA_START:
			/*
				O WiFi do ESP32 foi configurado com sucesso. 
				Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
			*/
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
		    /*
				Se chegou aqui é porque a rede WiFi foi aceita pelo roteador e um IP foi 
				recebido e gravado no ESP32.
			*/
			ESP_LOGI(TAG, "got ip:%s",
					 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
			/*
			   O bit WiFi_CONNECTED_BIT é setado pelo EventGroup pois precisamos avisar as demais Tasks
			   que a rede WiFi foi devidamente configurada. 
			*/
			xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
		    /*
				Se chegou aqui foi devido a falha de conexão com a rede WiFi.
				Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP32.
			*/
			esp_wifi_connect();
			
			/*
				É necessário apagar o bit WIFI_CONNECTED_BIT para avisar as demais Tasks que a conexão WiFi
				está offline no momento. 
			*/
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
			break;
		default:
			break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    tcpip_adapter_init();
	
	//--------------------------------------------------------------
	/*
		Desabilita o serviço dhcp do stack tcp e configura o ESP32 com IP fixo.
	*/
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	tcpip_adapter_ip_info_t ipInfo;
	IP4_ADDR(&ipInfo.ip, 10,1,1,45);
	IP4_ADDR(&ipInfo.gw, 10,1,1,1);
	IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    //--------------------------------------------------------------
	
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void Task_IP ( void *pvParam )
{
    printf("Task IP started \n");
	
    for(;;) {
        
		/*
		   Enquanto a rede WiFi não for configurada, esta Task permanecerá bloqueada pelo EventGroup.
		*/
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, true, true, portMAX_DELAY);	
        
		/*
			Se chegou aqui é porque a rede WiFi foi configurada e já podemos enviar dados via TCP.
			O código a seguir imprime o valor do IP do ESP32.
		*/
		tcpip_adapter_ip_info_t ip_info;
	    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));  
		printf("IP :  %s\n", ip4addr_ntoa(&ip_info.ip));
		
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
    xTaskCreate( Task_IP,"Task_IP", 2048, NULL, 5, NULL );
}
