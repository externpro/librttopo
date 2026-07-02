cmake_minimum_required(VERSION 4.3)
project(librttopo VERSION 1.1.0)
option(BUILD_AUTOMAKE "build with automake" OFF)
if(BUILD_AUTOMAKE)
  # ./docker-compose.sh
  # cmake -DBUILD_AUTOMAKE=ON --preset=Linux
  # cmake --build --preset=LinuxReleaseNoInstall
  # updates cmake/rttopo_config.h.in, which is used by configure.cmake
  include(ExternalProject)
  set_property(DIRECTORY PROPERTY "EP_BASE" ${CMAKE_BINARY_DIR}/epbase)
  ExternalProject_Add(${PROJECT_NAME}
    DOWNLOAD_COMMAND sudo dnf -y install --setopt=tsflags=nodocs autoconf automake libtool geos-devel
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}
    UPDATE_COMMAND <SOURCE_DIR>/autogen.sh
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> #--disable-shared
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/src/rttopo_config.h.in <SOURCE_DIR>/cmake/
    )
else()
  find_package(geos)
  include(cmake/configure.cmake)
  add_subdirectory(src)
endif()
