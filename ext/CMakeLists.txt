include(CheckCXXCompilerFlag)

# prevent glfw from changing the directory on macOS bundles
SET(GLFW_USE_CHDIR OFF CACHE BOOL "Prevent glfwInit from chdir to Contents/Resources" FORCE)

#============================================================================
# Check if we have all the submodules checked out
#============================================================================
if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glfw")
    message(FATAL_ERROR "The HDRView dependency repositories (NanoGUI, GLFW, etc.) are missing! "
        "You probably did not clone the project with --recursive. It is possible to recover "
        "by calling \"git submodule update --init --recursive\"")
endif()

#============================================================================
# Set a default build configuration (Release)
#============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                 "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

#============================================================================
# Enable folders for projects in Visual Studio
#============================================================================
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  
  if("cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    message(STATUS "Using C++17 standard")
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
  elseif("cxx_std_14" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    message(STATUS "Using C++14 standard")
    set(CMAKE_CXX_STANDARD 14 CACHE STRING "The C++ standard to use")
  elseif("cxx_std_11" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    message(STATUS "Using C++11 standard")
    set(CMAKE_CXX_STANDARD 11 CACHE STRING "The C++ standard to use")
  else()
    message(FATAL_ERROR "Unsupported compiler -- HDRView requires at least C++11!")
  endif()
  
  
  # Prefer libc++ in conjunction with Clang
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" HAS_LIBCPP)
    if (HAS_LIBCPP)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
      message(STATUS "HDRView: using libc++.")
    else()
      message(WARNING "libc++ is recommended in conjunction with clang. Please insteall the libc++ development headers, provided e.g. by the packages 'libc++-dev' and 'libc++abi-dev' on Debian/Ubuntu.")
    endif()
  endif()

  # Enable link time optimization and set the default symbol
  # visibility to hidden (very important to obtain small binaries)
  if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
    # Default symbol visibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    # set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  endif()

  # Disable specific GCC 7 warnings
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-misleading-indentation -Wformat-truncation=0 -Wno-int-in-bool-context -Wimplicit-fallthrough=0")
  endif()
endif()

#============================================================================
# Sanitize build environment for static build with C++11
#============================================================================
if (MSVC)
  # Disable annoying secure CRT warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  endif()

  # Static build
  set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif()


#============================================================================
# Build ZLIB on Windows (needed for OpenEXR)
#============================================================================
if (WIN32)
  # Build zlib (only on Windows)
  set(ZLIB_BUILD_STATIC_LIBS ON CACHE BOOL " " FORCE)
  set(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
  add_subdirectory(zlib)


  set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib;${CMAKE_CURRENT_BINARY_DIR}/zlib" CACHE PATH " " FORCE)
  set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib/Release/zlibstatic.lib" CACHE FILEPATH " " FORCE)

  # set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" CACHE PATH " " FORCE)
  # if (CMAKE_GENERATOR STREQUAL "Ninja")
  #   set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib/zlibstatic.lib" CACHE FILEPATH " " FORCE)
  # else()
  #   set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib/$<CONFIG>/zlibstatic.lib" CACHE FILEPATH " " FORCE)
  # endif()

  set_property(TARGET zlibstatic PROPERTY FOLDER "dependencies")
  include_directories(${ZLIB_INCLUDE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/zlib")
endif()


#============================================================================
# Build OpenEXR
#============================================================================
SET (CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set (ILMBASE_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
set (OPENEXR_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
set (OPENEXR_BUILD_SHARED_LIBS    OFF CACHE BOOL " " FORCE)
set (ILMBASE_BUILD_SHARED_LIBS    OFF CACHE BOOL " " FORCE)
set (PYILMBASE_ENABLE             OFF CACHE BOOL " " FORCE)
set (OPENEXR_BUILD_UTILS          OFF CACHE BOOL " " FORCE)
set (INSTALL_OPENEXR_EXAMPLES     OFF CACHE BOOL " " FORCE)
set (BUILD_TESTING                OFF CACHE BOOL " " FORCE)

add_subdirectory (openexr)

set_property (TARGET IexMath IlmThread Half
              Iex Imath IlmImf IlmImfUtil
              PROPERTY FOLDER "ext/OpenEXR")


if (WIN32)
  add_dependencies(IlmImf zlibstatic)
endif()


#============================================================================
# Build NanoGUI
#============================================================================
# Use OpenGL backend for UI. In the future, this can be removed to use Metal on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION=1")
endif()
# set(NANOGUI_BACKEND "OpenGL" CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " ")
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
add_subdirectory(nanogui)
set_property(TARGET glfw glfw_objects nanogui PROPERTY FOLDER "dependencies")


#============================================================================
# Build docopt.cpp
#============================================================================
add_subdirectory(docopt EXCLUDE_FROM_ALL)
set_property(TARGET docopt PROPERTY FOLDER "dependencies")


#============================================================================
# Build PlatformFolders
#============================================================================
add_subdirectory(PlatformFolders)

#============================================================================
# Define variables for the include directories for the various dependencies
# and export them to the parent scope
#============================================================================
set (OPENEXR_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Imath
  ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Half
  ${CMAKE_CURRENT_SOURCE_DIR}/openexr/IlmBase/Iex
  ${CMAKE_CURRENT_SOURCE_DIR}/openexr/OpenEXR/IlmImf
  ${CMAKE_CURRENT_BINARY_DIR}/openexr/IlmBase/config
  ${CMAKE_CURRENT_BINARY_DIR}/openexr/OpenEXR/config
)

if (WIN32)
  set (OPENEXR_LIBS OpenEXR::IlmImf IlmBase::Imath IlmBase::Half zlibstatic)
else ()
  set (OPENEXR_LIBS OpenEXR::IlmImf IlmBase::Imath IlmBase::Half)
endif ()

set(GLFW_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glfw/include)
set(GLEW_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glew/include)
set(NANOVG_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/nanovg/src)
set(NANOGUI_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/include)
set(TINYDIR_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/tinydir)
set(DOCOPT_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/docopt)
set(TINYDNG_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/tinydngloader)
set(STB_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/stb)
set(ALPHANUM_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/alphanum)
set(SPDLOG_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)
set(JSON_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/json)
set(PLATFORMFOLDERS_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/PlatformFolders)



#============================================================================
# Compile remainder of the codebase with compiler warnings turned on
#============================================================================
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types -Wno-deprecated-register")
  endif()
endif()

set(CompilerFlags
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS
      CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELWITHDEBINFO COMPILE_DEFINITIONS U_CMAKE_BUILD_TYPE
      CMAKE_MACOSX_RPATH
      OPENEXR_INCLUDE_DIRS OPENEXR_LIBS
      GLFW_INCLUDE_DIR GLEW_INCLUDE_DIR
      NANOVG_INCLUDE_DIR NANOGUI_EXTRA_INCS NANOGUI_EXTRA_DEFS
	    NANOGUI_EXTRA_LIBS NANOGUI_INCLUDE_DIR NANOGUI_BACKEND
      DOCOPT_INCLUDE_DIR
      TINYDNG_INCLUDE_DIR
      TINYDIR_INCLUDE_DIR
      STB_INCLUDE_DIR
      ALPHANUM_INCLUDE_DIR
      SPDLOG_INCLUDE_DIR
      JSON_INCLUDE_DIR
      PLATFORMFOLDERS_INCLUDE_DIR
)
foreach(CompilerFlag ${CompilerFlags})
  set(${CompilerFlag} "${${CompilerFlag}}" PARENT_SCOPE)
endforeach()

set(FILESYSTEM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/filesystem PARENT_SCOPE)
