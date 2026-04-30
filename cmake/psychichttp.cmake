if(NOT EXISTS ${CMAKE_SOURCE_DIR}/components/PsychicHttp/CMakeLists.txt)
    file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/components/PsychicHttp)
    FetchContent_Declare(
            psychichttp
            GIT_REPOSITORY https://github.com/hoeken/PsychicHttp.git
            GIT_TAG        b719b484d79860fd92623eecd9ed337b63d51905
            SOURCE_DIR     ${CMAKE_SOURCE_DIR}/components/PsychicHttp
            GIT_PROGRESS   TRUE
    )
    FetchContent_Populate(psychichttp)
endif()
