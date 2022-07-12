cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP32_Led_RTOS)
spiffs_create_partition_image(storage website FLASH_IN_PROJECT)
