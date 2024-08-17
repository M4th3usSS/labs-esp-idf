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
#include <errno.h>
/**
 * NVS
 */
#include "nvs_flash.h"
/**
 * Lwip
 */
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/sockets.h>

/**
 * Lib HTTPs
 */
#include "request_http.h"

/**
 * Definições Gerais
 */
#define DEBUG 1
#define SERVER_IP 	"www.geniot.io"
#define SERVER_PORT 443
#define EXAMPLE_ESP_WIFI_SSID 	"FSimplicio"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"
#define BUTTON	GPIO_NUM_17 

/**
 * Variáveis
 */
QueueHandle_t Queue_Button;
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "main: ";

/**
 * Protótipos
 */
static esp_err_t event_handler(void *ctx, system_event_t *event);
void wifi_init_sta( void );
void task_http ( void *pvParameter );
void task_button ( void *pvParameter );

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador.
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

/**
 * Função responsável em configurar a rede WiFi em modo Station;
 */
void wifi_init_sta( void )
{
    tcpip_adapter_init();
	
	//IP fixo?.
	/*
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	tcpip_adapter_ip_info_t ipInfo;
	IP4_ADDR(&ipInfo.ip, 10,1,1,45);
	IP4_ADDR(&ipInfo.gw, 10,1,1,1);
	IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	*/
	
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI( TAG, "wifi_init_sta finished.");
    ESP_LOGI( TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS );
}


/**
 * Task responsável pelo envio e leitura das requisições via https 
 * entre cliente (ESP32) e servidor web;
 */
void task_http ( void *pvParameter )
{
	int count; 
	int error;

	if( DEBUG )
		ESP_LOGI(TAG, "task_http run...\r\n");

	/**
	 * Para enviar e armazenar os bytes trocados com o servidor Web 
	 * será utilizado um buffer de HTTP_BUFFER_SIZE bytes criado dinamicamente;
	 */
	char *buffer = pvPortMalloc( HTTP_BUFFER_SIZE );
	if( buffer == NULL ) 
	{
		if( DEBUG )
			ESP_LOGI(TAG, "pvPortMalloc Error\r\n");
		
		vTaskDelete(NULL); 	  
		return;
	 }
	
    for(;;) 
	{	
		
		/**
		 * Aguarda o botão ser pressionado;
		 */
	    xQueueReceive( Queue_Button, &count, portMAX_DELAY ); 
		
		/***********************************************
		 * Prepara o cabeçalho HTTP. 
		 * Neste programa o cabeçalho HTTP é fixo, porém o conteúdo enviado para o servidor web
		 * pode ser alterado;
		 * O valor da variável counter (incrementada cada vez que button for pressionado) será 
		 * enviado para a web. 
		 * Segue exemplo da mensagem a ser enviada via GET:

			"GET /things/services/api/v1/variables/S00/value/?token=7f33f3ce94f774494feb8f1843511509 HTTP/1.1\n"
			"HOST: www.geniot.io\n"
			"content-type: application/json\n"
			"connection: close\n\n";

		 */
		const char * msg_get = \
					"GET /things/services/api/v1/variables/S00/value/?token=7f33f3ce94f774494feb8f1843511509 HTTP/1.1\r\n"
					"HOST: www.geniot.io\r\n"
					"content-type: application/json\r\n"
					"connection: close\r\n\r\n";
					
		sprintf( buffer , "%s", msg_get); 
		
		/**
		 * Configura o descritor informando o endereço IP,PORTA e cabeçalho http
		 * a ser enviado para o servidor web;
		 */
		http_config_t rest = 
		{
			.ip = SERVER_IP, 
			.port = SERVER_PORT,
			.url = buffer
		};
		
		/**
		 * Dispara a requisição para o servidor web e aguarda o retorno da mensagem;
		 * Espera-se que o retorno da mensagem seja um Json formatado conforme o
		 * exemplo a seguir:
	  	 * Segue exemplo da resposta em Json do Servidor;
	  	 * 
			{
			  "id": "54e483ae67877a1d4e9c5ddb77adc0f5",
			  "name": "Temperatura",
			  "icon": null,
			  "unit": "\u00b0C",
			  "datasource": "bede5f484f7361416ce116bef92d63a7",
			  "url": "https:\/\/www.geniot.io\/things\/services\/api\/v1\/variables\/54e483ae67877a1d4e9c5ddb77adc0f5",
			  "description": "",
			  "properties": null,
			  "tags": null,
			  "value_url": "https:\/\/www.geniot.io\/things\/services\/api\/v1\/variables\/54e483ae67877a1d4e9c5ddb77adc0f5\/values",
			  "created_at": "2019-05-03 22:36:43",
			  "last_value": "{\"timestamp\":1556923011,\"value\":123}",
			  "last_activity": "2019-05-03 19:36:51",
			  "type": "0",
			  "derived_expr": null,
			  "label": "S00"
			}
	  	 */
		 
		if((error = http_send( &rest, buffer, HTTP_BUFFER_SIZE )) == 0) 
		{
			if( DEBUG )
				ESP_LOGI( TAG, "Retorno:\r\n %s\r\n", buffer );
		} else {
			
			/**
			 * Xiii, houve algum erro durante a requisição!!
			 */
			if( DEBUG )
				ESP_LOGI( TAG, "error request: %d\r\n", error );			
		}
	
		/**
		 * Delay recomendado para que as outras tasks de menor prioridade tenham 
		 * condições de serem escalonadas pelo scheduler;
		 */
		vTaskDelay( 10/portTICK_PERIOD_MS );	
    }

	/**
	* A variável buffer precisa ser desalocada da memória, visto que foi
	* utilizado malloc;
	*/
	vPortFree( buffer );

	vTaskDelete( NULL );
}

/**
 * Task responsável pelos acionamentos do botão;
 */
void task_button( void *pvParameter )
{
	int count = 0;
    int aux=0;

	if( DEBUG )
		ESP_LOGI(TAG, "task_button run...\r\n");

    /**
     * GPIO (Button) configurada como entrada;
     */
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 
    
    for(;;) 
	{
	
		/**
		 * Botão pressionado?
		 */
		if( gpio_get_level( BUTTON ) == 0 && aux == 0 )
		{ 
			/**
			* Aguarda 80ms devido ao bounce;
			*/
			vTaskDelay( 80/portTICK_PERIOD_MS );	

			if( gpio_get_level( BUTTON ) == 0 && aux == 0 ) 
			{
				if( xQueueSend( Queue_Button, (void *)&count, (TickType_t )0 ) )
				{
					count++;
				}	

				aux = 1; 
			}
		}
		/**
		 * Botão solto?
		 */
		if( gpio_get_level( BUTTON ) == 1 && aux == 1 )
		{   
		    /**
			* Aguarda 80ms devido ao bounce;
			*/
		    vTaskDelay( 80/portTICK_PERIOD_MS );	

			if( gpio_get_level( BUTTON ) == 1 && aux == 1 )
			{
				aux = 0;
			}
		}	

		/**
		 * Em algum momento é preciso dar oportunidade para as demais task de menor prioridade
		 * do programa serem executadas. 
		 */
		vTaskDelay( 10/portTICK_PERIOD_MS );	
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
	
	/*
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação TCP quando a rede WiFi estiver configurada, ou seja, 
	   somente após o aceite de conexão e a liberação do IP pelo roteador da rede (nos casos de IPs dinâmicos).
	*/
	wifi_event_group = xEventGroupCreate();
	
	/**
	 * Fila responsável pela troca de informações entre a task_button e task_http;
	 */
	Queue_Button = xQueueCreate( 5, sizeof(uint32_t) );

	/*
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda o ESP32 se conectar a rede WiFi local;
	 */
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);	

	/**
	 * Task responsável pelas rotinas de tratamento de button;
	 */
	if( xTaskCreate( task_button, "task_button", 4098, NULL, 5, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_button.\n" );	
		return;		
	}

	/**
	 * Task responsável pelas requisições (GET) http;
	 */
	if( xTaskCreate( task_http, "task_http", 10000, NULL, 5, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_http.\n" );	
		return;		
	}
}
