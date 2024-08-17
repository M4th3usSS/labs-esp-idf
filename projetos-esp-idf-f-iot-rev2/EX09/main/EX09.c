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
#include <string.h>
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
 * Protótipos;
 */
void app_main( void );
void task_ap( void *pvParameter );
static void show_station_list( void );
static void start_dhcp_server( void );
void wifi_init_ap( void );
static esp_err_t event_handler( void *ctx, system_event_t *event );

/**
 * Definições;
 */
#define DEBUG           1

/*
	São Permitidos no máximo 4 clientes conectados ao AP.
	Este parâmetro pode ser alterado no pelo "menuconfig".
	Ver arquivo: Kconfig.projbuild
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/**
 * 0-> SSID não oculto na rede.	
 * 1-> SSID oculto na rede.
 */
#define AP_SSID_HIDDEN 			   0
										
/**
 * Variáveis;
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT 	= BIT0;
const int WIFI_DISCONNECTED_BIT = BIT1;
static const char *TAG = "main: ";

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador.
  ref: http://esp32.info/docs/esp_idf/html/dc/df5/esp__wifi_8h_source.html
*/
static esp_err_t event_handler( void *ctx, system_event_t *event )
{
    switch(event->event_id) {
		case SYSTEM_EVENT_AP_START:
			if( DEBUG )
				ESP_LOGI(TAG, "ESP32 Inicializado em modo AP.\n");
			break;
			
		case SYSTEM_EVENT_AP_STACONNECTED:
		
		/* É possível saber o MAC address do cliente conectado pelo código abaixo. */
			if( DEBUG )
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

/**
 * Configura a rede WiFi do ESP32 em Modo Acess Point;
 */
void wifi_init_ap( void )
{
    /*
		Registra a função de callback dos eventos do WiFi.
	*/
    ESP_ERROR_CHECK( esp_event_loop_init( event_handler, NULL ) );
	
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init( &cfg ));
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
    if ( strlen( EXAMPLE_ESP_WIFI_PASS ) == 0 ) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    if( DEBUG )
    	ESP_LOGI(TAG, "wifi_init_ap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

/**
 * Configura o rede Acess Point do ESP32;
 */
static void start_dhcp_server( void )
{  
	tcpip_adapter_init();

	/**
	 * Desliga o Serviço DHCP (Server) do ESP32; 
	 * 
	 */
	ESP_ERROR_CHECK( tcpip_adapter_dhcps_stop( TCPIP_ADAPTER_IF_AP ) );

	/**
	 * Configura o Endereço IP, Gateway e Máscara da Rede WiFi do ESP32; 
	 */
	tcpip_adapter_ip_info_t info;
	memset( &info, 0, sizeof(info) );
	IP4_ADDR( &info.ip, 192, 168, 1, 1 );
	IP4_ADDR( &info.gw, 192, 168, 1, 1 );
	IP4_ADDR( &info.netmask, 255, 255, 255, 0 );
	ESP_ERROR_CHECK( tcpip_adapter_set_ip_info( TCPIP_ADAPTER_IF_AP, &info ) );
	
	/* 
		Start DHCP server. Portanto, o ESP32 fornecerá IPs aos clientes STA na 
		faixa de IP configurada acima.
	*/
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	
	if( DEBUG )
		ESP_LOGI( TAG, "Servico DHCP reinicializado.\r\n");
}

/**
 * Imprime no Console os dados dos Clientes conectados a rede WiFi do ESP32;
 */
static void show_station_list( void ) 
{
	if( DEBUG )
	{
		ESP_LOGI( TAG, "Lista de Clientes Conectados:\n");
		ESP_LOGI( TAG, "--------------------------------------------------\n");		
	}

	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset( &wifi_sta_list, 0, sizeof( wifi_sta_list ) );
	memset( &adapter_sta_list, 0, sizeof( adapter_sta_list ) );
   
	ESP_ERROR_CHECK( esp_wifi_ap_get_sta_list( &wifi_sta_list ) );	
	ESP_ERROR_CHECK( tcpip_adapter_get_sta_list( &wifi_sta_list, &adapter_sta_list ) );
	tcpip_adapter_sta_info_t station;
	
	for( int i = 0; i < adapter_sta_list.num; i++ ) 
	{
		
		station = adapter_sta_list.sta[i];
		
		/**
		 * Imprime na saída do console o MAC Address e Endereço IP do Cliente
		 * conectado ao ESP32;
		 */
		if( DEBUG )
        	ESP_LOGI( TAG, "%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n", i + 1,
				station.mac[0], station.mac[1], station.mac[2],
				station.mac[3], station.mac[4], station.mac[5],
				ip4addr_ntoa(&station.ip));   
	}
	
	if( DEBUG )
		ESP_LOGI( TAG, "\n");
}

/**
 * Task Responsável em Imprimir as informações do cliente 
 * que permanece conectado a rede do ESP32;
 */
void task_ap( void *pvParameter )
{
    EventBits_t staBits;

    if( DEBUG )
      ESP_LOGI( TAG, "task_ap run...\r\n" );
	
    for(;;) 
	{	
		/*
			Qualquer um dos bits WIFI_CONNECTED_BIT e WIFI_DISCONNECTED_BIT que forem setados, fará com que
			esta Task entre novamente em operação. 
		*/
		staBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		
		//Verifica qual foi o bit do EventGroup que está setado. 
		if( ( staBits & WIFI_CONNECTED_BIT ) != 0 ) { 
			 ESP_LOGI( TAG, "Novo Cliente Conectado.\r\n");
		} else {
			ESP_LOGI( TAG, "Cliente foi Desconectado.\r\n");
		}
		/*
			Este delay é necessário para que o endereço IP gerado pelo serviço do DHCP seja
			propagado e alocado em memória. Caso você tirar o delay, verá que o valor impresso no console
			inicialmente será IP: 0.0.0.0. Isso se deve, pois, o DHCP ainda não armazenou em memória 
			a publicação do IP.
		*/
		vTaskDelay( 3000/portTICK_PERIOD_MS );

		/**
		 * Função responsável por imprimir os dados da estão (cliente)
		 * conectada ao ESP32 configurado em modo WiFi Acess Point;
		 */
        show_station_list();
	}
}

/**
 * Função de entrada da Aplicação; 
 * Chamada assim que as rotinas do bootloader for finalizada;
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
	   Task responsável em informar quando um novo client estiver conectado
	   ao ESP32 (inicialmente configurado como um servidor em modo Acess Point);
	*/
	if( xTaskCreate( task_ap ,"task_ap", 8096, NULL, 5, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_ap.\n" );	
		return;		
	}
}
