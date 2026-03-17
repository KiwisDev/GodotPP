add_executable(GodotPPServer
    src/main.cpp
        include/types.h
)

target_link_libraries(GodotPPServer PRIVATE
        snl
        shared
        EnTT::EnTT)