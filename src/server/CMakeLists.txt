add_executable(GodotPPServer
    src/main.cpp
        include/types.h
        include/scene/components.h
        include/scene/scene.h
        src/scene/scene.cpp
)

target_include_directories(GodotPPServer PRIVATE include)

target_link_libraries(GodotPPServer PRIVATE
        snl
        shared
        EnTT::EnTT)