cmake_minimum_required( VERSION 3.20 )

if( NOT CMAKE_TOOLCHAIN_FILE )
    set( CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/mingw-w64-toolchain.cmake" CACHE FILEPATH "" FORCE )
endif()

project( MecchaCheatV LANGUAGES CXX C )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_C_STANDARD 17 )
set( CMAKE_C_STANDARD_REQUIRED ON )

set( ROOT "${CMAKE_SOURCE_DIR}/.." )
set( DLL_ROOT "${ROOT}/client" )
set( INJECTOR_ROOT "${ROOT}/injector" )
set( PACK_ROOT "${ROOT}/pack" )

add_compile_definitions(
    NDEBUG
    MECCHACHEATV_EXPORTS
    _WINDOWS
    _USRDLL
)

add_compile_options(
    -Wno-invalid-offsetof
    -Wno-narrowing
    -mms-bitfields
    -fpermissive
    -fms-extensions
)

file( GLOB MINHOOK_SOURCES
    "${DLL_ROOT}/libs/minhook/*.c"
    "${DLL_ROOT}/libs/minhook/hde/*.c"
)

add_library( minhook STATIC ${MINHOOK_SOURCES} )
target_include_directories( minhook PUBLIC "${DLL_ROOT}/libs/minhook" )

set( DLL_SOURCES
    "${DLL_ROOT}/dllmain.cpp"
    "${DLL_ROOT}/libs/imgui/imgui.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_draw.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_tables.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_widgets.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_impl_dx11.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_impl_dx12.cpp"
    "${DLL_ROOT}/libs/imgui/imgui_impl_win32.cpp"
    "${DLL_ROOT}/main/features/feats/alwvisible/alwvisible.cpp"
    "${DLL_ROOT}/main/features/feats/autodisshadow/autodisshadow.cpp"
    "${DLL_ROOT}/main/features/feats/decoyesp/decoyesp.cpp"
    "${DLL_ROOT}/main/features/feats/nodetection/nodetection.cpp"
    "${DLL_ROOT}/main/features/feats/playeresp/playeresp.cpp"
    "${DLL_ROOT}/main/features/feats/setname/setname.cpp"
    "${DLL_ROOT}/main/features/feats/sprintmultiplier/sprintmultiplier.cpp"
    "${DLL_ROOT}/main/features/feats/teleport/teleport.cpp"
    "${DLL_ROOT}/main/features/feats/webonly/webonly.cpp"
    "${DLL_ROOT}/main/features/features.cpp"
    "${DLL_ROOT}/main/hooking/hooking.cpp"
    "${DLL_ROOT}/main/hooking/hooks/Present.cpp"
    "${DLL_ROOT}/main/hooking/hooks/ResizeBuffers.cpp"
    "${DLL_ROOT}/main/hooking/hooks/ProcessEvent.cpp"
    "${DLL_ROOT}/main/hooking/hooks/WndProc.cpp"
    "${DLL_ROOT}/main/logger/logger.cpp"
    "${DLL_ROOT}/main/menu/menu.cpp"
    "${DLL_ROOT}/main/menu/renderer.cpp"
    "${DLL_ROOT}/main/sdk/SDK/Basic.cpp"
    "${DLL_ROOT}/main/sdk/SDK/BP_FirstPersonCharacter_cLeon_Character_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/BP_FirstPersonCharacter_cLeon_Character_Hunter_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/BP_FirstPersonPlayerState_Online_cLeon_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/CoreUObject_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/Engine_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/MoverExamples_functions.cpp"
    "${DLL_ROOT}/main/sdk/SDK/Mover_functions.cpp"
    "${DLL_ROOT}/main/utils/utils.cpp"
    "${DLL_ROOT}/main/webpanel/overlay_canvas.cpp"
    "${DLL_ROOT}/main/webpanel/visual_draw.cpp"
    "${DLL_ROOT}/main/webpanel/web_server.cpp"
)

add_library( MecchaCheatV SHARED ${DLL_SOURCES} )

target_include_directories( MecchaCheatV PRIVATE
    "${DLL_ROOT}"
    "${DLL_ROOT}/libs"
    "${ROOT}/res"
)

target_link_libraries( MecchaCheatV PRIVATE
    minhook
    d3d11
    dxgi
    d3d12
    d3dcompiler
    dwmapi
    gdi32
    user32
    kernel32
    psapi
    ole32
    oleaut32
    uuid
    shell32
    winmm
    ws2_32
)

target_link_options( MecchaCheatV PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
)

set_target_properties( MecchaCheatV PROPERTIES
    OUTPUT_NAME "xv_meca"
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

find_package( Python3 REQUIRED COMPONENTS Interpreter )

set( INJECTOR_GEN_DIR "${CMAKE_BINARY_DIR}/injector/generated" )
set( INJECTOR_EMBEDDED_CPP "${INJECTOR_GEN_DIR}/embedded_payload.cpp" )
set( INJECTOR_EMBEDDED_HPP "${INJECTOR_GEN_DIR}/embedded_payload.hpp" )

add_custom_command(
    OUTPUT "${INJECTOR_EMBEDDED_CPP}" "${INJECTOR_EMBEDDED_HPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${INJECTOR_GEN_DIR}"
    COMMAND ${Python3_EXECUTABLE}
        "${ROOT}/scripts/embed_payload.py"
        --dll "$<TARGET_FILE:MecchaCheatV>"
        --out-cpp "${INJECTOR_EMBEDDED_CPP}"
        --out-hpp "${INJECTOR_EMBEDDED_HPP}"
    DEPENDS
        MecchaCheatV
        "${ROOT}/scripts/embed_payload.py"
    COMMENT "Embedding DLL into injector"
    VERBATIM
)

add_executable( MecchaCheatVLauncher
    "${INJECTOR_ROOT}/gui.cpp"
    "${INJECTOR_ROOT}/inject.cpp"
    "${INJECTOR_EMBEDDED_CPP}"
)

add_dependencies( MecchaCheatVLauncher MecchaCheatV )

target_include_directories( MecchaCheatVLauncher PRIVATE
    "${INJECTOR_GEN_DIR}"
    "${INJECTOR_ROOT}"
)

target_compile_definitions( MecchaCheatVLauncher PRIVATE
    UNICODE
    _UNICODE
)

target_link_libraries( MecchaCheatVLauncher PRIVATE
    kernel32
    psapi
    shell32
    gdi32
    user32
    gdiplus
    dwmapi
    ole32
    uuid
    version
)

target_link_options( MecchaCheatVLauncher PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
    -mwindows
    -municode
)

set_target_properties( MecchaCheatVLauncher PROPERTIES
    OUTPUT_NAME "xv_meca"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command( TARGET MecchaCheatVLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ROOT}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MecchaCheatVLauncher>" "${ROOT}/dist/xv_meca.exe"
    COMMENT "Copying xv_meca.exe to dist/"
)
