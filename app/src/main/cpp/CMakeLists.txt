cmake_minimum_required(VERSION 3.22.1)
project(esdroid LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-fdeclspec)
add_compile_options(-fexceptions)
add_compile_options(-funwind-tables)
add_compile_options(-frtti)
add_compile_options("-include${CMAKE_CURRENT_SOURCE_DIR}/android-backend/msvc_compat.h" "-include${CMAKE_CURRENT_SOURCE_DIR}/android-backend/wchar_file_compat.h")
add_compile_options(-Wno-backslash-newline-escape -Wno-error=format-security)
add_compile_options(-Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function)
add_compile_options(-Wno-shift-count-overflow -Wno-shift-op-parentheses)
add_compile_options(-Wno-inconsistent-missing-override -Wno-delete-abstract-non-virtual-dtor)
add_compile_options(-Wno-macro-redefined -Wno-unused-private-field -Wno-unused-but-set-variable)
# optimized build -o3 + lto for maximum performance no semantic
# interposition so lto can inline across the exported symbols too
add_compile_options(-O3 -flto -ffast-math -fno-math-errno -fno-trapping-math
    -fno-semantic-interposition)

add_compile_definitions(ATG_ANDROID)
add_compile_definitions($<$<CONFIG:Debug>:ATG_DEBUG>)
# atg_engine_sim_discord_enabled not defined disabled
add_compile_definitions(ATG_ENGINE_SIM_PIRANHA_ENABLED=1)
add_compile_definitions(WINAPI=)
add_compile_definitions(NDEBUG)

include_directories(
    engine-sim/include engine-sim/scripting/include engine-sim/dependencies/submodules
    delta-studio/include delta-studio/engines/basic/include delta-studio/physics/include
    delta-studio/dependencies/libraries/OpenGL/include delta-studio/dependencies/libraries/stb
   piranha/include piranha/src piranha/dependencies/libraries/flex/include csv-io/include scs/include android-backend ${ANDROID_NDK}/sources/android/native_app_glue
)

# engine-sim core simulation/synthesis only
add_library(es_core STATIC
    engine-sim/src/audio_buffer.cpp
    engine-sim/src/camshaft.cpp
    engine-sim/src/crankshaft.cpp
    engine-sim/src/combustion_chamber.cpp
    engine-sim/src/connecting_rod.cpp
    engine-sim/src/convolution_filter.cpp
    engine-sim/src/cylinder_bank.cpp
    engine-sim/src/cylinder_head.cpp
    engine-sim/src/delay_filter.cpp
    engine-sim/src/derivative_filter.cpp
    engine-sim/src/direct_throttle_linkage.cpp
    engine-sim/src/dynamometer.cpp
    engine-sim/src/engine.cpp
    engine-sim/src/exhaust_system.cpp
    engine-sim/src/feedback_comb_filter.cpp
    engine-sim/src/filter.cpp
    engine-sim/src/fuel.cpp
    engine-sim/src/function.cpp
    engine-sim/src/gas_system.cpp
    engine-sim/src/gaussian_filter.cpp
    engine-sim/src/governor.cpp
    engine-sim/src/ignition_module.cpp
    engine-sim/src/impulse_response.cpp
    engine-sim/src/intake.cpp
    engine-sim/src/jitter_filter.cpp
    engine-sim/src/leveling_filter.cpp
    engine-sim/src/low_pass_filter.cpp
    engine-sim/src/part.cpp
    engine-sim/src/piston.cpp
    engine-sim/src/piston_engine_simulator.cpp
    engine-sim/src/simulator.cpp
    engine-sim/src/standard_valvetrain.cpp
    engine-sim/src/starter_motor.cpp
    engine-sim/src/synthesizer.cpp
    engine-sim/src/throttle.cpp
    engine-sim/src/transmission.cpp
    engine-sim/src/utilities.cpp
    engine-sim/src/valvetrain.cpp
    engine-sim/src/vehicle.cpp
    engine-sim/src/vehicle_drag_constraint.cpp
    engine-sim/src/vtec_valvetrain.cpp
)

# delta-studio delta-core
set(DELTA_CORE_SRCS
    delta-studio/src/yds_animation_action.cpp delta-studio/src/yds_animation_action_binding.cpp
    delta-studio/src/yds_animation_curve.cpp delta-studio/src/yds_animation_interchange_file.cpp
    delta-studio/src/yds_animation_interchange_file_reader.cpp delta-studio/src/yds_animation_interchange_file_reader_0_0.cpp
    delta-studio/src/yds_animation_interchange_file_reader_0_1.cpp delta-studio/src/yds_animation_mixer.cpp
    delta-studio/src/yds_animation_target.cpp delta-studio/src/yds_audio_buffer.cpp
    delta-studio/src/yds_audio_device.cpp delta-studio/src/yds_audio_file.cpp
    delta-studio/src/yds_audio_parameters.cpp delta-studio/src/yds_audio_source.cpp
    delta-studio/src/yds_audio_streaming.cpp delta-studio/src/yds_audio_system.cpp
    delta-studio/src/yds_audio_system_object.cpp delta-studio/src/yds_base.cpp
    delta-studio/src/yds_breakdown_timer.cpp delta-studio/src/yds_breakdown_timer_channel.cpp
    delta-studio/src/yds_color.cpp delta-studio/src/yds_context_object.cpp
    delta-studio/src/yds_depth_buffer.cpp delta-studio/src/yds_device.cpp
    delta-studio/src/yds_dynamic_allocator.cpp delta-studio/src/yds_error_handler.cpp
    delta-studio/src/yds_error_system.cpp delta-studio/src/yds_file.cpp
    delta-studio/src/yds_file_logger.cpp delta-studio/src/yds_geometry_export_file.cpp
    delta-studio/src/yds_geometry_preprocessing.cpp delta-studio/src/yds_gpu_buffer.cpp
    delta-studio/src/yds_input_device.cpp delta-studio/src/yds_input_layout.cpp
    delta-studio/src/yds_input_system.cpp delta-studio/src/yds_interchange_file_0_0.cpp
    delta-studio/src/yds_interchange_file_0_1.cpp delta-studio/src/yds_interchange_object.cpp
    delta-studio/src/yds_keyboard.cpp delta-studio/src/yds_keyboard_aggregator.cpp
    delta-studio/src/yds_linked_list.cpp delta-studio/src/yds_logger.cpp
    delta-studio/src/yds_logger_output.cpp delta-studio/src/yds_math.cpp
    delta-studio/src/yds_memory_base.cpp delta-studio/src/yds_monitor.cpp
    delta-studio/src/yds_mouse.cpp delta-studio/src/yds_mouse_aggregator.cpp
    delta-studio/src/yds_object_animation_data.cpp delta-studio/src/yds_object_data.cpp
    delta-studio/src/yds_opengl_context.cpp delta-studio/src/yds_opengl_device.cpp
    delta-studio/src/yds_opengl_gpu_buffer.cpp delta-studio/src/yds_opengl_input_layout.cpp
    delta-studio/src/yds_opengl_render_target.cpp delta-studio/src/yds_opengl_shader.cpp
    delta-studio/src/yds_opengl_shader_program.cpp delta-studio/src/yds_opengl_texture.cpp
    delta-studio/src/yds_opengl_egl_context.cpp
    delta-studio/src/yds_render_geometry_channel.cpp delta-studio/src/yds_render_geometry_format.cpp
    delta-studio/src/yds_render_target.cpp delta-studio/src/yds_rendering_context.cpp
    delta-studio/src/yds_shader.cpp delta-studio/src/yds_shader_program.cpp
    delta-studio/src/yds_stat.cpp delta-studio/src/yds_stb_image.cpp
    delta-studio/src/yds_texture.cpp delta-studio/src/yds_time_tag_data.cpp
    delta-studio/src/yds_timing.cpp delta-studio/src/yds_tool_animation_file.cpp
    delta-studio/src/yds_tool_geometry_file.cpp delta-studio/src/yds_transform.cpp
    delta-studio/src/yds_window.cpp delta-studio/src/yds_window_event_handler.cpp
    delta-studio/src/yds_window_system.cpp delta-studio/src/yds_window_system_object.cpp
)
add_library(delta_core STATIC ${DELTA_CORE_SRCS})

# delta-studio delta-basic
file(GLOB_RECURSE DELTA_BASIC_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/delta-studio/engines/basic/src/*.cpp")
add_library(delta_basic STATIC ${DELTA_BASIC_SRCS})

# delta-studio physics
file(GLOB_RECURSE DELTA_PHYSICS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/delta-studio/physics/src/*.cpp")
add_library(delta_physics STATIC ${DELTA_PHYSICS_SRCS})

#
# piranha - includes pre-generated parser/scanner files flex+bison
file(GLOB_RECURSE PIRANHA_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/piranha/src/*.cpp")
add_library(piranha STATIC
    ${PIRANHA_SRCS}
    piranha/generated/parser.auto.cpp
    piranha/generated/scanner.auto.cpp
)
target_include_directories(piranha PUBLIC
    piranha/include
    piranha/src
    piranha/generated
    piranha/dependencies/libraries/flex/include
)

# csv-io scs
file(GLOB_RECURSE CSV_IO_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/csv-io/src/*.cpp")
add_library(csv_io STATIC ${CSV_IO_SRCS})
file(GLOB_RECURSE SCS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/scs/src/*.cpp")
add_library(scs STATIC ${SCS_SRCS})

# engine-sim scripting requires piranha
file(GLOB_RECURSE ES_SCRIPT_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/engine-sim/scripting/src/*.cpp")
add_library(es_script STATIC ${ES_SCRIPT_SRCS})
target_link_libraries(es_script PUBLIC piranha)

# android backend
# android native_app_glue local copy with 16mb stack fix
add_library(native_app_glue STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/android_native_app_glue.c
)
target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(android_backend STATIC
    android-backend/android_backend.cpp android-backend/android_shim.cpp android-backend/touch_ui.cpp)

# main shared library
add_library(esdroid SHARED
    android_main.cpp
    engine-sim/src/engine_sim_application.cpp engine-sim/src/geometry_generator.cpp
    engine-sim/src/simulation_object.cpp engine-sim/src/piston_object.cpp
    engine-sim/src/connecting_rod_object.cpp engine-sim/src/ui_element.cpp
    engine-sim/src/ui_manager.cpp engine-sim/src/cylinder_pressure_gauge.cpp
    engine-sim/src/ui_math.cpp engine-sim/src/gauge.cpp
    engine-sim/src/crankshaft_object.cpp engine-sim/src/cylinder_bank_object.cpp
    engine-sim/src/cylinder_head_object.cpp engine-sim/src/ui_button.cpp
    engine-sim/src/ui_utilities.cpp engine-sim/src/combustion_chamber_object.cpp
    engine-sim/src/oscilloscope.cpp engine-sim/src/shaders.cpp
    engine-sim/src/engine_view.cpp engine-sim/src/right_gauge_cluster.cpp
    engine-sim/src/cylinder_temperature_gauge.cpp engine-sim/src/labeled_gauge.cpp
    engine-sim/src/throttle_display.cpp engine-sim/src/afr_cluster.cpp
    engine-sim/src/fuel_cluster.cpp engine-sim/src/oscilloscope_cluster.cpp
    engine-sim/src/performance_cluster.cpp engine-sim/src/firing_order_display.cpp
    engine-sim/src/load_simulation_cluster.cpp engine-sim/src/mixer_cluster.cpp
    engine-sim/src/info_cluster.cpp
)

target_link_libraries(esdroid PRIVATE
    es_core es_script delta_core delta_basic delta_physics piranha csv_io scs android_backend
    EGL GLESv3 OpenSLES android log atomic -Wl,--whole-archive native_app_glue -Wl,--no-whole-archive
    -flto -Wl,--lto-O3 -Wl,--icf=all
)
