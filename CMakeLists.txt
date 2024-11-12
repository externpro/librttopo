cmake_minimum_required(VERSION 3.28)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.devcontainer/cmake ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(preproject)
project(librttopo VERSION 1.1.0.1)
include(flags)
include(GNUInstallDirs)
xpFindPkg(PKGS geos)
if(TRUE)
  include(configure)
  add_subdirectory(src)
else()
  include(ExternalProject)
  set_property(DIRECTORY PROPERTY "EP_BASE" ${CMAKE_BINARY_DIR}/epbase)
  ExternalProject_Add(${PROJECT_NAME}
    DOWNLOAD_COMMAND sudo dnf -y install --setopt=tsflags=nodocs autoconf automake libtool
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}
    UPDATE_COMMAND <SOURCE_DIR>/autogen.sh
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> #--disable-shared
    )
endif()
