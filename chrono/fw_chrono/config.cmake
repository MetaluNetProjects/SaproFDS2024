target_link_libraries(${CMAKE_PROJECT_NAME} ws2812 fraise_eeprom audio)
#set_target_properties(audio PROPERTIES INTERFACE_COMPILE_OPTIONS "-DPICO_AUDIO_PIO=0")
