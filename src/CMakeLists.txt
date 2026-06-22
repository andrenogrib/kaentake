# Build injector
add_library(injector SHARED
    injector.cpp
    system.cpp
    bypass.cpp
    inlink.cpp
    resman.cpp
    avatar.cpp
    itemeff.cpp
    resolution.cpp
    mobhptag.cpp
    tooltip.cpp
    itemicon.cpp
    tempstat.cpp
    stringpool.cpp
    debug.cpp
    hook.cpp
    # world-map info tooltip feature
    worldmapinfo.cpp
    getwzinfo.cpp
    iconprovider.cpp
    # discord rich presence feature
    discordintegration.cpp
    discord/discord_rpc.cpp
    discord/connection_win.cpp
    discord/rpc_connection.cpp
    discord/serialization.cpp
    discord/discord_register_win.cpp
)

target_precompile_headers(injector PRIVATE pch.h)

# the vendored discord-rpc sources are self-contained third-party code: don't force
# our pch.h into them (they include their own windows.h with custom lean macros).
set_source_files_properties(
    discord/discord_rpc.cpp
    discord/connection_win.cpp
    discord/rpc_connection.cpp
    discord/serialization.cpp
    discord/discord_register_win.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# discord-rpc bundles its own rapidjson under src/discord/
target_include_directories(injector PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/discord"
)

# the WZ readers and discord sources use CRT string fns (swprintf/_itow_s/strcpy)
target_compile_definitions(injector PRIVATE
    _CRT_NON_CONFORMING_SWPRINTFS
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(injector PRIVATE
    Detours
    WzLib
)

set_target_properties(injector PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
)

add_custom_command(TARGET injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Custom.wz"
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    # Drop the ready-to-ship files into <project>/dist so they can be copied
    # straight into a v83 client folder.
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:injector>"
        "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Custom.wz"
        "${CMAKE_SOURCE_DIR}/dist"
    COMMENT "Staging Kaentake.dll + Custom.wz into dist/"
)


# Build launcher
add_executable(launcher WIN32
    launcher.manifest
    launcher.rc
    launcher.cpp
    debug.cpp
)

target_link_options(launcher PRIVATE "/MANIFEST:NO")

target_link_libraries(launcher PRIVATE
    Detours
)

set_target_properties(launcher PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
)

add_custom_command(TARGET launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:launcher>"
        "${CMAKE_SOURCE_DIR}/dist"
    COMMENT "Staging Kaentake.exe into dist/"
)