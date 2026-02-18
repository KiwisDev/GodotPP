add_executable(GodotPPServer
    src/main.cpp
)

target_link_libraries(GodotPPServer PRIVATE
        snl
        EnTT::EnTT)