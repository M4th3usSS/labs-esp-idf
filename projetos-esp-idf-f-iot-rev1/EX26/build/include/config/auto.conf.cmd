deps_config := \
	/home/Fernando/esp/esp-idf/components/app_trace/Kconfig \
	/home/Fernando/esp/esp-idf/components/aws_iot/Kconfig \
	/home/Fernando/esp/esp-idf/components/bt/Kconfig \
	/home/Fernando/esp/esp-idf/components/driver/Kconfig \
	/home/Fernando/esp/esp-idf/components/esp32/Kconfig \
	/home/Fernando/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/Fernando/esp/esp-idf/components/espmqtt/Kconfig \
	/home/Fernando/esp/esp-idf/components/ethernet/Kconfig \
	/home/Fernando/esp/esp-idf/components/fatfs/Kconfig \
	/home/Fernando/esp/esp-idf/components/freertos/Kconfig \
	/home/Fernando/esp/esp-idf/components/heap/Kconfig \
	/home/Fernando/esp/esp-idf/components/libsodium/Kconfig \
	/home/Fernando/esp/esp-idf/components/log/Kconfig \
	/home/Fernando/esp/esp-idf/components/lwip/Kconfig \
	/home/Fernando/esp/esp-idf/components/mbedtls/Kconfig \
	/home/Fernando/esp/esp-idf/components/openssl/Kconfig \
	/home/Fernando/esp/esp-idf/components/pthread/Kconfig \
	/home/Fernando/esp/esp-idf/components/spi_flash/Kconfig \
	/home/Fernando/esp/esp-idf/components/spiffs/Kconfig \
	/home/Fernando/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/Fernando/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/Fernando/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/Fernando/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/Fernando/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/Fernando/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
