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
#include "esp_event.h"

/**
 * Logs
 */
#include "esp_system.h"
#include "esp_log.h"
#include <errno.h>

/**
 * NVS;
 */
#include "nvs_flash.h"

/**
 *  LWIP;
 */
#include "esp_netif.h"
#include "netdb.h"
#include "lwip/dns.h"
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
 * Variáveis
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "SERVER";

/**
 * Protótipos
 */
static void task_socket_server(void * pvParameter);
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
static void task_socket(void * socket_handle);
static void wifi_init_sta(void); 

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
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
	{
        esp_wifi_connect();
    } else 
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
	{
        esp_wifi_connect();
        xEventGroupClearBits( wifi_event_group, WIFI_CONNECTED_BIT );
    }else 
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
	{
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));   
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta( void )
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *netif_sta = esp_netif_create_default_wifi_sta();
    /**
     * Deseja trabalhar com o endereço IP fixo na rede? 
     */		
#if IP_FIXO
    esp_netif_dhcpc_stop(netif_sta);
    esp_netif_ip_info_t ip_info;

	IP4_ADDR(&ip_info.ip,      192,168,0,222 );
	IP4_ADDR(&ip_info.gw,      192,168,0,1   );
	IP4_ADDR(&ip_info.netmask, 255,255,255,0 );
                                    
    esp_netif_set_ip_info(netif_sta, &ip_info);

	/**
		I referred from here.
		https://www.esp32.com/viewtopic.php?t=5380
		if we should not be using DHCP (for example we are using static IP addresses),
		then we need to instruct the ESP32 of the locations of the DNS servers manually.
		Google publicly makes available two name servers with the addresses of 8.8.8.8 and 8.8.4.4.
	*/
	
	ip_addr_t d;
	d.type = IPADDR_TYPE_V4;
	d.u_addr.ip4.addr = 0x08080808; //8.8.8.8 dns
	dns_setserver(0, &d);
	d.u_addr.ip4.addr = 0x08080404; //8.8.4.4 dns
	dns_setserver(1, &d);

#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
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
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda a conexão do ESP32 com a rede WiFi local;
	 */
	xEventGroupWaitBits( wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY );	
	
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



