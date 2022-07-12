# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
# target_sources(ESP32_Led_RTOS
#     PRIVATE
#         main.c
#         touch.c
#         touch.h
#         wifi.c
#         wifi.h
#         led_driver.c
#         led_driver.h
#     )
idf_component_register(SRCS "main.c" "touch.c" "wifi.c" "led_driver.c"
INCLUDE_DIRS ".")
