cmake_minimum_required(VERSION 3.20)
project(VGClient LANGUAGES C CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "VG-CLIENT requires Windows x64")
endif()
option(S4_LOCAL_XUID_OVERRIDE "Enable persistent local XUID" ON)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/systemlink_lobby.lua")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/systemlink_lobby.lua" S4_SYSTEMLINK_LUA)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/legacy_main_menu.lua")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/legacy_main_menu.lua" S4_LEGACY_MENU_LUA)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/runtime_prelude.lua")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Lua/runtime_prelude.lua" S4_RUNTIME_PRELUDE_LUA)
configure_file(Resources/Lua/main_menu_addon.hpp.in "${CMAKE_BINARY_DIR}/generated/main_menu_addon.hpp" @ONLY)
add_subdirectory(ThirdParty/MinHook EXCLUDE_FROM_ALL)
add_library(version SHARED
    Src/Client/Lua/autoload.cpp
    Src/Client/Lua/lua_vm.cpp
    Src/Client/Lua/runtime.cpp
    Src/Client/Lua/lua_hook.cpp
    Src/Client/Lua/lua_registry.cpp
    Src/Client/Patches/systemlink_lobby.cpp
    Src/Client/Utils/hook.cpp
    Src/Client/addresses.cpp
    Src/Client/auth_lua.cpp
    Src/Client/auth.cpp
    Src/Client/commands.cpp
    Src/Client/component_runtime.cpp
    Src/Client/console.cpp
    Src/Client/fov.cpp
    Src/Client/local_profile.cpp
    Src/Client/loot.cpp
    Src/Client/ownership_fence.cpp
    Src/Client/logger.cpp
    Src/Client/lua.cpp
    Src/Client/offsets.cpp
    Src/Client/patches.cpp
    Src/Client/postload.cpp
    Src/Client/profile_format.cpp
    Src/Loader/component_loader.cpp
    Src/Proxy/version.def
    Src/Proxy/version_proxy.c)
target_compile_features(version PRIVATE cxx_std_17)
target_compile_definitions(version PRIVATE S4_WITH_COMPONENTS S4_MINIMAL_HOOKS S4_BUILDING_CLIENT
    WIN32_LEAN_AND_MEAN NOMINMAX S4_LOCAL_XUID_OVERRIDE=$<BOOL:${S4_LOCAL_XUID_OVERRIDE}>)
target_include_directories(version PRIVATE Src Src/Client "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(version PRIVATE minhook user32 kernel32 bcrypt)
set_target_properties(version PROPERTIES PREFIX "" OUTPUT_NAME "version"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(MSVC)
    target_compile_options(version PRIVATE /W4 /EHsc)
elseif(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(version PRIVATE -static -static-libgcc -static-libstdc++)
endif()
