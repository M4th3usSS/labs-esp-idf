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
 * Definições Gerais
 */
#define DEBUG 1
#define SERVER_IP 	"10.1.1.6"
#define SERVER_PORT 9765

#define BUTTON	GPIO_NUM_17 

#define EXAMPLE_ESP_WIFI_SSID 	"FSimplicio"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"

/**
 * Variáveis
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "main: ";

/**
 * Protótipos
 */
static esp_err_t event_handler( void *ctx, system_event_t *event );
void wifi_init_sta( void );
void socket_client( int count );
void task_button ( void *pvParameter );
void app_main( void );

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador.
  ref: http://esp32.info/docs/esp_idf/html/dc/df5/esp__wifi_8h_source.html
*/
static esp_err_t event_handler( void *ctx, system_event_t *event )
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
	
    ESP_ERROR_CHECK(esp_event_loop_init( event_handler, NULL ) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init( &cfg ) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI( TAG, "wifi_init_sta finished." );
    ESP_LOGI( TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS );
}

/**
 * 1-Configura o ESP32 em modo Socket Client;
 * 2-Conecta ao servidor TCP; 
 * 3-Envia string para o servidor; 
 * 4-Encerra a conexão socket com o servidor;
 */
void socket_client( int count ) 
{
    int rc; 
    char str[20];
	
	if( DEBUG )
		ESP_LOGI(TAG, "socket_client...\n");

	/**
	 * Cria o socket TCP;
	 */
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if( DEBUG )
		ESP_LOGI( TAG, "socket: rc: %d", sock );

	/**
	 * Configura/Vincula o endereço IP e Port do Servidor (Bind);
	 */
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(SERVER_PORT);

	/**
	 * Tenta estabelecer a conexão socket TCP entre ESP32 e Servidor;
	 */
	rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
	
	/**
	 * error? (-1)
	 */
	if( DEBUG )
		ESP_LOGI(TAG, "connect rc: %d", rc);

	if( rc == 0 )
	{
		/**
		 * Converte o valor de count para string e envia a string 
		 * ao servidor via socket tcp;
		 */
		sprintf( str, "Count = %d\r\n", count );
		rc = send( sock, str, strlen(str), 0 );
		/**
		 * error durante o envio? (-1)
		 */
		if( DEBUG )
			ESP_LOGI(TAG, "send: rc: %d", rc);		
	}

	rc = close( sock );
	/**
	 * error no fechamento do socket? (-1)
	 */
	if( DEBUG )
		ESP_LOGI(TAG, "close: rc: %d", rc);

}

/**
 * task responsável em ler o nível lógico de button
 * e por enviar o valor de count ao servidor via socket tcp;
 */
void task_button ( void *pvParameter )
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
				/**
				 * Chama a função responsável em enviar o valor de count 
				 * para o servidor tcp;
				 */
				socket_client( count++ );

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
	
	/*
	  Configura a rede WiFi em modo Station;
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda a conexão do ESP32 a rede WiFi local;
	 */
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);	

	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	if( xTaskCreate( task_button, "task_button", 4098, NULL, 5, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_button.\n" );	
		return;		
	}
}
