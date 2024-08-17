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
#include "freertos/queue.h"
#include "freertos/semphr.h"
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
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/sockets.h>
/**
 * Erros
 */
#include <errno.h>

/**
 * Definições
 */
#define DEBUG 		1
#define SERVER_IP 	"52.73.133.37"
#define SERVER_PORT 80

#define BUTTON	GPIO_NUM_17 

#define EXAMPLE_ESP_WIFI_SSID 	"FSimplicio"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"

/**
 * Variáveis
 */
QueueHandle_t Queue_Button;
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "main: ";

typedef struct xData {
 	int sock; 
 	uint32_t counter; 
} xSocket_t; 

/**
 * Protótipos
 */
static esp_err_t event_handler(void *ctx, system_event_t *event);
void wifi_init_sta( void );
void task_send_receive( void * pvParameter );
void task_button ( void *pvParameter );
void app_main( void );

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
			ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
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
 * Configuração do WiFi;
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


/**
 * Socket Client;
 */
void task_socket ( void * pvParameter ) 
{
    int rc; 
	uint32_t counter; 
	xSocket_t xSocket;
	
	if( DEBUG )
		ESP_LOGI(TAG, "Task_Socket run ...\r\n");
	
    for(;;) 
    {
		/**
		 * Aguarda o Button ser pressionado.
		 */
		xQueueReceive( Queue_Button, &counter, portMAX_DELAY ); 
		

		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if( DEBUG )
			ESP_LOGI(TAG, "socket: rc: %d", sock);
		
		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		
		/**
		 * Registra o endereço IP e PORTA do servidor;
		 */
		inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr.s_addr);
		serverAddress.sin_port = htons(SERVER_PORT);

		/**
		 * Tenta realiza a conexão socket com o servidor; 
		 * Caso a conexão ocorra com sucesso, será retornado 0, caso contrário, -1;
		 */
		rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
		
		if( DEBUG )
			ESP_LOGI(TAG, "Status Socket: %d", rc);
		
		if( rc == -1 ) 
		{
			if( DEBUG )
				ESP_LOGI(TAG, "xiii Socket Error: %d", sock);

			/**
			 * Aguarda 5 segundos antes de abrir um novo socket;
			 */
			for( int i = 1; i <= 5; ++i )
			{
				if( DEBUG )
					ESP_LOGI(TAG, "timeout: %d", 5-i);

				vTaskDelay( 1000/portTICK_PERIOD_MS );
			}
			continue; 
		} 

		/**
		 * Se chegou aqui é porque o ESP32 está conectado com sucesso com o servidor Web.
		 * Portanto, é possível criar uma Task para enviar e receber dados com o servidor.
		 * O legal deste programa é que cada vez que button for pressionado uma nova task/thread
		 * será criada;
		 */
		xSocket.sock = sock; 
		xSocket.counter = counter; 
		
		/**
		 * Cria uma thread/task responsável em enviar e ler as solicitações trocadas
		 * com o servidor web;
		 */
	    xTaskCreate( task_send_receive, "task_send_rec", 10000, (void*)&(xSocket), 5, NULL );
		
	}

	vTaskDelete( NULL );

}

/**
 * Task Responsável em enviar e receber dados com o servidor web;
 */
void task_send_receive ( void * pvParameter ) 
{
	int rec_offset = 0; 
	/**
	 * Para armazenar os bytes recebidos do servidor Web 
	 * vou utilizar um buffer de 1024 bytes criado dinamicamente;
	 */
	int total =	1*1024; 
	char *buffer = pvPortMalloc( total );
	if( buffer == NULL ) 
	{
		if( DEBUG )
			ESP_LOGI(TAG, "pvPortMalloc Error\r\n");
		vTaskDelete(NULL); 	  
		return;
	 }
	 
	/**
	 * Recebe o Socket da conexão com o servidor web;
	 */
    xSocket_t * xSocket = (xSocket_t*) pvParameter;
	
	/***********************************************
	 * Prepara o cabeçalho HTTP. 
	 * Neste programa o cabeçalho HTTP é fixo, porém o conteúdo enviado para o servidor web
	 * pode ser alterado;
	 * O valor da variável counter (incrementada cada vez que button for pressionado) será 
	 * enviado para a web. 
	 * Segue exemplo da mensagem a ser enviada via POST:

		POST /things/services/api/v1/variables/s00/value/?token=7f33f3ce94f774494feb8f1843511509 HTTP/1.1
		HOST: www.geniot.io
		content-type: application/json
		content-length: 13

		{"value":123}

	 */
	const char * msg_post = \
				"POST /things/services/api/v1/variables/s00/value/?token=7f33f3ce94f774494feb8f1843511509 HTTP/1.1\r\n"
				"HOST: geniot.io\r\n"
				"content-type: application/json\r\n"
				"content-length: ";
		
	char databody[50];
	sprintf( databody, "{\"value\":%d}", xSocket->counter );
	sprintf( buffer , "%s%d\r\n\r\n%s\r\n\r\n", msg_post, strlen(databody), databody); 
	
	/****************************************************
	 * OK, agora é necessário enviar o cabeçalho HTTP para o servidor Web; 
	 * Lembre-se que já estamos conectados ao servidor Web. 
	 */

	int rc = send( xSocket->sock, buffer, strlen(buffer), 0 );

	if( DEBUG )
		ESP_LOGI(TAG, "Cabecalho HTTP Enviado? rc: %d", rc);
	
	for(;;)
	{
	  
	  	/**
	  	 * Aguarda o Json de Retorno do Servidor Web. 
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
		/* sizeRead armazena a quantidade de bytes recebidos e armazenados em buffer */
		ssize_t sizeRead = recv(xSocket->sock, buffer+rec_offset, total-rec_offset, 0);
		
		/**
		 * Error durante a leitura. Devemos encerrar o socket; 
		 */
		if ( sizeRead == -1 ) 
		{
			if( DEBUG )
				ESP_LOGI( TAG, "recv: %d", sizeRead );
			break;
		}
		/**
		 * Servidor Web encerrou a conexão com o ESP32; Devemos encerrar o socket; 
		 */
		if ( sizeRead == 0 ) 
		{
			break;
		}
		
		/**
		 * Imprime na saída do console a resposta em Json do servidor Web;
		 * Lembre-se que os pacotes podem chegar fragmentados. Isso é do TCP! 
		 * Portanto, é necessário utilizar uma variável de offset para ir sempre 
		 * populando o buffer a partir de determinados indices;
		 */
		if( sizeRead > 0 ) 
		{	
			if( DEBUG ) 
		    	ESP_LOGI(TAG, "Socket: %d - Data read (size: %d) was: %.*s", xSocket->sock, sizeRead, sizeRead, buffer);
		   
		   rec_offset += sizeRead; 
		 }
        
        /**
         * É preciso dar oportunidade para o scheduler trocar o contexto e
         * chamar outras tasks;
         */
		vTaskDelay( 5/portTICK_PERIOD_MS );
	}
	
	/**
	 * Se chegou aqui é porque o servidor web desconectou o client;
	 * Portanto, fecha a conexão socket, libera o buffer da memória e deleta a task;
	 */
	rc = close(xSocket->sock);
	
	if( DEBUG )
		ESP_LOGI(TAG, "close: rc: %d", rc); 
	
	vPortFree( buffer );	

	vTaskDelete(NULL); 
}

/**
 * Task responsável pela manipulação buttton;
 */
void task_button ( void *pvParameter )
{
    int aux=0;
	uint32_t counter = 0;
		
    /* Configura a GPIO 17 como saída */ 
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 

	if( DEBUG )
    	ESP_LOGI(TAG, "Leitura da GPIO BUTTON\n");
    
    for(;;) 
	{
	
		if(gpio_get_level( BUTTON ) == 0 && aux == 0)
		{ 
		  /* Delay para tratamento do bounce da tecla*/
		  vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
		  if(gpio_get_level( BUTTON ) == 0) 
		  {			
			//Somente envia o valor de counter para a Fila caso exista espaço.
			if( xQueueSend(Queue_Button, (void *)&counter, (TickType_t )0) )
			{
			   counter++;
			}	
			
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

/**
 * Inicio da aplicação; 
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
	 * Queue button é responsável em armazenar o valor do contador "count" incrementado
	 * cada vez que button for pressionado. O valor de count será enviado via Json POST
	 * para o servidor Web;
	 */
	Queue_Button = xQueueCreate( 2, sizeof(uint32_t) );

	/*
	  Configura a rede WiFi em modo station;
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda o ESP32 se conectar a rede WiFi local.
	 */
	xEventGroupWaitBits( wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY );	
	
	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	if( xTaskCreate( task_button, "task_button", 4098, NULL, 5, NULL )!= pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_button.\r\n" );  
      return;   
    }
    /**
     * Task responsável em receber o valor de count e enviar via POST a um 
     * servidor web;
     */
	if( xTaskCreate( task_socket, "task_socket", 10000, NULL, 5, NULL )!= pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_socket.\r\n" );  
      return;   
    }
}
