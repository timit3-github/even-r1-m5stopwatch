idf_component_register(SRCS "main.c" "r1_wire.c" "r1_legacy.c" INCLUDE_DIRS "." "../include" REQUIRES bt nvs_flash driver esp_timer)
