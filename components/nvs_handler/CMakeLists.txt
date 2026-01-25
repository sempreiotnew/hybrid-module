idf_component_register(
    SRCS "nvs_handler.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash json types 
)
