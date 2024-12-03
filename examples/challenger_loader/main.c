/* Copyright 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pi_pico_port.h"
#include "esp_loader.h"
#include "example_common.h"
#include "binaries.h"

/*
 * This is how the output from the build tool look like for V4.0.0 and
 *   for now we have to hardcode the load addresses to match these.
 * A future improvement could be to let have the partition table csv file
 *   in the binaries folder and let the cmake script include these into
 *   the header file.
 * 
 * esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32c6  write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m 
 *     0x0000 build/bootloader/bootloader.bin
 *     0x8000 build/partition_table/partition-table.bin
 *     0xd000 build/ota_data_initial.bin 
 *    0x1e000 build/at_customize.bin
 *    0x1f000 build/customized_partitions/mfg_nvs.bin
 *    0x60000 build/esp-at.bin
 */
#define BOOTLOADER_ADDRESS_V1           0x0
#define PARTITION_TABLE_ADDRESS         0x8000
#define OTA_DATA_INITIAL_ADDRESS        0xd000
#define AT_CUSTOMIZE_ADDRESS            0x1e000
#define CUSTOMIZED_PARTITIONS_ADDRESS   0x1f000
#define ESP_AT_ADDRESS                  0x60000

#define HIGHER_BAUDRATE 2000000

typedef struct {
    partition_attr_t bootloader;
    partition_attr_t partition_table;
    partition_attr_t at_customize;
    partition_attr_t ota_data_initial;
    partition_attr_t mfg_nvs;
    partition_attr_t esp_at;
} esp_at_binaries_t;

void get_esp_at_binaries(target_chip_t target, esp_at_binaries_t *bins)
{
    if (target == ESP32C6_CHIP) {
        bins->bootloader.data = ESP32_C6_bootloader_bin;
        bins->bootloader.size = ESP32_C6_bootloader_bin_size;
        bins->bootloader.addr = BOOTLOADER_ADDRESS_V1;
        bins->partition_table.data = ESP32_C6_partition_table_bin;
        bins->partition_table.size = ESP32_C6_partition_table_bin_size;
        bins->partition_table.addr = PARTITION_TABLE_ADDRESS;
        bins->ota_data_initial.data = ESP32_C6_ota_data_initial_bin;
        bins->ota_data_initial.size = ESP32_C6_ota_data_initial_bin_size;
        bins->ota_data_initial.addr = OTA_DATA_INITIAL_ADDRESS;
        bins->at_customize.data = ESP32_C6_at_customize_bin;
        bins->at_customize.size = ESP32_C6_at_customize_bin_size;
        bins->at_customize.addr = AT_CUSTOMIZE_ADDRESS;
        bins->mfg_nvs.data = ESP32_C6_mfg_nvs_bin;
        bins->mfg_nvs.size = ESP32_C6_mfg_nvs_bin_size;
        bins->mfg_nvs.addr = CUSTOMIZED_PARTITIONS_ADDRESS;
        bins->esp_at.data = ESP32_C6_esp_at_bin;
        bins->esp_at.size = ESP32_C6_esp_at_bin_size;
        bins->esp_at.addr = ESP_AT_ADDRESS;
    }
}

bool repeating_timer_callback(struct repeating_timer *t) {
    gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    return true;
}

int main(void)
{
    esp_at_binaries_t bin;
    struct repeating_timer timer;

    const loader_pi_pico_config_t config = {
        .uart_inst = uart1,
        .baudrate = 115200,
        .uart_rx_pin_num = 5,
        .uart_tx_pin_num = 4,
        .reset_trigger_pin_num = 15,
        .boot_pin_num = 14,
    };

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    stdio_init_all();

    printf("iLabs ESP32-C2/C3/C6 flash utility V1.0\n");
    printf("ESP-AT interpreter V4.0.0.0 for Challenger Boards.\n");
    printf("Baudrate: %d baud.\n", HIGHER_BAUDRATE);

    loader_port_pi_pico_init(&config);

    if (connect_to_target(HIGHER_BAUDRATE) == ESP_LOADER_SUCCESS) {
        add_repeating_timer_ms(250, repeating_timer_callback, NULL, &timer);

        get_esp_at_binaries(esp_loader_get_target(), &bin);

        printf("Writing bootloader... ");
        flash_binary(bin.bootloader.data, bin.bootloader.size, bin.bootloader.addr);
        printf("Writing partition table... ");
        flash_binary(bin.partition_table.data, bin.partition_table.size, bin.partition_table.addr);
        printf("Writing initial OTA data... ");
        flash_binary(bin.ota_data_initial.data, bin.ota_data_initial.size, bin.ota_data_initial.addr);
        printf("Writing AT custom data... ");
        flash_binary(bin.at_customize.data, bin.at_customize.size, bin.at_customize.addr);
        printf("Writing manufacturer non volotile data... ");
        flash_binary(bin.mfg_nvs.data, bin.mfg_nvs.size, bin.mfg_nvs.addr);
        printf("Writing esp-at stack... ");
        flash_binary(bin.esp_at.data,  bin.esp_at.size,  bin.esp_at.addr);
        printf("Done!\n");

        esp_loader_reset_target();
        loader_port_pi_pico_deinit();

        // Delay for skipping the boot message of the targets
        //sleep_ms(500);
        uart_init(uart1, 115200);
        gpio_set_function(config.uart_tx_pin_num, GPIO_FUNC_UART);
        gpio_set_function(config.uart_rx_pin_num, GPIO_FUNC_UART);

        printf("********************************************\n");
        printf("*** Logs below are print from slave .... ***\n");
        printf("********************************************\n");
        char ch;

        cancel_repeating_timer(&timer);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);

        while (true) {
            if (uart_is_readable(uart1)) {
                ch = uart_getc(uart1);
                printf("%c", ch);
            } else {
                tight_loop_contents();
            }
        }
    } else {
        printf("Failed to connect to ESP32\n");
    }

    while (true) {
        tight_loop_contents();
    }
}
