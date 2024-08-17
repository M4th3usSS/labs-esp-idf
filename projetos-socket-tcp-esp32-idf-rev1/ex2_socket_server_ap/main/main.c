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
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * WiFi
 */
#include "esp_wifi.h"
/**
 * Callback do WiFi
 */
#include "esp_event.h"

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
 *  LWIP;
 */
#include "esp_netif.h"
#include "netdb.h"
#include "lwip/dns.h"

/**
 * LWIP
 */
#include "lwip/api.h"
#include "lwip/err.h"
#include <sys/socket.h>
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"

/**
 *  mDNS lib;
 */
#include "mdns.h"

/**
 * Lib socket server;
 */
#include "config.h" 

/**
 * Variáveis;
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT 	= BIT0;
const int WIFI_DISCONNECTED_BIT = BIT1;
static const char *TAG = "SERVER-AP";
static esp_netif_t* esp_netif_ap;

/**
 * Protótipos
 */
static void task_socket_server(void * pvParameter);
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
static void task_socket(void * socket_handle);
static void wifi_init_ap( void ); 

/**
 * Task responsável pela configuração do ESP32 em modo socket server; 
 */
static void task_socket_server(void * pvParameter)
{
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	/**
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
	}

	/**
	 *  Vincula o IP do servidor com a PORTA a ser aberta.
	 */
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
	}

	/**
	 *  Define o tamanho máximo da Queue para atendimento das conexões dos clientes.
	 */
	rc = listen( sock, MAX_NUMERO_CLIENTES );
	
	/**
	 * error?
	 */
	if (rc < 0) 
	{
		if( DEBUG )
			ESP_LOGI( TAG, "listen: %d", rc );
	}
    
    socklen_t clientAddressLength = sizeof( clientAddress );

	for(;;) 
	{
		/**
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
			
			/**
			  Cria uma nova task para tratamento dos dados deste socket. Se tiver apenas 1 cliente em todo o 
			  projeto, você pode tratar o cliente sem criar uma nova task, porém, para trabalhar com multiplas
			  conexões socket, ou seja, com vários clientes conectados ao ESP32 (Server TCP) tem que criar essa
			  task abaixo.  Essa task é necessária pois precisamos informar o handle do socket para podemos receber
			  ou enviar dados para o cliente. E por isso é necessário rodar em outra task.
			*/
			if( xTaskCreate( task_socket,"task_socket", 1024*10, (void*)clientSock, 2, NULL ) != pdTRUE )
			{
				if( DEBUG )
					ESP_LOGI( TAG, "error - nao foi possivel alocar task_socket\n" );	

			}
		}
		
	}
	
	/**
	  Se chegou aqui é porque houve falha na abertura do socket Server, ou por falta de memória ou por ter
	  configurado o IP errado do servidor. 
	*/
	if( DEBUG )
		ESP_LOGI(TAG, "socket_server foi encerrada...\n");

	vTaskDelete(NULL);
}

/**
  Esta task é responsável em tratar as multiplas conexões sockets dos clientes. 
  Para testarmos envio e recepção de dados, o cliente envia a string "CMD" e recebe um número Randômico
  como resposta. 

*/
static void task_socket( void * socket_handle )
{
	int clientSock = (int) socket_handle;
	
	if( DEBUG )
		ESP_LOGI(TAG, "task_socket %d run...\r\n", clientSock );
	/**
	  Aumente o tamanho do buffer de recepção conforme o tamanho da mensagem a ser recebida ou enviada. 
	  Porém, seja cauteloso, pois será criado um buffer para cada cliente conectado ao servidor.
	*/
	char buffer[64];  
	
	for(;;)
	{
		/**
		 * A função recv da LWIP é bloqueante, ou seja, o contador de programa permanecerá
		 * nesta função até que exista bytes a serem lidos no buffer ou até que ocorra um
		 * error ou encerramento do socket;
		 */
		ssize_t sizeRead = recv( clientSock, buffer, sizeof(buffer), 0 );
		
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
		 * Verifica se há espaço na variável 'buffer' para o armazenamento do NULL;
		 */
		if(sizeRead < (sizeof(buffer) - 1)) 
		{
			//buffer[sizeRead] = 0; /** Garante o NULL, formando assim uma 'string'*/
			
			/**
			 *  Imprime a mensagem recebida;
			 */		
			ESP_LOGI( TAG, "%.*s", sizeRead, buffer); 

			/**
			 *  Envia pacote de confirmação de entrega;
			 */				
			static unsigned int count = 0;
			snprintf(buffer, sizeof(buffer) - 1, "Packet Num=%d\r\n", count++ ); 
			send(clientSock, buffer, strlen(buffer), 0);	
		}
			
		/** Este delay é recomendado pois em algum momento esta task tem que ser bloqueada, caso necessário*/
		vTaskDelay( 300/portTICK_PERIOD_MS );
   
	}

	/**
	 * Encerra a conexão socket a fim de liberar o objeto em memória;
	 */
	close( clientSock );
	
	if( DEBUG )
		ESP_LOGI(TAG, "task_socket %d foi deletada.\r\n", clientSock );

	/**
	 * Desaloca esta task do heap;
	 */
    vTaskDelete(NULL);
}



/**
 * Função de callback chamada quando é alterado o estado da
 * conexão WiFi;
 */
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) 
	{
       	if( DEBUG )
			ESP_LOGI(TAG, "ESP32 Inicializado em modo AP.\n");
    } else 
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) 
	{
        xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECTED_BIT);
    } else 
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) 
	{
        xEventGroupSetBits( wifi_event_group, WIFI_CONNECTED_BIT );	
    }
}

/**
 * Configura a rede WiFi do ESP32 em Modo Acess Point;
 */
static void wifi_init_ap( void )
{
	esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
														
    wifi_config_t wifi_config = 
	{
        .ap = 
		{
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 5,
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
    if ( strlen( WIFI_PASS ) == 0 ) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    if( DEBUG )
    	ESP_LOGI(TAG, "wifi_init_ap finished.SSID:%s password:%s",
             WIFI_SSID, WIFI_PASS);
}


/**
 * Configura o rede Acess Point do ESP32;
 */
static void start_dhcp_server( void )
{  
	ESP_ERROR_CHECK(esp_netif_init());
    /*
		Registra a função de callback dos eventos do WiFi.
	*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init( &cfg ));
	
	/* IP FIXO */
	esp_netif_dhcps_stop(esp_netif_ap); 
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
	inet_pton(AF_INET, AP_IP, &ap_ip_info.ip);
	inet_pton(AF_INET, AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, AP_MASCARA, &ap_ip_info.netmask);
	ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));
	esp_netif_dhcps_start(esp_netif_ap);
	/* FIM IP FIXO */	
	
	if( DEBUG )
		ESP_LOGI( TAG, "Servico DHCP reinicializado.\r\n");
}


void app_main(void)
{
    /**
		Inicialização da memória não volátil para armazenamento de dados (Non-volatile storage (NVS)).
		Necessário para realização da calibração do PHY. 
	*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
	/**
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação TCP quando a rede WiFi 
	   estiver configurada, ou seja, somente após o aceite de conexão e a 
	   liberação do IP pelo roteador da rede (nos casos de IPs dinâmicos).
	*/
	wifi_event_group = xEventGroupCreate();
	
	/**
	 * Inicializa o servidor DHCP e define o IP statico da rede;
	 * Esta configuração é necessária pois a rede WiFi será inicializada
	 * em modo Acess Point, ou seja, será criada uma rede com o ssid "esp32"
	 * de password "esp32pwd".
	 */
	start_dhcp_server();

	/**
	 * Inicicializa a rede WiFi no modo Acess Point;
	 */
	wifi_init_ap();
		
	/**
	   Task responsável em imprimir o valor do IP recebido pelo roteador da rede. 
	*/
	if( xTaskCreate( task_socket_server,"task_socket_server", 1024*10, NULL, 2, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_socket_server.\n" );	
		return;		
	}

}

