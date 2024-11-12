set(lib_name ${CMAKE_PROJECT_NAME})
########################################
set(root_srcs
  Makefile.am
  box2d.c
  bytebuffer.c
  bytebuffer.h
  g_box.c
  g_serialized.c
  g_util.c
  librttopo_geom_internal.h
  librttopo_internal.h
  measures.c
  measures.h
  measures3d.c
  measures3d.h
  ptarray.c
  rtalgorithm.c
  rtcircstring.c
  rtcollection.c
  rtcompound.c
  rtcurvepoly.c
  rtgeodetic.c
  rtgeodetic.h
  rtgeom.c
  rtgeom_api.c
  rtgeom_debug.c
  rtgeom_geos.c
  rtgeom_geos.h
  rtgeom_geos_clean.c
  rtgeom_geos_node.c
  rtgeom_geos_split.c
  rtgeom_log.h
  rtgeom_topo.c
  rthomogenize.c
  rtin_geojson.c
  rtin_twkb.c
  rtin_wkb.c
  rtiterator.c
  rtline.c
  rtlinearreferencing.c
  rtmcurve.c
  rtmline.c
  rtmpoint.c
  rtmpoly.c
  rtmsurface.c
  rtout_encoded_polyline.c
  rtout_geojson.c
  rtout_gml.c
  rtout_kml.c
  rtout_svg.c
  rtout_twkb.c
  rtout_twkb.h
  rtout_wkb.c
  rtout_wkt.c
  rtout_x3d.c
  rtpoint.c
  rtpoly.c
  rtprint.c
  rtpsurface.c
  rtspheroid.c
  rtstroke.c
  rtt_tpsnap.c
  rttin.c
  rttree.c
  rttree.h
  rttriangle.c
  rtutil.c
  stringbuffer.c
  stringbuffer.h
  varint.c
  varint.h
  )
source_group("" FILES ${root_srcs})
list(APPEND ${lib_name}_libsrcs ${root_srcs})
########################################
set(headers ${CMAKE_SOURCE_DIR}/headers)
set(SRID_MAX 999999)
set(SRID_USR_MAX 998999)
configure_file(${headers}/librttopo_geom.h.in ${CMAKE_BINARY_DIR}/cmake/librttopo_geom.h)
set(headers_srcs
  ${CMAKE_BINARY_DIR}/cmake/librttopo_geom.h
  ${headers}/librttopo.h
  )
source_group(headers FILES ${headers_srcs})
list(APPEND ${lib_name}_libsrcs ${headers_srcs})
########################################
add_library(${lib_name} STATIC ${${lib_name}_libsrcs})
target_include_directories(${lib_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/cmake> $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/headers> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${lib_name} PUBLIC xpro::geos_c)
set_target_properties(${lib_name} PROPERTIES
  PREFIX "" # strip off the "lib" prefix, since it's already libspatialite
  )
########################################
if(NOT DEFINED XP_INSTALL_CMAKEDIR)
  set(XP_INSTALL_CMAKEDIR ${CMAKE_INSTALL_DATADIR}/cmake)
endif()
if(NOT DEFINED XP_NAMESPACE)
  set(XP_NAMESPACE xpro)
endif()
set(targetsFile ${CMAKE_PROJECT_NAME}-targets)
xpPackageDevel(TARGETS_FILE ${targetsFile} DEPS geos LIBRARIES ${XP_NAMESPACE}::${lib_name})
install(TARGETS ${lib_name} EXPORT ${targetsFile}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
install(FILES ${headers_srcs} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ${targetsFile} DESTINATION ${XP_INSTALL_CMAKEDIR} NAMESPACE ${XP_NAMESPACE}::)
