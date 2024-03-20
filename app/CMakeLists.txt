add_executable(${PROJECT_NAME} main.cpp common.hpp manager.hpp manager.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE compile-options ${X11_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR}
                                                   "${PROJECT_SOURCE_DIR}/app")
