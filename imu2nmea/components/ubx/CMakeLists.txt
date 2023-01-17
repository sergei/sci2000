set(sources
        ./UbxParser.cpp
        )

idf_component_register(SRCS ${sources}
        INCLUDE_DIRS ./
        )
