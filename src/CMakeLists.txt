idf_component_register(SRCS "main.c" "r1_wire.c" "r1_legacy.c" "r1_inputs.c" "r1_battery.c" "r1_battery_math.c" INCLUDE_DIRS "." "../include" REQUIRES bt nvs_flash driver esp_timer esp_driver_i2c)
