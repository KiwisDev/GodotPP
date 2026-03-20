add_library(shared INTERFACE
        include/net_serializer.h
        include/maths_types.h)
target_include_directories(shared INTERFACE include)
