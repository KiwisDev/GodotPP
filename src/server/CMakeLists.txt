add_executable(GodotPPServer
    src/main.cpp
        ../shared/PacketDefinition.h
)

target_link_libraries(GodotPPServer PRIVATE
        snl
        EnTT::EnTT)