add_library(shared STATIC
        include/maths_types.h
        include/utils.h
        include/serialization/serialization_utils.h
        include/serialization/serializer.h)
target_include_directories(shared PUBLIC include)

target_link_libraries(shared PUBLIC glm::glm)