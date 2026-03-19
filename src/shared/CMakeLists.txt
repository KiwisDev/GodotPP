add_library(shared INTERFACE
        src/net_serializer.cpp
        include/net_serializer.h
        include/maths_types.h)
target_include_directories(shared INTERFACE include)
