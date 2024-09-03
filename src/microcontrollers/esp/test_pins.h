#pragma once

/**
  Added 2024/09/02
  Initially used for debugging to test if pins were working correctly
  this tests pins saved in an array
  TODO: make this more useful
*/
static void _test_pins() {
    uint8_t i = 0, j = 0;
    uint8_t pin = i;
    while (i < 8) {
        pin = (i > 3) ? 7 - i : i;
        ESP_LOGI("Testing Pin", "D%d", pin + 4);
        _TEST_PIN_(config.data_pins[pin], 500);
        i++;
    }

    i = 0;
    while (i < 2) {
        pin = i ? config.rs : config.e;
        ESP_LOGI("Testing Pin", "%s (pin %d)", (pin == config.e) ? "enable" : "rw", pin);
        j = 0;
        while (j < 20) {
            _TEST_PIN_(pin, 100);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            j++;
        }
        i++; 
    }
}
