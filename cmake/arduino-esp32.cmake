if(NOT EXISTS ${CMAKE_SOURCE_DIR}/components/arduino-esp32/idf_component.yml)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/components/arduino-esp32)
    cmake_policy(SET CMP0169 OLD)
    FetchContent_Declare(
            arduino_esp32
            GIT_REPOSITORY https://github.com/espressif/arduino-esp32.git
            GIT_TAG        9e60bbe4bc05e8d27050d23ce3a61a955c1851eb
            SOURCE_DIR     ${CMAKE_SOURCE_DIR}/components/arduino-esp32
            GIT_PROGRESS   TRUE
    )
    FetchContent_Populate(arduino_esp32)
endif()
