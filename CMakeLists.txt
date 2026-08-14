cmake_minimum_required(VERSION 3.20)

# Standalone Maho game project (generated from Build/Templates/GameProject).
# generateProject.py passes -DMAHO_ENGINE_ROOT=<engine repo>

project(Hiyajo-Project
	VERSION 0.1.0
	DESCRIPTION ""
	LANGUAGES CXX
)

if(NOT MAHO_ENGINE_ROOT)
	message(FATAL_ERROR "MAHO_ENGINE_ROOT is not set. Open/generate via the .cproject file.")
endif()

file(TO_CMAKE_PATH "${MAHO_ENGINE_ROOT}" MAHO_ENGINE_ROOT)
if(NOT EXISTS "${MAHO_ENGINE_ROOT}/Maho/CMakeLists.txt")
	message(FATAL_ERROR "Invalid MAHO_ENGINE_ROOT: ${MAHO_ENGINE_ROOT}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Hiyajo-Project)

list(APPEND CMAKE_MODULE_PATH "${MAHO_ENGINE_ROOT}/Build/CMake")
include(MahoDirectories)
include(MahoHelpers)

maho_ensure_workspace_dirs()
maho_warn_if_not_intermediate_binary_dir()

option(MAHO_BUILD_SHARED "Build Maho engine as a shared library (DLL)" ON)
# Push Maho::FEditorLayer from the game App (Editor sources live in the game project under Source/Editor/).
option(GAME_WITH_EDITOR "Enable Maho::FEditorLayer overlay (disable for shipping)" ON)

# Enabled plugins come from the sibling .cproject (or -DMAHO_CPROJECT=...).
if(NOT MAHO_CPROJECT)
	set(_MAHO_CPROJECT_CANDIDATE "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.cproject")
	if(EXISTS "${_MAHO_CPROJECT_CANDIDATE}")
		set(MAHO_CPROJECT "${_MAHO_CPROJECT_CANDIDATE}")
	endif()
	unset(_MAHO_CPROJECT_CANDIDATE)
endif()
if(MAHO_CPROJECT)
	file(TO_CMAKE_PATH "${MAHO_CPROJECT}" MAHO_CPROJECT)
	set(MAHO_CPROJECT "${MAHO_CPROJECT}" CACHE FILEPATH "Game .cproject (enabled Plugins list)" FORCE)
	message(STATUS "Maho: using .cproject for plugins: ${MAHO_CPROJECT}")
endif()

add_subdirectory("${MAHO_ENGINE_ROOT}/Maho" "${CMAKE_BINARY_DIR}/MahoBuild")

set(GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(GAME_SOURCE_DIR "${GAME_ROOT}/Source")
set(GAME_SHADERS_DIR "${GAME_ROOT}/Shaders")
set(GAME_PLUGINS_DIR "${GAME_ROOT}/Plugins")
set(GAME_CONFIG_DIR "${GAME_ROOT}/Config")
set(GAME_SCRIPTS_DIR "${GAME_ROOT}/Scripts")
set(GAME_CONTENT_DIR "${GAME_ROOT}/Content")
set(GAME_GENERATED_DIR "${GAME_ROOT}/Intermediate/Generated")

maho_collect_sources(GAME_SOURCES "${GAME_SOURCE_DIR}")
if(GAME_SOURCES STREQUAL "")
	message(FATAL_ERROR "Hiyajo-Project: no sources under Source/")
endif()

add_executable(Hiyajo-Project ${GAME_SOURCES})
target_include_directories(Hiyajo-Project PRIVATE
	"${GAME_SOURCE_DIR}"
	"${GAME_SOURCE_DIR}/Game"
	"${GAME_GENERATED_DIR}"
	"${MAHO_ENGINE_ROOT}/Maho/Source/Private"
	"${MAHO_NLOHMANN_JSON_INCLUDE_DIR}"
	"${MAHO_SOL2_INCLUDE_DIR}"
)
target_include_directories(Hiyajo-Project PRIVATE
	$<TARGET_PROPERTY:assimp,SOURCE_DIR>/include
	$<TARGET_PROPERTY:assimp,BINARY_DIR>/include
)
target_link_libraries(Hiyajo-Project PRIVATE Maho::Engine Maho::Modules
	lua
	assimp)
if(TARGET zlibstatic)
	target_link_libraries(Hiyajo-Project PRIVATE zlibstatic)
endif()

# Copy third-party shared DLLs next to the EXE (assimp is built as a DLL).
if(TARGET assimp)
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:assimp>"
			"$<TARGET_FILE_DIR:Hiyajo-Project>"
		COMMENT "Copy assimp DLL"
		VERBATIM
	)
endif()
# GLFW is built as a DLL when Maho is shared; Platform/Render plugins + Maho.dll share it.
if(TARGET glfw)
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:glfw>"
			"$<TARGET_FILE_DIR:Hiyajo-Project>"
		COMMENT "Copy glfw DLL"
		VERBATIM
	)
endif()
target_compile_features(Hiyajo-Project PRIVATE cxx_std_20)
target_compile_definitions(Hiyajo-Project PRIVATE
	$<$<BOOL:${GAME_WITH_EDITOR}>:GAME_WITH_EDITOR=1>
	MAHO_WITH_ASSIMP=1
)

if(MSVC)
	target_compile_options(Hiyajo-Project PRIVATE /W4 /permissive- /Zc:preprocessor /utf-8)
else()
	target_compile_options(Hiyajo-Project PRIVATE -Wall -Wextra -Wpedantic)
endif()

maho_set_output_dirs(Hiyajo-Project)
maho_source_group_mirrors("${GAME_SOURCE_DIR}" ${GAME_SOURCES})
maho_add_lua_scripts(Hiyajo-Project "${GAME_SCRIPTS_DIR}")
set_target_properties(Hiyajo-Project PROPERTIES
	FOLDER "Games"
	VS_DEBUGGER_WORKING_DIRECTORY "${GAME_ROOT}"
	WIN32_EXECUTABLE $<BOOL:${WIN32}>
)
add_dependencies(Hiyajo-Project Maho MahoModules)

maho_copy_shaders(Hiyajo-Project "${GAME_SHADERS_DIR}" "Shaders")
if(IS_DIRECTORY "${GAME_CONFIG_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Config"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${GAME_CONFIG_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Config"
		COMMENT "Copy project Config (DefaultEngine.ini)"
		VERBATIM
	)
endif()
if(IS_DIRECTORY "${GAME_CONTENT_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Content"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${GAME_CONTENT_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Content"
		COMMENT "Copy project Content (casset, etc.)"
		VERBATIM
	)
endif()
if(IS_DIRECTORY "${GAME_SCRIPTS_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Scripts"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${GAME_SCRIPTS_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Scripts"
		COMMENT "Copy project Scripts (Lua)"
		VERBATIM
	)
endif()
add_custom_command(TARGET Hiyajo-Project POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Project/Plugins"
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Engine/Plugins"
	COMMENT "Ensure plugin runtime folders exist"
	VERBATIM
)
if(IS_DIRECTORY "${MAHO_ENGINE_PLUGINS_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${MAHO_ENGINE_PLUGINS_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Engine/Plugins"
		COMMENT "Copy engine plugins"
		VERBATIM
	)
endif()
if(DEFINED MAHO_ENGINE_FONTS_DIR AND IS_DIRECTORY "${MAHO_ENGINE_FONTS_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Hiyajo-Project>/Engine/Fonts"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${MAHO_ENGINE_FONTS_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Engine/Fonts"
		COMMENT "Copy engine Fonts"
		VERBATIM
	)
endif()
if(IS_DIRECTORY "${GAME_PLUGINS_DIR}")
	add_custom_command(TARGET Hiyajo-Project POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${GAME_PLUGINS_DIR}"
			"$<TARGET_FILE_DIR:Hiyajo-Project>/Project/Plugins"
		COMMENT "Copy project plugins"
		VERBATIM
	)
endif()

maho_install_runtime_target(Hiyajo-Project)
maho_install_directory("${GAME_SHADERS_DIR}" "Shaders")
maho_install_directory("${GAME_PLUGINS_DIR}" "Project/Plugins")
maho_install_directory("${GAME_CONFIG_DIR}" "Config")
maho_install_directory("${GAME_SCRIPTS_DIR}" "Scripts")
maho_install_directory("${GAME_CONTENT_DIR}" "Content")

# Packaging is done via project package.bat → Tools/package_ui.py (cmake --install).
