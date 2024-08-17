/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
#include <stdio.h>
#include <string.h>
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

void Show_StationList ( ) ;
static void start_dhcp_server ( );
void wifi_init_ap ( );
void Task_STA ( void *pvParam );
/*
 Informe o SSID e Password de sua rede WiFi local.
*/

#define EXAMPLE_ESP_WIFI_SSID 	"ESP32"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"
//#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD

/*
	É permitido no máximo 4 clientes conectados ao AP.
	Este parâmetro pode ser alterado no pelo "menuconfig".
	Ver arquivo: Kconfig.projbuild
*/
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN
#define AP_SSID_HIDDEN 			   0//0-> SSID não oculto na rede.	
									//1-> SSID oculto na rede.
										

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT 	= BIT0;
const int WIFI_DISCONNECTED_BIT = BIT1;

static const char *TAG = "WIFI: ";

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador.
  ref: http://esp32.info/docs/esp_idf/html/dc/df5/esp__wifi_8h_source.html
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
		case SYSTEM_EVENT_AP_START:
			printf("ESP32 Inicializado em modo AP.\n");
			break;
			
		case SYSTEM_EVENT_AP_STACONNECTED:
		
		/* É possível saber o MAC address do cliente conectado pelo código abaixo. */
			ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
				 
			xEventGroupSetBits( wifi_event_group, WIFI_CONNECTED_BIT );			
			break;
			
		case SYSTEM_EVENT_AP_STADISCONNECTED:	
			/*Informa que um cliente foi desconectado do AP */
			xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECTED_BIT);
			break;
			
		default:
			break;
    }
    return ESP_OK;
}

void wifi_init_ap()
{
    /*
		Registra a função de callback dos eventos do WiFi.
	*/
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
			/*
				Caso queira tornar o SSID da rede AP oculta, ou seja, nenhum cliente conseguirá escanear a rede, 
				defina AP_SSID_HIDDEN com o valor 1; Caso o cliente STA conheça o SSID e o PASSWORD do AP, ele conseguirá acessar a rede mesmo com o SSID oculto.
			*/
			.ssid_hidden = AP_SSID_HIDDEN,
			
			/* O password deve ter 8 ou mais caracteres */
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
	/* Caso o tamanho do password seja Zero, então programa a rede sem password. */
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

static void start_dhcp_server()
{  
	tcpip_adapter_init();
	/* Stop DHCP server */
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	
	/* 
		Start DHCP server. Portanto, o ESP32 fornecerá IPs aos clientes STA na 
		faixa de IP configurada acima.
	*/
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	printf("Servico DHCP inicializado. \n");
}


void Show_StationList() 
{
	printf(" Clientes STA Conectados:\n");
	printf("--------------------------------------------------\n");
	
	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
   
	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));	
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));
	tcpip_adapter_sta_info_t station;
	
	for(int i = 0; i < adapter_sta_list.num; i++) {
		
		station = adapter_sta_list.sta[i];
		
         printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n", i + 1,
				station.mac[0], station.mac[1], station.mac[2],
				station.mac[3], station.mac[4], station.mac[5],
				ip4addr_ntoa(&station.ip));   
	}
	
	printf("\n");
}

void Task_STA( void *pvParam )
{
    EventBits_t staBits;
    printf("print_sta_info task started \n");
	
    for(;;) 
	{	
		/*
			Qualquer um dos bits WIFI_CONNECTED_BIT e WIFI_DISCONNECTED_BIT que forem setados, fará com que
			esta Task entre novamente em operação. 
		*/
		staBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		
		//Verifica qual foi o bit do EventGroup que está setado. 
		if( (staBits & WIFI_CONNECTED_BIT ) != 0 ) { 
			printf("New station connected\n\n");
		} else {
			printf("A station disconnected\n\n");
		}
		/*
			Este delay é necessário para que o endereço IP gerado pelo serviço do DHCP seja
			propagado e alocado em memória. Caso você tirar o delay, verá que o valor impresso no console
			inicialmente será IP: 0.0.0.0. Isso se deve, pois, o DHCP ainda não armazenou em memória 
			a publicação do IP.
		*/
		vTaskDelay(3000/portTICK_PERIOD_MS );
        Show_StationList();
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
	  Configura e Inicializa o servidor DHCP .
	*/
	start_dhcp_server();
	
	/*
	  Configura o modo AP do ESP32.
	*/
    wifi_init_ap();
	
	/*
	   Task responsável em imprimir o valor do IP das STA's aceito pelo servidor AP. 
	*/
	xTaskCreate(Task_STA,"Task_STA",8096,NULL,5,NULL);
}
