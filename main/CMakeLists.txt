idf_component_register(
    SRCS "main.c" "sketch.cpp"
    INCLUDE_DIRS "."
    REQUIRES bluepad32 bluepad32_arduino arduino btstack
)
