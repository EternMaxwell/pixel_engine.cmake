cmake_minimum_required(VERSION 3.21)

add_executable(pixel_engine_render_ogl_test
    "main.cpp"
)

target_link_libraries(pixel_engine_render_ogl_test PRIVATE pixel_engine::render::ogl)
target_link_libraries(pixel_engine_render_ogl_test PRIVATE pixel_engine::app)
target_link_libraries(pixel_engine_render_ogl_test PRIVATE pixel_engine::window)