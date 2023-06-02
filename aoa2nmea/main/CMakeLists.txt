idf_component_register(
    SRCS
        aoa2nmea_main.cpp
        USBAccHandler.cpp
        N2KHandler.cpp
        LEDBlinker.cpp
    INCLUDE_DIRS
        ""
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")
