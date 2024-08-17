/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/*
	Importante:
	Habilite o Bluetooth via menuconfig 
*/
#if defined(CONFIG_BT_ENABLED)

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#endif 


#define SPP_TAG "BLUE"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXAMPLE_DEVICE_NAME "BLUETOOTH SPP"

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

/* Esta é a função de Callback do Bluetooth (registrada no início do programa) */

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
	
	/* Chamado quando é inicializado o evento do SPP */
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
	/* Chamado após o SDP discovery 
	https://www.bluetooth.com/specifications/assigned-numbers/service-discovery*/
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
	/* Este evento é chamado quando o central BLUE (ESP32) se conectar a uma periferico Bluetooth, tal como o Android */
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
	/* Chamado quando o cliente ou o servidor se desconecta da rede */
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
        break;
	/* Evento chamado logo após "ESP_SPP_INIT_EVT" */
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
	/* Chamado quando é inicializado a conexão do cliente ESP32 */	
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
	/* Chamado quando um dado for recebido pelo Bluetooth */
    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
        esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
		
		//echo
		ESP_LOGI(SPP_TAG, "msg: %.*s", param->data_ind.len, param->data_ind.data);	
		esp_spp_write(param->srv_open.handle, param->data_ind.len, param->data_ind.data);
		
        break;
	/* Chamado quando o bit de congestionamento de dados for sinalizado */
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
	/* Chamado quando um dado for enviado com sucesso */
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
	/* Chamado quando o Bluetooth server (ex: Smartphone) se conectar ao bluetooth cliente, no caso o ESP32 */
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
        break;
    default:
        break;
    }
}


void app_main()
{
    /* Inicializa o setor de flash não volátil */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    /* Inicializa o controlador do Bluetooth */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e Verifica se o controlador do Bluetooth foi inicializado corretamente */
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Bluedroid é o nome do Stack BLUE da Espressif. Inicializa o Stack Bluetooth*/
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e verifica se o stack bluetooth foi inicializado corretamente */
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Registra a função de Callback responsável em receber e tratar todos os eventos do rádio Bluetooth */
    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* O bluetooth possui vários perfis. Dentre eles o SPP (Serial Port Profile). Este perfil pertence ao Bluetooth clássico
	e é por meio dele que fazemos uma comunicação serial transparente (ponto a ponto). 
	Habilita e verifica se o perfil SPP do Bluetooth.*/
    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
}

