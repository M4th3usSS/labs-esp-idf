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
#define DR_REG_RNG_BASE   0x3ff75144
#define MAX_NUMERO_CLIENTES 5

/*Porta que será aberta pelo Servidor TCP*/
#define PORT_NUMBER 9765 

/*
SSID e Password de sua rede WiFi local.
*/
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
void task_socket( void * socket_handle );
static esp_err_t event_handler(void *ctx, system_event_t *event);
void wifi_init_sta( void );
void socket_server( void * vParam );

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
 * Configura rede WiFi em Modo station;
 */
void wifi_init_sta( void )
{
    tcpip_adapter_init();
		
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
 * Task responsável pela configuração do ESP32 em modo socket server; 
 */
void socket_server( void * vParam ) 
{
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	/*
	  Cria o socket TCP.
	*/
	int sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	/**
	 * error?
	 */
	if (sock < 0) 
	{
		if( DEBUG )
			ESP_LOGI( TAG, "socket: %d", sock );
		goto END;
	}

	//Vincula o IP do servidor com a PORTA a ser aberta.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(PORT_NUMBER);
	int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	/**
	 * error?
	 */
	if (rc < 0) 
	{
		if( DEBUG )
			ESP_LOGI( TAG, "bind: %d", rc );
		goto END;
	}

	//Define o tamanho máximo da Queue para atendimento das conexões dos clientes.
	rc = listen( sock, MAX_NUMERO_CLIENTES );
	/**
	 * error?
	 */
	if (rc < 0) 
	{
		if( DEBUG )
			ESP_LOGI( TAG, "listen: %d", rc );
		goto END;
	}
    
    socklen_t clientAddressLength = sizeof( clientAddress );

	for(;;) 
	{
		/*
			Aceita a conexão e retorna o socket do cliente.
			A variável clientSock armazenará o handle da conexão socket do cliente;
		*/
		int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
		
		/**
		 * error? (-1)
		 */
		if (clientSock < 0) 
		{
			if( DEBUG )
				ESP_LOGI(TAG, "error accept: %d", clientSock );
			
		} else {
	  		
	  		/**
	  		 * Se chegou aqui é porque existe um cliente conectado com sucesso ao servidor (ESP32);
	  		 */
	  		if( DEBUG )
				ESP_LOGI(TAG, "Socket Handle ID: %d\r\n", clientSock);	
			/*
			  Cria uma nova task para tratamento dos dados deste socket. Se tiver apenas 1 cliente em todo o 
			  projeto, você pode tratar o cliente sem criar uma nova task, porém, para trabalhar com multiplas
			  conexões socket, ou seja, com vários clientes conectados ao ESP32 (Server TCP) tem que criar essa
			  task abaixo.  Essa task é necessária pois precisamos informar o handle do socket para podemos receber
			  ou enviar dados para o cliente. E por isso é necessário rodar em outra task.
			*/
			xTaskCreate( task_socket,"task_socket", 10000, (void*)clientSock, 5, NULL );
		}
		
	}
	END:
	/*
	  Se chegou aqui é porque houve falha na abertura do socket Server, ou por falta de memória ou por ter
	  configurado o IP errado do servidor. 
	*/
	if( DEBUG )
		ESP_LOGI(TAG, "socket_server foi encerrada...\r\n");

	vTaskDelete(NULL);
}

/*
  Esta task é responsável em tratar as multiplas conexões sockets dos clientes. 
  Para testarmos envio e recepção de dados, o cliente envia a string "CMD" e recebe um número Randômico
  como resposta. 

*/
void task_socket( void * socket_handle )
{
	int clientSock = (int) socket_handle;
	/*
	  Aumente o tamanho do buffer de recepção conforme o tamanho da mensagem a ser recebida ou enviada. 
	  Porém, seja cauteloso, pois será criado um buffer para cada cliente conectado ao servidor.
	*/
	int total =	1*1024;  
	char *data = pvPortMalloc( total );
	if( data == NULL ) 
	{
		if( DEBUG )
			ESP_LOGI(TAG, "pvPortMalloc Error\r\n");
		
		vTaskDelete(NULL); 	  
		return;
	 }
	
	if( DEBUG )
		ESP_LOGI(TAG, "task_socket %d run...\r\n", clientSock );

	for(;;)
	{
		/**
		 * A função recv da LWIP é bloqueante, ou seja, o contador de programa permanecerá
		 * nesta função até que exista bytes a serem lidos no buffer ou até que ocorra um
		 * error ou encerramento do socket;
		 */
		ssize_t sizeRead = recv( clientSock, data, total, 0 );
		
		/**
		 * error? (-1)
		 */
		if (sizeRead < 0) 
		{
			if( DEBUG )
				ESP_LOGI( TAG, "recv: %d", sizeRead );
			break;
		}
		/**
		 * Cliente encerrou a conexão com o servidor?
		 */
		if (sizeRead == 0) {
			break;
		}
		
		/**
		 * Se chegou aqui é porque existem bytes a serem lidos do buffer TCP da LWIP.
		 * Portanto, é preciso ler este buffer;
		 */
		if( DEBUG )
			ESP_LOGI(TAG, "Socket Handle ID: %d\nBytes Recebidos(size: %d)\n Conteudo: %.*s\r\n", clientSock, sizeRead, sizeRead, data );
		
		/*
		   Comando "CMD" enviado pelo cliente. 
		   Em resposta a este comando o Servidor irá enviar um número aleatório (int) ao cliente.
		*/
		if( sizeRead >= 3 && data[0]=='C' && data[1]=='M' && data[2]=='D' )
		{
			
			char randonN_str[20];
			
			/**
			 * Realiza a leitura do registrador de números aleatórios do ESP32;
			 */
			sprintf( randonN_str, "%u\r\n", READ_PERI_REG(DR_REG_RNG_BASE) );
			
			/**
			 * Envia randonN_str para o client;
			 */
			int rc = send( clientSock, randonN_str, strlen(randonN_str), 0 );
			
			/**
			 * Verifica se o envio ocorreu com sucesso;
			 */
			if( DEBUG )
				ESP_LOGI(TAG, "send: rc: %d", rc);
		}
		
		/* Este delay é recomendado pois em algum momento esta task tem que ser bloqueada, caso necessário*/
		vTaskDelay( 300/portTICK_PERIOD_MS );
   
	}

	/*
	   Libera o buffer data utilizado no armazenamento dos bytes recebidos que foi criado de maneira dinâmica;
	*/
	vPortFree( data );

	/**
	 * Encerra a conexão socket a fim de liberar o objeto em memória;
	 */
	close( clientSock );
	
	if( DEBUG )
		ESP_LOGI(TAG, "task_socket %d foi deletada.\r\n", clientSock );

    vTaskDelete(NULL);
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
	
	/*
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda a conexão do ESP32 com a rede WiFi local;
	 */
	xEventGroupWaitBits( wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY );	
	
	/*
	   Task responsável em imprimir o valor do IP recebido pelo roteador da rede. 
	*/
	if( xTaskCreate( socket_server,"Socket", 10000, NULL, 5, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar socket_server.\n" );	
		return;		
	}

}

