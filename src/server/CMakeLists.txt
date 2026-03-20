add_executable(GodotPPServer
    src/main.cpp
        include/types.h
)

target_include_directories(GodotPPServer PRIVATE include)

target_link_libraries(GodotPPServer PRIVATE
        snl
        shared
        EnTT::EnTT)