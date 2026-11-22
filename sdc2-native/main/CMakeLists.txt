idf_component_register(
    SRCS app.cpp board.cpp model.cpp store.cpp web.cpp network.cpp
         ../../src/features/theme/ThemeEngine.cpp
    INCLUDE_DIRS . ../../src/features/theme ../../.deps/ArduinoJson/src
    EMBED_TXTFILES index.html
    PRIV_REQUIRES esp_lcd esp_driver_spi esp_driver_ledc esp_http_client
        esp_http_server esp_wifi esp_netif esp_event nvs_flash app_update
        esp_partition esp_timer esp_app_format spi_flash)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Os -fno-exceptions -fno-rtti -Wno-misleading-indentation)
target_include_directories(${COMPONENT_LIB} SYSTEM PRIVATE ../../.deps/ArduinoJson/src)
