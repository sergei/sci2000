set(sources
        ./wmm.c
        ./GeomagnetismLibrary.c
        ./mem_file.c
        )

idf_component_register(SRCS ${sources}
        INCLUDE_DIRS ./
        )
