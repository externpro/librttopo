/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2015 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Topology extension for librtgeom.
 * Initially funded by Tuscany Region (Italy) - SITA (CIG: 60351023B8)
 *
 **********************************************************************/


#include "rttopo_config.h"

/*#define RTGEOM_DEBUG_LEVEL 1*/
#include "rtgeom_log.h"

#include "librtgeom_internal.h"
#include "librtgeom_topo_internal.h"
#include "rtgeom_geos.h"

#include <stdio.h>
#include <inttypes.h> /* for PRId64 */
#include <errno.h>
#include <math.h>

#ifdef WIN32
# define RTTFMT_ELEMID "lld"
#else
# define RTTFMT_ELEMID PRId64
#endif

/* TODO: move this to rtgeom_log.h */
#define RTDEBUGG(level, geom, msg) \
  if (RTGEOM_DEBUG_LEVEL >= level) \
  do { \
    size_t sz; \
    char *wkt1 = rtgeom_to_wkt(geom, RTWKT_EXTENDED, 15, &sz); \
    /* char *wkt1 = rtgeom_to_hexwkb(geom, RTWKT_EXTENDED, &sz); */ \
    RTDEBUGF(level, msg ": %s", wkt1); \
    rtfree(wkt1); \
  } while (0);


/*********************************************************************
 *
 * Backend iface
 *
 ********************************************************************/

RTT_BE_IFACE* rtt_CreateBackendIface(const RTT_BE_DATA *data)
{
  RTT_BE_IFACE *iface = rtalloc(sizeof(RTT_BE_IFACE));
  iface->data = data;
  iface->cb = NULL;
  return iface;
}

void rtt_BackendIfaceRegisterCallbacks(RTT_BE_IFACE *iface,
                                       const RTT_BE_CALLBACKS* cb)
{
  iface->cb = cb;
}

void rtt_FreeBackendIface(RTT_BE_IFACE* iface)
{
  rtfree(iface);
}

/*********************************************************************
 *
 * Backend wrappers
 *
 ********************************************************************/

#define CHECKCB(be, method) do { \
  if ( ! (be)->cb || ! (be)->cb->method ) \
  rterror("Callback " # method " not registered by backend"); \
} while (0)

#define CB0(be, method) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data)

#define CB1(be, method, a1) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data, a1)

#define CBT0(to, method) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo)

#define CBT1(to, method, a1) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1)

#define CBT2(to, method, a1, a2) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2)

#define CBT3(to, method, a1, a2, a3) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3)

#define CBT4(to, method, a1, a2, a3, a4) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4)

#define CBT5(to, method, a1, a2, a3, a4, a5) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5)

#define CBT6(to, method, a1, a2, a3, a4, a5, a6) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5, a6)

const char *
rtt_be_lastErrorMessage(const RTT_BE_IFACE* be)
{
  CB0(be, lastErrorMessage);
}

RTT_BE_TOPOLOGY *
rtt_be_loadTopologyByName(RTT_BE_IFACE *be, const char *name)
{
  CB1(be, loadTopologyByName, name);
}

static int
rtt_be_topoGetSRID(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetSRID);
}

static double
rtt_be_topoGetPrecision(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetPrecision);
}

static int
rtt_be_topoHasZ(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoHasZ);
}

int
rtt_be_freeTopology(RTT_TOPOLOGY *topo)
{
  CBT0(topo, freeTopology);
}

RTT_ISO_NODE*
rtt_be_getNodeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getNodeById, ids, numelems, fields);
}

RTT_ISO_NODE*
rtt_be_getNodeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getNodeWithinDistance2D, pt, dist, numelems, fields, limit);
}

static RTT_ISO_NODE*
rtt_be_getNodeWithinBox2D( const RTT_TOPOLOGY* topo,
                           const GBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getNodeWithinBox2D, box, numelems, fields, limit);
}

static RTT_ISO_EDGE*
rtt_be_getEdgeWithinBox2D( const RTT_TOPOLOGY* topo,
                           const GBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getEdgeWithinBox2D, box, numelems, fields, limit);
}

static RTT_ISO_FACE*
rtt_be_getFaceWithinBox2D( const RTT_TOPOLOGY* topo,
                           const GBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getFaceWithinBox2D, box, numelems, fields, limit);
}

int
rtt_be_insertNodes(RTT_TOPOLOGY* topo, RTT_ISO_NODE* node, int numelems)
{
  CBT2(topo, insertNodes, node, numelems);
}

static int
rtt_be_insertFaces(RTT_TOPOLOGY* topo, RTT_ISO_FACE* face, int numelems)
{
  CBT2(topo, insertFaces, face, numelems);
}

static int
rtt_be_deleteFacesById(const RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int numelems)
{
  CBT2(topo, deleteFacesById, ids, numelems);
}

static int
rtt_be_deleteNodesById(const RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int numelems)
{
  CBT2(topo, deleteNodesById, ids, numelems);
}

RTT_ELEMID
rtt_be_getNextEdgeId(RTT_TOPOLOGY* topo)
{
  CBT0(topo, getNextEdgeId);
}

RTT_ISO_EDGE*
rtt_be_getEdgeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeById, ids, numelems, fields);
}

static RTT_ISO_FACE*
rtt_be_getFaceById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getFaceById, ids, numelems, fields);
}

static RTT_ISO_EDGE*
rtt_be_getEdgeByNode(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeByNode, ids, numelems, fields);
}

static RTT_ISO_EDGE*
rtt_be_getEdgeByFace(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields, const GBOX *box)
{
  CBT4(topo, getEdgeByFace, ids, numelems, fields, box);
}

static RTT_ISO_NODE*
rtt_be_getNodeByFace(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields, const GBOX *box)
{
  CBT4(topo, getNodeByFace, ids, numelems, fields, box);
}

RTT_ISO_EDGE*
rtt_be_getEdgeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getEdgeWithinDistance2D, pt, dist, numelems, fields, limit);
}

int
rtt_be_insertEdges(RTT_TOPOLOGY* topo, RTT_ISO_EDGE* edge, int numelems)
{
  CBT2(topo, insertEdges, edge, numelems);
}

int
rtt_be_updateEdges(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* sel_edge, int sel_fields,
  const RTT_ISO_EDGE* upd_edge, int upd_fields,
  const RTT_ISO_EDGE* exc_edge, int exc_fields
)
{
  CBT6(topo, updateEdges, sel_edge, sel_fields,
                          upd_edge, upd_fields,
                          exc_edge, exc_fields);
}

static int
rtt_be_updateNodes(RTT_TOPOLOGY* topo,
  const RTT_ISO_NODE* sel_node, int sel_fields,
  const RTT_ISO_NODE* upd_node, int upd_fields,
  const RTT_ISO_NODE* exc_node, int exc_fields
)
{
  CBT6(topo, updateNodes, sel_node, sel_fields,
                          upd_node, upd_fields,
                          exc_node, exc_fields);
}

static int
rtt_be_updateFacesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_FACE* faces, int numfaces
)
{
  CBT2(topo, updateFacesById, faces, numfaces);
}

static int
rtt_be_updateEdgesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* edges, int numedges, int upd_fields
)
{
  CBT3(topo, updateEdgesById, edges, numedges, upd_fields);
}

static int
rtt_be_updateNodesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_NODE* nodes, int numnodes, int upd_fields
)
{
  CBT3(topo, updateNodesById, nodes, numnodes, upd_fields);
}

int
rtt_be_deleteEdges(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* sel_edge, int sel_fields
)
{
  CBT2(topo, deleteEdges, sel_edge, sel_fields);
}

RTT_ELEMID
rtt_be_getFaceContainingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  CBT1(topo, getFaceContainingPoint, pt);
}


int
rtt_be_updateTopoGeomEdgeSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_edge, RTT_ELEMID new_edge1, RTT_ELEMID new_edge2)
{
  CBT3(topo, updateTopoGeomEdgeSplit, split_edge, new_edge1, new_edge2);
}

static int
rtt_be_updateTopoGeomFaceSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_face,
                               RTT_ELEMID new_face1, RTT_ELEMID new_face2)
{
  CBT3(topo, updateTopoGeomFaceSplit, split_face, new_face1, new_face2);
}

static int
rtt_be_checkTopoGeomRemEdge(RTT_TOPOLOGY* topo, RTT_ELEMID edge_id,
                            RTT_ELEMID face_left, RTT_ELEMID face_right)
{
  CBT3(topo, checkTopoGeomRemEdge, edge_id, face_left, face_right);
}

static int
rtt_be_checkTopoGeomRemNode(RTT_TOPOLOGY* topo, RTT_ELEMID node_id,
                            RTT_ELEMID eid1, RTT_ELEMID eid2)
{
  CBT3(topo, checkTopoGeomRemNode, node_id, eid1, eid2);
}

static int
rtt_be_updateTopoGeomFaceHeal(RTT_TOPOLOGY* topo,
                             RTT_ELEMID face1, RTT_ELEMID face2,
                             RTT_ELEMID newface)
{
  CBT3(topo, updateTopoGeomFaceHeal, face1, face2, newface);
}

static int
rtt_be_updateTopoGeomEdgeHeal(RTT_TOPOLOGY* topo,
                             RTT_ELEMID edge1, RTT_ELEMID edge2,
                             RTT_ELEMID newedge)
{
  CBT3(topo, updateTopoGeomEdgeHeal, edge1, edge2, newedge);
}

static RTT_ELEMID*
rtt_be_getRingEdges( RTT_TOPOLOGY* topo,
                     RTT_ELEMID edge, int *numedges, int limit )
{
  CBT3(topo, getRingEdges, edge, numedges, limit);
}


/* wrappers of backend wrappers... */

int
rtt_be_ExistsCoincidentNode(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  int exists = 0;
  rtt_be_getNodeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

int
rtt_be_ExistsEdgeIntersectingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  int exists = 0;
  rtt_be_getEdgeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

/************************************************************************
 *
 * Utility functions
 *
 ************************************************************************/

static void
_rtt_release_faces(RTT_ISO_FACE *faces, int num_faces)
{
  int i;
  for ( i=0; i<num_faces; ++i ) {
    if ( faces[i].mbr ) rtfree(faces[i].mbr);
  }
  rtfree(faces);
}

static void
_rtt_release_edges(RTT_ISO_EDGE *edges, int num_edges)
{
  int i;
  for ( i=0; i<num_edges; ++i ) {
    if ( edges[i].geom ) rtline_free(edges[i].geom);
  }
  rtfree(edges);
}

static void
_rtt_release_nodes(RTT_ISO_NODE *nodes, int num_nodes)
{
  int i;
  for ( i=0; i<num_nodes; ++i ) {
    if ( nodes[i].geom ) rtpoint_free(nodes[i].geom);
  }
  rtfree(nodes);
}

/************************************************************************
 *
 * API implementation
 *
 ************************************************************************/

RTT_TOPOLOGY *
rtt_LoadTopology( RTT_BE_IFACE *iface, const char *name )
{
  RTT_BE_TOPOLOGY* be_topo;
  RTT_TOPOLOGY* topo;

  be_topo = rtt_be_loadTopologyByName(iface, name);
  if ( ! be_topo ) {
    //rterror("Could not load topology from backend: %s",
    rterror("%s", rtt_be_lastErrorMessage(iface));
    return NULL;
  }
  topo = rtalloc(sizeof(RTT_TOPOLOGY));
  topo->be_iface = iface;
  topo->be_topo = be_topo;
  topo->srid = rtt_be_topoGetSRID(topo);
  topo->hasZ = rtt_be_topoHasZ(topo);
  topo->precision = rtt_be_topoGetPrecision(topo);

  return topo;
}

void
rtt_FreeTopology( RTT_TOPOLOGY* topo )
{
  if ( ! rtt_be_freeTopology(topo) ) {
    rtnotice("Could not release backend topology memory: %s",
            rtt_be_lastErrorMessage(topo->be_iface));
  }
  rtfree(topo);
}

RTT_ELEMID
rtt_AddIsoNode( RTT_TOPOLOGY* topo, RTT_ELEMID face,
                RTPOINT* pt, int skipISOChecks )
{
  RTT_ELEMID foundInFace = -1;

  if ( ! skipISOChecks )
  {
    if ( rtt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      rterror("SQL/MM Spatial exception - coincident node");
      return -1;
    }
    if ( rtt_be_ExistsEdgeIntersectingPoint(topo, pt) ) /*x*/
    {
      rterror("SQL/MM Spatial exception - edge crosses node.");
      return -1;
    }
  }

  if ( face == -1 || ! skipISOChecks )
  {
    foundInFace = rtt_be_getFaceContainingPoint(topo, pt); /*x*/
    if ( foundInFace == -2 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( foundInFace == -1 ) foundInFace = 0;
  }

  if ( face == -1 ) {
    face = foundInFace;
  }
  else if ( ! skipISOChecks && foundInFace != face ) {
#if 0
    rterror("SQL/MM Spatial exception - within face %d (not %d)",
            foundInFace, face);
#else
    rterror("SQL/MM Spatial exception - not within face");
#endif
    return -1;
  }

  RTT_ISO_NODE node;
  node.node_id = -1;
  node.containing_face = face;
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return node.node_id;
}

/* Check that an edge does not cross an existing node or edge
 *
 * @param myself the id of an edge to skip, if any
 *               (for ChangeEdgeGeom). Can use 0 for none.
 *
 * Return -1 on cross or error, 0 if everything is fine.
 * Note that before returning -1, rterror is invoked...
 */
static int
_rtt_CheckEdgeCrossing( RTT_TOPOLOGY* topo,
                        RTT_ELEMID start_node, RTT_ELEMID end_node,
                        const RTLINE *geom, RTT_ELEMID myself )
{
  int i, num_nodes, num_edges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_NODE *nodes;
  const GBOX *edgebox;
  GEOSGeometry *edgegg;
  const GEOSPreparedGeometry* prepared_edge;

  initGEOS(rtnotice, rtgeom_geos_error);

  edgegg = RTGEOM2GEOS( rtline_as_rtgeom(geom), 0);
  if ( ! edgegg ) {
    rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
    return -1;
  }
  prepared_edge = GEOSPrepare( edgegg );
  if ( ! prepared_edge ) {
    rterror("Could not prepare edge geometry: %s", rtgeom_geos_errmsg);
    return -1;
  }
  edgebox = rtgeom_get_bbox( rtline_as_rtgeom(geom) );

  /* loop over each node within the edge's gbox */
  nodes = rtt_be_getNodeWithinBox2D( topo, edgebox, &num_nodes,
                                            RTT_COL_NODE_ALL, 0 );
  RTDEBUGF(1, "rtt_be_getNodeWithinBox2D returned %d nodes", num_nodes);
  if ( num_nodes == -1 ) {
    GEOSPreparedGeom_destroy(prepared_edge);
    GEOSGeom_destroy(edgegg);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    RTT_ISO_NODE* node = &(nodes[i]);
    GEOSGeometry *nodegg;
    int contains;
    if ( node->node_id == start_node ) continue;
    if ( node->node_id == end_node ) continue;
    /* check if the edge contains this node (not on boundary) */
    nodegg = RTGEOM2GEOS( rtpoint_as_rtgeom(node->geom) , 0);
    /* ST_RelateMatch(rec.relate, 'T********') */
    contains = GEOSPreparedContains( prepared_edge, nodegg );
    GEOSGeom_destroy(nodegg);
    if (contains == 2)
    {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      _rtt_release_nodes(nodes, num_nodes);
      rterror("GEOS exception on PreparedContains: %s", rtgeom_geos_errmsg);
      return -1;
    }
    if ( contains )
    {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      _rtt_release_nodes(nodes, num_nodes);
      rterror("SQL/MM Spatial exception - geometry crosses a node");
      return -1;
    }
  }
  if ( nodes ) _rtt_release_nodes(nodes, num_nodes);
               /* may be NULL if num_nodes == 0 */

  /* loop over each edge within the edge's gbox */
  edges = rtt_be_getEdgeWithinBox2D( topo, edgebox, &num_edges, RTT_COL_EDGE_ALL, 0 );
  RTDEBUGF(1, "rtt_be_getEdgeWithinBox2D returned %d edges", num_edges);
  if ( num_edges == -1 ) {
    GEOSPreparedGeom_destroy(prepared_edge);
    GEOSGeom_destroy(edgegg);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_edges; ++i )
  {
    RTT_ISO_EDGE* edge = &(edges[i]);
    RTT_ELEMID edge_id = edge->edge_id;
    GEOSGeometry *eegg;
    char *relate;
    int match;

    if ( edge_id == myself ) continue;

    if ( ! edge->geom ) {
      _rtt_release_edges(edges, num_edges);
      rterror("Edge %d has NULL geometry!", edge_id);
      return -1;
    }

    eegg = RTGEOM2GEOS( rtline_as_rtgeom(edge->geom), 0 );
    if ( ! eegg ) {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      _rtt_release_edges(edges, num_edges);
      rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
      return -1;
    }

    RTDEBUGF(2, "Edge %d converted to GEOS", edge_id);

    /* check if the edge crosses our edge (not boundary-boundary) */

    relate = GEOSRelateBoundaryNodeRule(eegg, edgegg, 2);
    if ( ! relate ) {
      GEOSGeom_destroy(eegg);
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      _rtt_release_edges(edges, num_edges);
      rterror("GEOSRelateBoundaryNodeRule error: %s", rtgeom_geos_errmsg);
      return -1;
    }

    RTDEBUGF(2, "Edge %d relate pattern is %s", edge_id, relate);

    match = GEOSRelatePatternMatch(relate, "F********");
    if ( match ) {
      /* error or no interior intersection */
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        _rtt_release_edges(edges, num_edges);
        GEOSPreparedGeom_destroy(prepared_edge);
        GEOSGeom_destroy(edgegg);
        rterror("GEOSRelatePatternMatch error: %s", rtgeom_geos_errmsg);
        return -1;
      }
      else continue; /* no interior intersection */
    }

    match = GEOSRelatePatternMatch(relate, "1FFF*FFF2");
    if ( match ) {
      _rtt_release_edges(edges, num_edges);
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        rterror("GEOSRelatePatternMatch error: %s", rtgeom_geos_errmsg);
      } else {
        rterror("SQL/MM Spatial exception - coincident edge %" RTTFMT_ELEMID,
                edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "1********");
    if ( match ) {
      _rtt_release_edges(edges, num_edges);
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        rterror("GEOSRelatePatternMatch error: %s", rtgeom_geos_errmsg);
      } else {
        rterror("Spatial exception - geometry intersects edge %"
                RTTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "T********");
    if ( match ) {
      _rtt_release_edges(edges, num_edges);
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        rterror("GEOSRelatePatternMatch error: %s", rtgeom_geos_errmsg);
      } else {
        rterror("SQL/MM Spatial exception - geometry crosses edge %"
                RTTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    RTDEBUGF(2, "Edge %d analisys completed, it does no harm", edge_id);

    GEOSFree(relate);
    GEOSGeom_destroy(eegg);
  }
  if ( edges ) _rtt_release_edges(edges, num_edges);
              /* would be NULL if num_edges was 0 */

  GEOSPreparedGeom_destroy(prepared_edge);
  GEOSGeom_destroy(edgegg);

  return 0;
}


RTT_ELEMID
rtt_AddIsoEdge( RTT_TOPOLOGY* topo, RTT_ELEMID startNode,
                RTT_ELEMID endNode, const RTLINE* geom )
{
  int num_nodes;
  int i;
  RTT_ISO_EDGE newedge;
  RTT_ISO_NODE *endpoints;
  RTT_ELEMID containing_face = -1;
  RTT_ELEMID node_ids[2];
  RTT_ISO_NODE updated_nodes[2];
  int skipISOChecks = 0;
  POINT2D p1, p2;

  /* NOT IN THE SPECS:
   * A closed edge is never isolated (as it forms a face)
   */
  if ( startNode == endNode )
  {
    rterror("Closed edges would not be isolated, try rtt_AddEdgeNewFaces");
    return -1;
  }

  if ( ! skipISOChecks )
  {
    /* Acurve must be simple */
    if ( ! rtgeom_is_simple(rtline_as_rtgeom(geom)) )
    {
      rterror("SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  /*
   * Check for:
   *    existence of nodes
   *    nodes faces match
   * Extract:
   *    nodes face id
   *    nodes geoms
   */
  num_nodes = 2;
  node_ids[0] = startNode;
  node_ids[1] = endNode;
  endpoints = rtt_be_getNodeById( topo, node_ids, &num_nodes,
                                             RTT_COL_NODE_ALL );
  if ( num_nodes < 0 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  else if ( num_nodes < 2 )
  {
    if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);
    rterror("SQL/MM Spatial exception - non-existent node");
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    const RTT_ISO_NODE *n = &(endpoints[i]);
    if ( n->containing_face == -1 )
    {
      _rtt_release_nodes(endpoints, num_nodes);
      rterror("SQL/MM Spatial exception - not isolated node");
      return -1;
    }
    if ( containing_face == -1 ) containing_face = n->containing_face;
    else if ( containing_face != n->containing_face )
    {
      _rtt_release_nodes(endpoints, num_nodes);
      rterror("SQL/MM Spatial exception - nodes in different faces");
      return -1;
    }

    if ( ! skipISOChecks )
    {
      if ( n->node_id == startNode )
      {
        /* l) Check that start point of acurve match start node geoms. */
        getPoint2d_p(geom->points, 0, &p1);
        getPoint2d_p(n->geom->point, 0, &p2);
        if ( ! p2d_same(&p1, &p2) )
        {
          _rtt_release_nodes(endpoints, num_nodes);
          rterror("SQL/MM Spatial exception - "
                  "start node not geometry start point.");
          return -1;
        }
      }
      else
      {
        /* m) Check that end point of acurve match end node geoms. */
        getPoint2d_p(geom->points, geom->points->npoints-1, &p1);
        getPoint2d_p(n->geom->point, 0, &p2);
        if ( ! p2d_same(&p1, &p2) )
        {
          _rtt_release_nodes(endpoints, num_nodes);
          rterror("SQL/MM Spatial exception - "
                  "end node not geometry end point.");
          return -1;
        }
      }
    }
  }

  if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);

  if ( ! skipISOChecks )
  {
    if ( _rtt_CheckEdgeCrossing( topo, startNode, endNode, geom, 0 ) )
    {
      /* would have called rterror already, leaking :( */
      return -1;
    }
  }

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = rtt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* TODO: this should likely be an exception instead ! */
  if ( containing_face == -1 ) containing_face = 0;

  newedge.start_node = startNode;
  newedge.end_node = endNode;
  newedge.face_left = newedge.face_right = containing_face;
  newedge.next_left = -newedge.edge_id;
  newedge.next_right = newedge.edge_id;
  newedge.geom = (RTLINE *)geom; /* const cast.. */

  int ret = rtt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rterror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /*
   * Update Node containing_face values
   *
   * the nodes anode and anothernode are no more isolated
   * because now there is an edge connecting them
   */ 
  updated_nodes[0].node_id = startNode;
  updated_nodes[0].containing_face = -1;
  updated_nodes[1].node_id = endNode;
  updated_nodes[1].containing_face = -1;
  ret = rtt_be_updateNodesById(topo, updated_nodes, 2,
                               RTT_COL_NODE_CONTAINING_FACE);
  if ( ret == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return newedge.edge_id;
}

static RTCOLLECTION *
_rtt_EdgeSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge, RTPOINT* pt, int skipISOChecks, RTT_ISO_EDGE** oldedge )
{
  RTGEOM *split;
  RTCOLLECTION *split_col;
  int i;

  /* Get edge */
  i = 1;
  RTDEBUG(1, "calling rtt_be_getEdgeById");
  *oldedge = rtt_be_getEdgeById(topo, &edge, &i, RTT_COL_EDGE_ALL);
  RTDEBUGF(1, "rtt_be_getEdgeById returned %p", *oldedge);
  if ( ! *oldedge )
  {
    RTDEBUGF(1, "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    else if ( i == 0 )
    {
      rterror("SQL/MM Spatial exception - non-existent edge");
      return NULL;
    }
    else
    {
      rterror("Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return NULL;
    }
  }


  /*
   *  - check if a coincident node already exists
   */
  if ( ! skipISOChecks )
  {
    RTDEBUG(1, "calling rtt_be_ExistsCoincidentNode");
    if ( rtt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      RTDEBUG(1, "rtt_be_ExistsCoincidentNode returned");
      _rtt_release_edges(*oldedge, 1);
      rterror("SQL/MM Spatial exception - coincident node");
      return NULL;
    }
    RTDEBUG(1, "rtt_be_ExistsCoincidentNode returned");
  }

  /* Split edge */
  split = rtgeom_split((RTGEOM*)(*oldedge)->geom, (RTGEOM*)pt);
  if ( ! split )
  {
    _rtt_release_edges(*oldedge, 1);
    rterror("could not split edge by point ?");
    return NULL;
  }
  split_col = rtgeom_as_rtcollection(split);
  if ( ! split_col ) {
    _rtt_release_edges(*oldedge, 1);
    rtgeom_free(split);
    rterror("rtgeom_as_rtcollection returned NULL");
    return NULL;
  }
  if (split_col->ngeoms < 2) {
    _rtt_release_edges(*oldedge, 1);
    rtgeom_free(split);
    rterror("SQL/MM Spatial exception - point not on edge");
    return NULL;
  }

#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt((RTGEOM*)split_col, RTWKT_EXTENDED, 2, &sz);
  RTDEBUGF(1, "returning split col: %s", wkt);
  rtfree(wkt);
  }
#endif
  return split_col;
}

RTT_ELEMID
rtt_ModEdgeSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                  RTPOINT* pt, int skipISOChecks )
{
  RTT_ISO_NODE node;
  RTT_ISO_EDGE* oldedge = NULL;
  RTCOLLECTION *split_col;
  const RTGEOM *oldedge_geom;
  const RTGEOM *newedge_geom;
  RTT_ISO_EDGE newedge1;
  RTT_ISO_EDGE seledge, updedge, excedge;
  int ret;

  split_col = _rtt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];
  /* Make sure the SRID is set on the subgeom */
  ((RTGEOM*)oldedge_geom)->srid = split_col->srid;
  ((RTGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    /* should have been set by backend */
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Insert the new edge */
  newedge1.edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedge1.edge_id == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedge1.start_node = node.node_id;
  newedge1.end_node = oldedge->end_node;
  newedge1.face_left = oldedge->face_left;
  newedge1.face_right = oldedge->face_right;
  newedge1.next_left = oldedge->next_left == -oldedge->edge_id ?
      -newedge1.edge_id : oldedge->next_left;
  newedge1.next_right = -oldedge->edge_id;
  newedge1.geom = rtgeom_as_rtline(newedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedge1.geom ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("first geometry in rtgeom_split output is not a line");
    return -1;
  }
  ret = rtt_be_insertEdges(topo, &newedge1, 1);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update the old edge */
  updedge.geom = rtgeom_as_rtline(oldedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! updedge.geom ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("second geometry in rtgeom_split output is not a line");
    return -1;
  }
  updedge.next_left = newedge1.edge_id;
  updedge.end_node = node.node_id;
  ret = rtt_be_updateEdges(topo,
      oldedge, RTT_COL_EDGE_EDGE_ID,
      &updedge, RTT_COL_EDGE_GEOM|RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      NULL, 0);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Edge being split (%d) disappeared during operations?", oldedge->edge_id);
    return -1;
  } else if ( ret > 1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("More than a single edge found with id %d !", oldedge->edge_id);
    return -1;
  }

  /* Update all next edge references to match new layout (ST_ModEdgeSplit) */

  updedge.next_right = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_right = -oldedge->edge_id;
  seledge.start_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      &excedge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_left = -oldedge->edge_id;
  seledge.end_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      &excedge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = rtt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedge1.edge_id, -1);
  if ( ! ret ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  _rtt_release_edges(oldedge, 1);
  rtcollection_free(split_col);

  /* return new node id */
  return node.node_id;
}

RTT_ELEMID
rtt_NewEdgesSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                   RTPOINT* pt, int skipISOChecks )
{
  RTT_ISO_NODE node;
  RTT_ISO_EDGE* oldedge = NULL;
  RTCOLLECTION *split_col;
  const RTGEOM *oldedge_geom;
  const RTGEOM *newedge_geom;
  RTT_ISO_EDGE newedges[2];
  RTT_ISO_EDGE seledge, updedge;
  int ret;

  split_col = _rtt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];
  /* Make sure the SRID is set on the subgeom */
  ((RTGEOM*)oldedge_geom)->srid = split_col->srid;
  ((RTGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    /* should have been set by backend */
    rterror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Delete the old edge */
  seledge.edge_id = edge;
  ret = rtt_be_deleteEdges(topo, &seledge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Get new edges identifiers */
  newedges[0].edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedges[0].edge_id == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedges[1].edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedges[1].edge_id == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Define the first new edge (to new node) */
  newedges[0].start_node = oldedge->start_node;
  newedges[0].end_node = node.node_id;
  newedges[0].face_left = oldedge->face_left;
  newedges[0].face_right = oldedge->face_right;
  newedges[0].next_left = newedges[1].edge_id;
  if ( oldedge->next_right == edge )
    newedges[0].next_right = newedges[0].edge_id;
  else if ( oldedge->next_right == -edge )
    newedges[0].next_right = -newedges[1].edge_id;
  else
    newedges[0].next_right = oldedge->next_right;
  newedges[0].geom = rtgeom_as_rtline(oldedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedges[0].geom ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("first geometry in rtgeom_split output is not a line");
    return -1;
  }

  /* Define the second new edge (from new node) */
  newedges[1].start_node = node.node_id;
  newedges[1].end_node = oldedge->end_node;
  newedges[1].face_left = oldedge->face_left;
  newedges[1].face_right = oldedge->face_right;
  newedges[1].next_right = -newedges[0].edge_id;
  if ( oldedge->next_left == -edge )
    newedges[1].next_left = -newedges[1].edge_id;
  else if ( oldedge->next_left == edge )
    newedges[1].next_left = newedges[0].edge_id;
  else
    newedges[1].next_left = oldedge->next_left;
  newedges[1].geom = rtgeom_as_rtline(newedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedges[1].geom ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("second geometry in rtgeom_split output is not a line");
    return -1;
  }

  /* Insert both new edges */
  ret = rtt_be_insertEdges(topo, newedges, 2);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update all next edge references pointing to old edge id */

  updedge.next_right = newedges[1].edge_id;
  seledge.next_right = edge;
  seledge.start_node = oldedge->start_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_right = -newedges[0].edge_id;
  seledge.next_right = -edge;
  seledge.start_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = newedges[0].edge_id;
  seledge.next_left = edge;
  seledge.end_node = oldedge->start_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedges[1].edge_id;
  seledge.next_left = -edge;
  seledge.end_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_release(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = rtt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedges[0].edge_id, newedges[1].edge_id);
  if ( ! ret ) {
    _rtt_release_edges(oldedge, 1);
    rtcollection_free(split_col);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  _rtt_release_edges(oldedge, 1);
  rtcollection_free(split_col);

  /* return new node id */
  return node.node_id;
}

/* Data structure used by AddEdgeX functions */
typedef struct edgeend_t {
  /* Signed identifier of next clockwise edge (+outgoing,-incoming) */
  RTT_ELEMID nextCW;
  /* Identifier of face between myaz and next CW edge */
  RTT_ELEMID cwFace;
  /* Signed identifier of next counterclockwise edge (+outgoing,-incoming) */
  RTT_ELEMID nextCCW;
  /* Identifier of face between myaz and next CCW edge */
  RTT_ELEMID ccwFace;
  int was_isolated;
  double myaz; /* azimuth of edgeend geometry */
} edgeend;

/* 
 * Get first distinct vertex from endpoint
 * @param pa the pointarray to seek points in
 * @param ref the point we want to search a distinct one
 * @param from vertex index to start from
 * @param dir  1 to go forward
 *            -1 to go backward
 * @return 0 if edge is collapsed (no distinct points)
 */
static int
_rtt_FirstDistinctVertex2D(const POINTARRAY* pa, POINT2D *ref, int from, int dir, POINT2D *op)
{
  int i, toofar, inc;
  POINT2D fp;

  if ( dir > 0 )
  {
    toofar = pa->npoints;
    inc = 1;
  }
  else
  {
    toofar = -1;
    inc = -1;
  }

  RTDEBUGF(1, "first point is index %d", from);
  fp = *ref; /* getPoint2d_p(pa, from, &fp); */
  for ( i = from+inc; i != toofar; i += inc )
  {
    RTDEBUGF(1, "testing point %d", i);
    getPoint2d_p(pa, i, op); /* pick next point */
    if ( p2d_same(op, &fp) ) continue; /* equal to startpoint */
    /* this is a good one, neither same of start nor of end point */
    return 1; /* found */
  }

  /* no distinct vertices found */
  return 0;
}


/*
 * Return non-zero on failure (rterror is invoked in that case)
 * Possible failures:
 *  -1 no two distinct vertices exist
 *  -2 azimuth computation failed for first edge end
 */
static int
_rtt_InitEdgeEndByLine(edgeend *fee, edgeend *lee, RTLINE *edge,
                                            POINT2D *fp, POINT2D *lp)
{
  POINTARRAY *pa = edge->points;
  POINT2D pt;

  fee->nextCW = fee->nextCCW =
  lee->nextCW = lee->nextCCW = 0;
  fee->cwFace = fee->ccwFace =
  lee->cwFace = lee->ccwFace = -1;

  /* Compute azimuth of first edge end */
  RTDEBUG(1, "computing azimuth of first edge end");
  if ( ! _rtt_FirstDistinctVertex2D(pa, fp, 0, 1, &pt) )
  {
    rterror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(fp, &pt, &(fee->myaz)) ) {
    rterror("error computing azimuth of first edgeend [%g %g,%g %g]",
            fp->x, fp->y, pt.x, pt.y);
    return -2;
  }
  RTDEBUGF(1, "azimuth of first edge end [%g %g,%g %g] is %g",
            fp->x, fp->y, pt.x, pt.y, fee->myaz);

  /* Compute azimuth of second edge end */
  RTDEBUG(1, "computing azimuth of second edge end");
  if ( ! _rtt_FirstDistinctVertex2D(pa, lp, pa->npoints-1, -1, &pt) )
  {
    rterror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(lp, &pt, &(lee->myaz)) ) {
    rterror("error computing azimuth of last edgeend [%g %g,%g %g]",
            lp->x, lp->y, pt.x, pt.y);
    return -2;
  }
  RTDEBUGF(1, "azimuth of last edge end [%g %g,%g %g] is %g",
            lp->x, lp->y, pt.x, pt.y, lee->myaz);

  return 0;
}

/*
 * Find the first edges encountered going clockwise and counterclockwise
 * around a node, starting from the given azimuth, and take
 * note of the face on the both sides.
 *
 * @param topo the topology to act upon
 * @param node the identifier of the node to analyze
 * @param data input (myaz) / output (nextCW, nextCCW) parameter
 * @param other edgeend, if also incident to given node (closed edge).
 * @param myedge_id identifier of the edge id that data->myaz belongs to
 * @return number of incident edges found
 *
 */
static int
_rtt_FindAdjacentEdges( RTT_TOPOLOGY* topo, RTT_ELEMID node, edgeend *data,
                        edgeend *other, int myedge_id )
{
  RTT_ISO_EDGE *edges;
  int numedges = 1;
  int i;
  double minaz, maxaz;
  double az, azdif;

  data->nextCW = data->nextCCW = 0;
  data->cwFace = data->ccwFace = -1;

  if ( other ) {
    azdif = other->myaz - data->myaz;
    if ( azdif < 0 ) azdif += 2 * M_PI;
    minaz = maxaz = azdif;
    /* TODO: set nextCW/nextCCW/cwFace/ccwFace to other->something ? */
    RTDEBUGF(1, "Other edge end has cwFace=%d and ccwFace=%d",
                other->cwFace, other->ccwFace);
  } else {
    minaz = maxaz = -1;
  }

  RTDEBUGF(1, "Looking for edges incident to node %" RTTFMT_ELEMID
              " and adjacent to azimuth %g", node, data->myaz);

  /* Get incident edges */
  edges = rtt_be_getEdgeByNode( topo, &node, &numedges, RTT_COL_EDGE_ALL );
  if ( numedges == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }

  RTDEBUGF(1, "getEdgeByNode returned %d edges, minaz=%g, maxaz=%g",
              numedges, minaz, maxaz);

  /* For each incident edge-end (1 or 2): */
  for ( i = 0; i < numedges; ++i )
  {
    RTT_ISO_EDGE *edge;
    RTGEOM *g;
    RTGEOM *cleangeom;
    POINT2D p1, p2;
    POINTARRAY *pa;

    edge = &(edges[i]);

    if ( edge->edge_id == myedge_id ) continue;

    g = rtline_as_rtgeom(edge->geom);
    /* NOTE: remove_repeated_points call could be replaced by
     * some other mean to pick two distinct points for endpoints */
    cleangeom = rtgeom_remove_repeated_points( g, 0 );
    pa = rtgeom_as_rtline(cleangeom)->points;

    if ( pa->npoints < 2 ) {{
      RTT_ELEMID id = edge->edge_id;
      _rtt_release_edges(edges, numedges);
      rtgeom_free(cleangeom);
      rterror("corrupted topology: edge %" RTTFMT_ELEMID
              " does not have two distinct points", id);
      return -1;
    }}

    if ( edge->start_node == node ) {
      getPoint2d_p(pa, 0, &p1);
      getPoint2d_p(pa, 1, &p2);
      RTDEBUGF(1, "edge %" RTTFMT_ELEMID
                  " starts on node %" RTTFMT_ELEMID
                  ", edgeend is %g,%g-%g,%g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {{
        RTT_ELEMID id = edge->edge_id;
        _rtt_release_edges(edges, numedges);
        rtgeom_free(cleangeom);
        rterror("error computing azimuth of edge %d first edgeend [%g,%g-%g,%g]",
                id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      RTDEBUGF(1, "azimuth of edge %" RTTFMT_ELEMID
                  ": %g (diff: %g)", edge->edge_id, az, azdif);

      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = edge->edge_id; /* outgoing */
        data->cwFace = edge->face_left;
        data->ccwFace = edge->face_right;
        RTDEBUGF(1, "new nextCW and nextCCW edge is %" RTTFMT_ELEMID
                    ", outgoing, "
                    "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                    " (face_right is new ccwFace, face_left is new cwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = edge->edge_id; /* outgoing */
          data->cwFace = edge->face_left;
          RTDEBUGF(1, "new nextCW edge is %" RTTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had minaz=%g, face_left is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = edge->edge_id; /* outgoing */
          data->ccwFace = edge->face_right;
          RTDEBUGF(1, "new nextCCW edge is %" RTTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had maxaz=%g, face_right is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    if ( edge->end_node == node ) {
      getPoint2d_p(pa, pa->npoints-1, &p1);
      getPoint2d_p(pa, pa->npoints-2, &p2);
      RTDEBUGF(1, "edge %" RTTFMT_ELEMID " ends on node %" RTTFMT_ELEMID
                  ", edgeend is %g,%g-%g,%g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {{
        RTT_ELEMID id = edge->edge_id;
        _rtt_release_edges(edges, numedges);
        rtgeom_free(cleangeom);
        rterror("error computing azimuth of edge %d last edgeend [%g,%g-%g,%g]",
                id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      RTDEBUGF(1, "azimuth of edge %" RTTFMT_ELEMID
                  ": %g (diff: %g)", edge->edge_id, az, azdif);
      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = -edge->edge_id; /* incoming */
        data->cwFace = edge->face_right;
        data->ccwFace = edge->face_left;
        RTDEBUGF(1, "new nextCW and nextCCW edge is %" RTTFMT_ELEMID
                    ", incoming, "
                    "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                    " (face_right is new cwFace, face_left is new ccwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = -edge->edge_id; /* incoming */
          data->cwFace = edge->face_right;
          RTDEBUGF(1, "new nextCW edge is %" RTTFMT_ELEMID
                      ", incoming, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had minaz=%g, face_right is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = -edge->edge_id; /* incoming */
          data->ccwFace = edge->face_left;
          RTDEBUGF(1, "new nextCCW edge is %" RTTFMT_ELEMID
                      ", outgoing, from start point, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had maxaz=%g, face_left is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    rtgeom_free(cleangeom);
  }
  if ( numedges ) _rtt_release_edges(edges, numedges);

  RTDEBUGF(1, "edges adjacent to azimuth %g"
              " (incident to node %" RTTFMT_ELEMID ")"
              ": CW:%" RTTFMT_ELEMID "(%g) CCW:%" RTTFMT_ELEMID "(%g)",
              data->myaz, node, data->nextCW, minaz,
              data->nextCCW, maxaz);

  if ( myedge_id < 1 && numedges && data->cwFace != data->ccwFace )
  {
    if ( data->cwFace != -1 && data->ccwFace != -1 ) {
      rterror("Corrupted topology: adjacent edges %" RTTFMT_ELEMID " and %" RTTFMT_ELEMID
              " bind different face (%" RTTFMT_ELEMID " and %" RTTFMT_ELEMID ")",
              data->nextCW, data->nextCCW,
              data->cwFace, data->ccwFace);
      return -1;
    }
  }

  /* Return number of incident edges found */
  return numedges;
}

/*
 * Get a point internal to the line and write it into the "ip"
 * parameter
 *
 * return 0 on failure (line is empty or collapsed), 1 otherwise
 */
static int
_rtt_GetInteriorEdgePoint(const RTLINE* edge, POINT2D* ip)
{
  int i;
  POINT2D fp, lp, tp;
  POINTARRAY *pa = edge->points;

  if ( pa->npoints < 2 ) return 0; /* empty or structurally collapsed */

  getPoint2d_p(pa, 0, &fp); /* save first point */
  getPoint2d_p(pa, pa->npoints-1, &lp); /* save last point */
  for (i=1; i<pa->npoints-1; ++i)
  {
    getPoint2d_p(pa, i, &tp); /* pick next point */
    if ( p2d_same(&tp, &fp) ) continue; /* equal to startpoint */
    if ( p2d_same(&tp, &lp) ) continue; /* equal to endpoint */
    /* this is a good one, neither same of start nor of end point */
    *ip = tp;
    return 1; /* found */
  }

  /* no distinct vertex found */

  /* interpolate if start point != end point */

  if ( p2d_same(&fp, &lp) ) return 0; /* no distinct points in edge */
 
  ip->x = fp.x + ( (lp.x - fp.x) * 0.5 );
  ip->y = fp.y + ( (lp.y - fp.y) * 0.5 );

  return 1;
}

/*
 * Add a split face by walking on the edge side.
 *
 * @param topo the topology to act upon
 * @param sedge edge id and walking side and direction
 *              (forward,left:positive backward,right:negative)
 * @param face the face in which the edge identifier is known to be
 * @param mbr_only do not create a new face but update MBR of the current
 *
 * @return:
 *    -1: if mbr_only was requested
 *     0: if the edge does not form a ring
 *    -1: if it is impossible to create a face on the requested side
 *        ( new face on the side is the universe )
 *    -2: error
 *   >0 : id of newly added face
 */
static RTT_ELEMID
_rtt_AddFaceSplit( RTT_TOPOLOGY* topo,
                   RTT_ELEMID sedge, RTT_ELEMID face,
                   int mbr_only )
{
  int numedges, numfaceedges, i, j;
  int newface_outside;
  int num_signed_edge_ids;
  RTT_ELEMID *signed_edge_ids;
  RTT_ELEMID *edge_ids;
  RTT_ISO_EDGE *edges;
  RTT_ISO_EDGE *ring_edges;
  RTT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  RTT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;

  signed_edge_ids = rtt_be_getRingEdges(topo, sedge,
                                        &num_signed_edge_ids, 0);
  if ( ! signed_edge_ids ) {
    rterror("Backend error (no ring edges for edge %" RTTFMT_ELEMID "): %s",
            sedge, rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  RTDEBUGF(1, "getRingEdges returned %d edges", num_signed_edge_ids);

  /* You can't get to the other side of an edge forming a ring */
  for (i=0; i<num_signed_edge_ids; ++i) {
    if ( signed_edge_ids[i] == -sedge ) {
      /* No split here */
      RTDEBUG(1, "not a ring");
      rtfree( signed_edge_ids );
      return 0;
    }
  }

  RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " split face %" RTTFMT_ELEMID " (mbr_only:%d)",
           sedge, face, mbr_only);

  /* Construct a polygon using edges of the ring */
  numedges = 0;
  edge_ids = rtalloc(sizeof(RTT_ELEMID)*num_signed_edge_ids);
  for (i=0; i<num_signed_edge_ids; ++i) {
    int absid = llabs(signed_edge_ids[i]);
    int found = 0;
    /* Do not add the same edge twice */
    for (j=0; j<numedges; ++j) {
      if ( edge_ids[j] == absid ) {
        found = 1;
        break;
      }
    }
    if ( ! found ) edge_ids[numedges++] = absid;
  }
  i = numedges;
  ring_edges = rtt_be_getEdgeById(topo, edge_ids, &i,
                                  RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM);
  rtfree( edge_ids );
  if ( i == -1 )
  {
    rtfree( signed_edge_ids );
    /* ring_edges should be NULL */
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  else if ( i != numedges )
  {
    rtfree( signed_edge_ids );
    _rtt_release_edges(ring_edges, numedges);
    rterror("Unexpected error: %d edges found when expecting %d", i, numedges);
    return -2;
  }

  /* Should now build a polygon with those edges, in the order
   * given by GetRingEdges.
   */
  POINTARRAY *pa = NULL;
  for ( i=0; i<num_signed_edge_ids; ++i )
  {
    RTT_ELEMID eid = signed_edge_ids[i];
    RTDEBUGF(1, "Edge %d in ring of edge %" RTTFMT_ELEMID " is edge %" RTTFMT_ELEMID,
                i, sedge, eid);
    RTT_ISO_EDGE *edge = NULL;
    POINTARRAY *epa;
    for ( j=0; j<numedges; ++j )
    {
      if ( ring_edges[j].edge_id == llabs(eid) )
      {
        edge = &(ring_edges[j]);
        break;
      }
    }
    if ( edge == NULL )
    {
      rtfree( signed_edge_ids );
      _rtt_release_edges(ring_edges, numedges);
      rterror("missing edge that was found in ring edges loop");
      return -2;
    }

    if ( pa == NULL )
    {
      pa = ptarray_clone_deep(edge->geom->points);
      if ( eid < 0 ) ptarray_reverse(pa);
    }
    else
    {
      if ( eid < 0 )
      {
        epa = ptarray_clone_deep(edge->geom->points);
        ptarray_reverse(epa);
        ptarray_append_ptarray(pa, epa, 0);
        ptarray_free(epa);
      }
      else
      {
        /* avoid a clone here */
        ptarray_append_ptarray(pa, edge->geom->points, 0);
      }
    }
  }
  POINTARRAY **points = rtalloc(sizeof(POINTARRAY*));
  points[0] = pa;
  /* NOTE: the ring may very well have collapsed components,
   *       which would make it topologically invalid
   */
  RTPOLY* shell = rtpoly_construct(0, 0, 1, points);

  int isccw = ptarray_isccw(pa);
  RTDEBUGF(1, "Ring of edge %" RTTFMT_ELEMID " is %sclockwise",
              sedge, isccw ? "counter" : "");
  const GBOX* shellbox = rtgeom_get_bbox(rtpoly_as_rtgeom(shell));

  if ( face == 0 )
  {
    /* Edge split the universe face */
    if ( ! isccw )
    {
      rtpoly_free(shell);
      rtfree( signed_edge_ids );
      _rtt_release_edges(ring_edges, numedges);
      /* Face on the left side of this ring is the universe face.
       * Next call (for the other side) should create the split face
       */
      RTDEBUG(1, "The left face of this clockwise ring is the universe, "
                 "won't create a new face there");
      return -1;
    }
  }

  if ( mbr_only && face != 0 )
  {
    if ( isccw )
    {{
      RTT_ISO_FACE updface;
      updface.face_id = face;
      updface.mbr = (GBOX *)shellbox; /* const cast, we won't free it, later */
      int ret = rtt_be_updateFacesById( topo, &updface, 1 );
      if ( ret == -1 )
      {
        rtfree( signed_edge_ids );
        _rtt_release_edges(ring_edges, numedges);
        rtpoly_free(shell); /* NOTE: owns shellbox above */
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != 1 )
      {
        rtfree( signed_edge_ids );
        _rtt_release_edges(ring_edges, numedges);
        rtpoly_free(shell); /* NOTE: owns shellbox above */
        rterror("Unexpected error: %d faces found when expecting 1", ret);
        return -2;
      }
    }}
    rtfree( signed_edge_ids );
    _rtt_release_edges(ring_edges, numedges);
    rtpoly_free(shell); /* NOTE: owns shellbox above */
    return -1; /* mbr only was requested */
  }

  RTT_ISO_FACE *oldface = NULL;
  RTT_ISO_FACE newface;
  newface.face_id = -1;
  if ( face != 0 && ! isccw)
  {{
    /* Face created an hole in an outer face */
    int nfaces = 1;
    oldface = rtt_be_getFaceById(topo, &face, &nfaces, RTT_COL_FACE_ALL);
    if ( nfaces == -1 )
    {
      rtfree( signed_edge_ids );
      rtpoly_free(shell); /* NOTE: owns shellbox */
      _rtt_release_edges(ring_edges, numedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -2;
    }
    if ( nfaces != 1 )
    {
      rtfree( signed_edge_ids );
      rtpoly_free(shell); /* NOTE: owns shellbox */
      _rtt_release_edges(ring_edges, numedges);
      rterror("Unexpected error: %d faces found when expecting 1", nfaces);
      return -2;
    }
    newface.mbr = oldface->mbr;
  }}
  else
  {
    newface.mbr = (GBOX *)shellbox; /* const cast, we won't free it, later */
  }

  /* Insert the new face */
  int ret = rtt_be_insertFaces( topo, &newface, 1 );
  if ( ret == -1 )
  {
    rtfree( signed_edge_ids );
    rtpoly_free(shell); /* NOTE: owns shellbox */
    _rtt_release_edges(ring_edges, numedges);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( ret != 1 )
  {
    rtfree( signed_edge_ids );
    rtpoly_free(shell); /* NOTE: owns shellbox */
    _rtt_release_edges(ring_edges, numedges);
    rterror("Unexpected error: %d faces inserted when expecting 1", ret);
    return -2;
  }
  if ( oldface ) {
    newface.mbr = NULL; /* it is a reference to oldface mbr... */
    _rtt_release_faces(oldface, 1);
  }

  /* Update side location of new face edges */

  /* We want the new face to be on the left, if possible */
  if ( face != 0 && ! isccw ) { /* ring is clockwise in a real face */
    /* face shrinked, must update all non-contained edges and nodes */
    RTDEBUG(1, "New face is on the outside of the ring, updating rings in former shell");
    newface_outside = 1;
    /* newface is outside */
  } else {
    RTDEBUG(1, "New face is on the inside of the ring, updating forward edges in new ring");
    newface_outside = 0;
    /* newface is inside */
  }

  /* Update edges bounding the old face */
  /* (1) fetch all edges where left_face or right_face is = oldface */
  int fields = RTT_COL_EDGE_EDGE_ID |
               RTT_COL_EDGE_FACE_LEFT |
               RTT_COL_EDGE_FACE_RIGHT |
               RTT_COL_EDGE_GEOM
               ;
  numfaceedges = 1;
  edges = rtt_be_getEdgeByFace( topo, &face, &numfaceedges, fields, newface.mbr );
  if ( numfaceedges == -1 ) {
    rtfree( signed_edge_ids );
    _rtt_release_edges(ring_edges, numedges);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  RTDEBUGF(1, "rtt_be_getEdgeByFace returned %d edges", numfaceedges);
  GEOSGeometry *shellgg = 0;
  const GEOSPreparedGeometry* prepshell = 0;
  shellgg = RTGEOM2GEOS( rtpoly_as_rtgeom(shell), 0);
  if ( ! shellgg ) {
    rtpoly_free(shell);
    rtfree(signed_edge_ids);
    _rtt_release_edges(ring_edges, numedges);
    _rtt_release_edges(edges, numfaceedges);
    rterror("Could not convert shell geometry to GEOS: %s", rtgeom_geos_errmsg);
    return -2;
  }
  prepshell = GEOSPrepare( shellgg );
  if ( ! prepshell ) {
    GEOSGeom_destroy(shellgg);
    rtpoly_free(shell);
    rtfree(signed_edge_ids);
    _rtt_release_edges(ring_edges, numedges);
    _rtt_release_edges(edges, numfaceedges);
    rterror("Could not prepare shell geometry: %s", rtgeom_geos_errmsg);
    return -2;
  }

  if ( numfaceedges )
  {
    forward_edges = rtalloc(sizeof(RTT_ISO_EDGE)*numfaceedges);
    forward_edges_count = 0;
    backward_edges = rtalloc(sizeof(RTT_ISO_EDGE)*numfaceedges);
    backward_edges_count = 0;

    /* (2) loop over the results and: */
    for ( i=0; i<numfaceedges; ++i )
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      int found = 0;
      int contains;
      GEOSGeometry *egg;
      RTPOINT *epgeom;
      POINT2D ep;

      /* (2.1) skip edges whose ID is in the list of boundary edges ? */
      for ( j=0; j<num_signed_edge_ids; ++j )
      {
        int seid = signed_edge_ids[j];
        if ( seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          RTDEBUGF(1, "Edge %d is a forward edge of the new ring", e->edge_id);
          forward_edges[forward_edges_count].edge_id = e->edge_id;
          forward_edges[forward_edges_count++].face_left = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
        else if ( -seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          RTDEBUGF(1, "Edge %d is a backward edge of the new ring", e->edge_id);
          backward_edges[backward_edges_count].edge_id = e->edge_id;
          backward_edges[backward_edges_count++].face_right = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
      }
      if ( found ) continue;

      /* We need to check only a single point
       * (to avoid collapsed elements of the shell polygon
       * giving false positive).
       * The point but must not be an endpoint.
       */
      if ( ! _rtt_GetInteriorEdgePoint(e->geom, &ep) )
      {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        rtfree(signed_edge_ids);
        rtpoly_free(shell);
        rtfree(forward_edges); /* contents owned by ring_edges */
        rtfree(backward_edges); /* contents owned by ring_edges */
        _rtt_release_edges(ring_edges, numedges);
        _rtt_release_edges(edges, numfaceedges);
        rterror("Could not find interior point for edge %d: %s",
                e->edge_id, rtgeom_geos_errmsg);
        return -2;
      }

      epgeom = rtpoint_make2d(0, ep.x, ep.y);
      egg = RTGEOM2GEOS( rtpoint_as_rtgeom(epgeom) , 0);
      rtpoint_free(epgeom);
      if ( ! egg ) {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        rtfree(signed_edge_ids);
        rtpoly_free(shell);
        rtfree(forward_edges); /* contents owned by ring_edges */
        rtfree(backward_edges); /* contents owned by ring_edges */
        _rtt_release_edges(ring_edges, numedges);
        _rtt_release_edges(edges, numfaceedges);
        rterror("Could not convert edge geometry to GEOS: %s",
                rtgeom_geos_errmsg);
        return -2;
      }
      /* IDEA: can be optimized by computing this on our side rather
       *       than on GEOS (saves conversion of big edges) */
      /* IDEA: check that bounding box shortcut is taken, or use
       *       shellbox to do it here */
      contains = GEOSPreparedContains( prepshell, egg );
      GEOSGeom_destroy(egg);
      if ( contains == 2 )
      {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        rtfree(signed_edge_ids);
        rtpoly_free(shell);
        rtfree(forward_edges); /* contents owned by ring_edges */
        rtfree(backward_edges); /* contents owned by ring_edges */
        _rtt_release_edges(ring_edges, numedges);
        _rtt_release_edges(edges, numfaceedges);
        rterror("GEOS exception on PreparedContains: %s", rtgeom_geos_errmsg);
        return -2;
      }
      RTDEBUGF(1, "Edge %d %scontained in new ring", e->edge_id,
                  (contains?"":"not "));

      /* (2.2) skip edges (NOT, if newface_outside) contained in shell */
      if ( newface_outside )
      {
        if ( contains )
        {
          RTDEBUGF(1, "Edge %d contained in an hole of the new face",
                      e->edge_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          RTDEBUGF(1, "Edge %d not contained in the face shell",
                      e->edge_id);
          continue;
        }
      }

      /* (2.3) push to forward_edges if left_face = oface */
      if ( e->face_left == face )
      {
        RTDEBUGF(1, "Edge %d has new face on the left side", e->edge_id);
        forward_edges[forward_edges_count].edge_id = e->edge_id;
        forward_edges[forward_edges_count++].face_left = newface.face_id;
      }

      /* (2.4) push to backward_edges if right_face = oface */
      if ( e->face_right == face )
      {
        RTDEBUGF(1, "Edge %d has new face on the right side", e->edge_id);
        backward_edges[backward_edges_count].edge_id = e->edge_id;
        backward_edges[backward_edges_count++].face_right = newface.face_id;
      }
    }

    /* Update forward edges */
    if ( forward_edges_count )
    {
      ret = rtt_be_updateEdgesById(topo, forward_edges,
                                   forward_edges_count,
                                   RTT_COL_EDGE_FACE_LEFT);
      if ( ret == -1 )
      {
        rtfree( signed_edge_ids );
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != forward_edges_count )
      {
        rtfree( signed_edge_ids );
        rterror("Unexpected error: %d edges updated when expecting %d",
                ret, forward_edges_count);
        return -2;
      }
    }

    /* Update backward edges */
    if ( backward_edges_count )
    {
      ret = rtt_be_updateEdgesById(topo, backward_edges,
                                   backward_edges_count,
                                   RTT_COL_EDGE_FACE_RIGHT);
      if ( ret == -1 )
      {
        rtfree( signed_edge_ids );
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != backward_edges_count )
      {
        rtfree( signed_edge_ids );
        rterror("Unexpected error: %d edges updated when expecting %d",
                ret, backward_edges_count);
        return -2;
      }
    }

    rtfree(forward_edges);
    rtfree(backward_edges);

  }

  _rtt_release_edges(ring_edges, numedges);
  _rtt_release_edges(edges, numfaceedges);

  /* Update isolated nodes which are now in new face */
  int numisonodes = 1;
  fields = RTT_COL_NODE_NODE_ID | RTT_COL_NODE_GEOM;
  RTT_ISO_NODE *nodes = rtt_be_getNodeByFace(topo, &face,
                                             &numisonodes, fields, newface.mbr);
  if ( numisonodes == -1 ) {
    rtfree( signed_edge_ids );
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( numisonodes ) {
    RTT_ISO_NODE *updated_nodes = rtalloc(sizeof(RTT_ISO_NODE)*numisonodes);
    int nodes_to_update = 0;
    for (i=0; i<numisonodes; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      GEOSGeometry *ngg;
      ngg = RTGEOM2GEOS( rtpoint_as_rtgeom(n->geom), 0 );
      int contains;
      if ( ! ngg ) {
        _rtt_release_nodes(nodes, numisonodes);
        if ( prepshell ) GEOSPreparedGeom_destroy(prepshell);
        if ( shellgg ) GEOSGeom_destroy(shellgg);
        rtfree(signed_edge_ids);
        rtpoly_free(shell);
        rterror("Could not convert node geometry to GEOS: %s",
                rtgeom_geos_errmsg);
        return -2;
      }
      contains = GEOSPreparedContains( prepshell, ngg );
      GEOSGeom_destroy(ngg);
      if ( contains == 2 )
      {
        _rtt_release_nodes(nodes, numisonodes);
        if ( prepshell ) GEOSPreparedGeom_destroy(prepshell);
        if ( shellgg ) GEOSGeom_destroy(shellgg);
        rtfree(signed_edge_ids);
        rtpoly_free(shell);
        rterror("GEOS exception on PreparedContains: %s", rtgeom_geos_errmsg);
        return -2;
      }
      RTDEBUGF(1, "Node %d is %scontained in new ring, newface is %s",
                  n->node_id, contains ? "" : "not ",
                  newface_outside ? "outside" : "inside" );
      if ( newface_outside )
      {
        if ( contains )
        {
          RTDEBUGF(1, "Node %d contained in an hole of the new face",
                      n->node_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          RTDEBUGF(1, "Node %d not contained in the face shell",
                      n->node_id);
          continue;
        }
      }
      updated_nodes[nodes_to_update].node_id = n->node_id;
      updated_nodes[nodes_to_update++].containing_face =
                                       newface.face_id;
      RTDEBUGF(1, "Node %d will be updated", n->node_id);
    }
    _rtt_release_nodes(nodes, numisonodes);
    if ( nodes_to_update )
    {
      int ret = rtt_be_updateNodesById(topo, updated_nodes,
                                       nodes_to_update,
                                       RTT_COL_NODE_CONTAINING_FACE);
      if ( ret == -1 ) {
        rtfree( signed_edge_ids );
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
    }
    rtfree(updated_nodes);
  }

  GEOSPreparedGeom_destroy(prepshell);
  GEOSGeom_destroy(shellgg);
  rtfree(signed_edge_ids);
  rtpoly_free(shell);

  return newface.face_id;
}

static RTT_ELEMID
_rtt_AddEdge( RTT_TOPOLOGY* topo,
              RTT_ELEMID start_node, RTT_ELEMID end_node,
              RTLINE *geom, int skipChecks, int modFace )
{
  RTT_ISO_EDGE newedge;
  RTGEOM *cleangeom;
  edgeend span; /* start point analisys */
  edgeend epan; /* end point analisys */
  POINT2D p1, pn, p2;
  POINTARRAY *pa;
  RTT_ELEMID node_ids[2];
  const RTPOINT *start_node_geom = NULL;
  const RTPOINT *end_node_geom = NULL;
  int num_nodes;
  RTT_ISO_NODE *endpoints;
  int i;
  int prev_left;
  int prev_right;
  RTT_ISO_EDGE seledge;
  RTT_ISO_EDGE updedge;

  if ( ! skipChecks )
  {
    /* curve must be simple */
    if ( ! rtgeom_is_simple(rtline_as_rtgeom(geom)) )
    {
      rterror("SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  newedge.start_node = start_node;
  newedge.end_node = end_node;
  newedge.geom = geom;
  newedge.face_left = -1;
  newedge.face_right = -1;
  cleangeom = rtgeom_remove_repeated_points( rtline_as_rtgeom(geom), 0 );

  pa = rtgeom_as_rtline(cleangeom)->points;
  if ( pa->npoints < 2 ) {
    rtgeom_free(cleangeom);
    rterror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }

  /* Initialize endpoint info (some of that ) */
  span.cwFace = span.ccwFace =
  epan.cwFace = epan.ccwFace = -1;

  /* Compute azimut of first edge end on start node */
  getPoint2d_p(pa, 0, &p1);
  getPoint2d_p(pa, 1, &pn);
  if ( p2d_same(&p1, &pn) ) {
    rtgeom_free(cleangeom);
    /* Can still happen, for 2-point lines */
    rterror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(&p1, &pn, &span.myaz) ) {
    rtgeom_free(cleangeom);
    rterror("error computing azimuth of first edgeend [%g,%g-%g,%g]",
            p1.x, p1.y, pn.x, pn.y);
    return -1;
  }
  RTDEBUGF(1, "edge's start node is %g,%g", p1.x, p1.y);

  /* Compute azimuth of last edge end on end node */
  getPoint2d_p(pa, pa->npoints-1, &p2);
  getPoint2d_p(pa, pa->npoints-2, &pn);
  rtgeom_free(cleangeom);
  if ( ! azimuth_pt_pt(&p2, &pn, &epan.myaz) ) {
    rterror("error computing azimuth of last edgeend [%g,%g-%g,%g]",
            p2.x, p2.y, pn.x, pn.y);
    return -1;
  }
  RTDEBUGF(1, "edge's end node is %g,%g", p2.x, p2.y);

  /*
   * Check endpoints existance, match with Curve geometry
   * and get face information (if any)
   */

  if ( start_node != end_node ) {
    num_nodes = 2;
    node_ids[0] = start_node;
    node_ids[1] = end_node;
  } else {
    num_nodes = 1;
    node_ids[0] = start_node;
  }

  endpoints = rtt_be_getNodeById( topo, node_ids, &num_nodes, RTT_COL_NODE_ALL );
  if ( num_nodes < 0 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    RTT_ISO_NODE* node = &(endpoints[i]);
    if ( node->containing_face != -1 )
    {
      if ( newedge.face_left == -1 )
      {
        newedge.face_left = newedge.face_right = node->containing_face;
      }
      else if ( newedge.face_left != node->containing_face )
      {
        _rtt_release_nodes(endpoints, num_nodes);
        rterror("SQL/MM Spatial exception - geometry crosses an edge"
                " (endnodes in faces %" RTTFMT_ELEMID " and %" RTTFMT_ELEMID ")",
                newedge.face_left, node->containing_face);
      }
    }

    RTDEBUGF(1, "Node %d, with geom %p (looking for %d and %d)",
             node->node_id, node->geom, start_node, end_node);
    if ( node->node_id == start_node ) {
      start_node_geom = node->geom;
    } 
    if ( node->node_id == end_node ) {
      end_node_geom = node->geom;
    } 
  }

  if ( ! skipChecks )
  {
    if ( ! start_node_geom )
    {
      if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);
      rterror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = start_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! p2d_same(&pn, &p1) )
      {
        if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);
        rterror("SQL/MM Spatial exception"
                " - start node not geometry start point."
                //" - start node not geometry start point (%g,%g != %g,%g).", pn.x, pn.y, p1.x, p1.y
        );
        return -1;
      }
    }

    if ( ! end_node_geom )
    {
      if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);
      rterror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = end_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! p2d_same(&pn, &p2) )
      {
        if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);
        rterror("SQL/MM Spatial exception"
                " - end node not geometry end point."
                //" - end node not geometry end point (%g,%g != %g,%g).", pn.x, pn.y, p2.x, p2.y
        );
        return -1;
      }
    }

    if ( num_nodes ) _rtt_release_nodes(endpoints, num_nodes);

    if ( _rtt_CheckEdgeCrossing( topo, start_node, end_node, geom, 0 ) )
      return -1;

  } /* ! skipChecks */

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = rtt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Find adjacent edges to each endpoint */
  int isclosed = start_node == end_node;
  int found;
  found = _rtt_FindAdjacentEdges( topo, start_node, &span,
                                  isclosed ? &epan : NULL, -1 );
  if ( found ) {
    span.was_isolated = 0;
    newedge.next_right = span.nextCW ? span.nextCW : -newedge.edge_id;
    prev_left = span.nextCCW ? -span.nextCCW : newedge.edge_id;
    RTDEBUGF(1, "New edge %d is connected on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.cwFace;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.ccwFace;
    }
  } else {
    span.was_isolated = 1;
    newedge.next_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    prev_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    RTDEBUGF(1, "New edge %d is isolated on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
  }

  found = _rtt_FindAdjacentEdges( topo, end_node, &epan,
                                  isclosed ? &span : NULL, -1 );
  if ( found ) {
    epan.was_isolated = 0;
    newedge.next_left = epan.nextCW ? epan.nextCW : newedge.edge_id;
    prev_right = epan.nextCCW ? -epan.nextCCW : -newedge.edge_id;
    RTDEBUGF(1, "New edge %d is connected on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.ccwFace;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.cwFace;
    }
  } else {
    epan.was_isolated = 1;
    newedge.next_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    prev_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    RTDEBUGF(1, "New edge %d is isolated on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
  }

  /*
   * If we don't have faces setup by now we must have encountered
   * a malformed topology (no containing_face on isolated nodes, no
   * left/right faces on adjacent edges or mismatching values)
   */
  if ( newedge.face_left != newedge.face_right )
  {
    rterror("Left(%" RTTFMT_ELEMID ")/right(%" RTTFMT_ELEMID ")"
            "faces mismatch: invalid topology ?",
            newedge.face_left, newedge.face_right);
    return -1;
  }
  else if ( newedge.face_left == -1 )
  {
    rterror("Could not derive edge face from linked primitives:"
            " invalid topology ?");
    return -1;
  }

  /*
   * Insert the new edge, and update all linking
   */

  int ret = rtt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rterror("Insertion of split edge failed (no reason)");
    return -1;
  }

  int updfields;

  /* Link prev_left to us
   * (if it's not us already) */
  if ( llabs(prev_left) != newedge.edge_id )
  {
    if ( prev_left > 0 )
    {
      /* its next_left_edge is us */
      updfields = RTT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = newedge.edge_id;
      seledge.edge_id = prev_left;
    }
    else
    {
      /* its next_right_edge is us */
      updfields = RTT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = newedge.edge_id;
      seledge.edge_id = -prev_left;
    }

    ret = rtt_be_updateEdges(topo,
        &seledge, RTT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* Link prev_right to us 
   * (if it's not us already) */
  if ( llabs(prev_right) != newedge.edge_id )
  {
    if ( prev_right > 0 )
    {
      /* its next_left_edge is -us */
      updfields = RTT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = -newedge.edge_id;
      seledge.edge_id = prev_right;
    }
    else
    {
      /* its next_right_edge is -us */
      updfields = RTT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = -newedge.edge_id;
      seledge.edge_id = -prev_right;
    }

    ret = rtt_be_updateEdges(topo,
        &seledge, RTT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* NOT IN THE SPECS...
   * set containing_face = null for start_node and end_node
   * if they where isolated
   *
   */
  RTT_ISO_NODE updnode, selnode;
  updnode.containing_face = -1;
  if ( span.was_isolated )
  {
    selnode.node_id = start_node;
    ret = rtt_be_updateNodes(topo,
        &selnode, RTT_COL_NODE_NODE_ID,
        &updnode, RTT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  if ( epan.was_isolated )
  {
    selnode.node_id = end_node;
    ret = rtt_be_updateNodes(topo,
        &selnode, RTT_COL_NODE_NODE_ID,
        &updnode, RTT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  int newface1 = -1;

  if ( ! modFace )
  {
    newface1 = _rtt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 0 );
    if ( newface1 == 0 ) {
      RTDEBUG(1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }
  }

  /* Check face splitting */
  int newface = _rtt_AddFaceSplit( topo, newedge.edge_id,
                                   newedge.face_left, 0 );
  if ( modFace )
  {
    if ( newface == 0 ) {
      RTDEBUG(1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }

    if ( newface < 0 )
    {
      /* face on the left is the universe face */
      /* must be forming a maximal ring in universal face */
      newface = _rtt_AddFaceSplit( topo, -newedge.edge_id,
                                   newedge.face_left, 0 );
      if ( newface < 0 ) return newedge.edge_id; /* no split */
    }
    else
    {
      _rtt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 1 );
    }
  }

  /*
   * Update topogeometries, if needed
   */
  if ( newedge.face_left != 0 )
  {
    ret = rtt_be_updateTopoGeomFaceSplit(topo, newedge.face_left,
                                         newface, newface1);
    if ( ret == 0 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    if ( ! modFace )
    {
      /* drop old face from the face table */
      ret = rtt_be_deleteFacesById(topo, &(newedge.face_left), 1);
      if ( ret == -1 ) {
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }
  }

  return newedge.edge_id;
}

RTT_ELEMID
rtt_AddEdgeModFace( RTT_TOPOLOGY* topo,
                    RTT_ELEMID start_node, RTT_ELEMID end_node,
                    RTLINE *geom, int skipChecks )
{
  return _rtt_AddEdge( topo, start_node, end_node, geom, skipChecks, 1 );
}

RTT_ELEMID
rtt_AddEdgeNewFaces( RTT_TOPOLOGY* topo,
                    RTT_ELEMID start_node, RTT_ELEMID end_node,
                    RTLINE *geom, int skipChecks )
{
  return _rtt_AddEdge( topo, start_node, end_node, geom, skipChecks, 0 );
}

static RTGEOM *
_rtt_FaceByEdges(RTT_TOPOLOGY *topo, RTT_ISO_EDGE *edges, int numfaceedges)
{
  RTGEOM *outg;
  RTCOLLECTION *bounds;
  RTGEOM **geoms = rtalloc( sizeof(RTGEOM*) * numfaceedges );
  int i, validedges = 0;

  for ( i=0; i<numfaceedges; ++i )
  {
    /* NOTE: skipping edges with same face on both sides, although
     *       correct, results in a failure to build faces from
     *       invalid topologies as expected by legacy tests.
     * TODO: update legacy tests expectances/unleash this skipping ?
     */
    /* if ( edges[i].face_left == edges[i].face_right ) continue; */
    geoms[validedges++] = rtline_as_rtgeom(edges[i].geom);
  }
  if ( ! validedges )
  {
    /* Face has no valid boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    if ( numfaceedges ) rtfree(geoms);
    RTDEBUG(1, "_rtt_FaceByEdges returning empty polygon");
    return rtpoly_as_rtgeom(
            rtpoly_construct_empty(topo->srid, topo->hasZ, 0)
           );
  }
  bounds = rtcollection_construct(RTMULTILINETYPE,
                                  topo->srid,
                                  NULL, /* gbox */
                                  validedges,
                                  geoms);
  outg = rtgeom_buildarea( rtcollection_as_rtgeom(bounds) );
  rtcollection_release(bounds);
  rtfree(geoms);
#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt(outg, RTWKT_EXTENDED, 2, &sz);
  RTDEBUGF(1, "_rtt_FaceByEdges returning area: %s", wkt);
  rtfree(wkt);
  }
#endif
  return outg;
}

RTGEOM*
rtt_GetFaceGeometry(RTT_TOPOLOGY* topo, RTT_ELEMID faceid)
{
  int numfaceedges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_FACE *face;
  RTPOLY *out;
  RTGEOM *outg;
  int i;
  int fields;

  if ( faceid == 0 )
  {
    rterror("SQL/MM Spatial exception - universal face has no geometry");
    return NULL;
  }

  /* Construct the face geometry */
  numfaceedges = 1;
  fields = RTT_COL_EDGE_GEOM |
           RTT_COL_EDGE_FACE_LEFT |
           RTT_COL_EDGE_FACE_RIGHT
           ;
  edges = rtt_be_getEdgeByFace( topo, &faceid, &numfaceedges, fields, NULL );
  if ( numfaceedges == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }

  if ( numfaceedges == 0 )
  {
    i = 1;
    face = rtt_be_getFaceById(topo, &faceid, &i, RTT_COL_FACE_FACE_ID);
    if ( i == -1 ) {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    if ( i == 0 ) {
      rterror("SQL/MM Spatial exception - non-existent face.");
      return NULL;
    }
    rtfree( face );
    if ( i > 1 ) {
      rterror("Corrupted topology: multiple face records have face_id=%"
              PRId64, faceid);
      return NULL;
    }
    /* Face has no boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    out = rtpoly_construct_empty(topo->srid, topo->hasZ, 0);
    return rtpoly_as_rtgeom(out);
  }

  outg = _rtt_FaceByEdges( topo, edges, numfaceedges );
  _rtt_release_edges(edges, numfaceedges);

  return outg;
}

/* Find which edge from the "edges" set defines the next
 * portion of the given "ring".
 *
 * The edge might be either forward or backward.
 *
 * @param ring The ring to find definition of.
 *             It is assumed it does not contain duplicated vertices.
 * @param from offset of the ring point to start looking from
 * @param edges array of edges to search into
 * @param numedges number of edges in the edges array
 *
 * @return index of the edge defining the next ring portion or
 *               -1 if no edge was found to be part of the ring
 */
static int
_rtt_FindNextRingEdge(const POINTARRAY *ring, int from,
                      const RTT_ISO_EDGE *edges, int numedges)
{
  int i;
  POINT2D p1;

  /* Get starting ring point */
  getPoint2d_p(ring, from, &p1);

  RTDEBUGF(1, "Ring's 'from' point (%d) is %g,%g", from, p1.x, p1.y);

  /* find the edges defining the next portion of ring starting from
   * vertex "from" */
  for ( i=0; i<numedges; ++i )
  {
    const RTT_ISO_EDGE *isoe = &(edges[i]);
    RTLINE *edge = isoe->geom;
    POINTARRAY *epa = edge->points;
    POINT2D p2, pt;
    int match = 0;
    int j;

    /* Skip if the edge is a dangling one */
    if ( isoe->face_left == isoe->face_right )
    {
      RTDEBUGF(3, "_rtt_FindNextRingEdge: edge %" RTTFMT_ELEMID
                  " has same face (%" RTTFMT_ELEMID
                  ") on both sides, skipping",
                  isoe->edge_id, isoe->face_left);
      continue;
    }

#if 0
    size_t sz;
    RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " is %s",
                isoe->edge_id,
                rtgeom_to_wkt(rtline_as_rtgeom(edge), RTWKT_EXTENDED, 2, &sz));
#endif

    /* ptarray_remove_repeated_points ? */

    getPoint2d_p(epa, 0, &p2);
    RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " 'first' point is %g,%g",
                isoe->edge_id, p2.x, p2.y);
    RTDEBUGF(1, "Rings's 'from' point is still %g,%g", p1.x, p1.y);
    if ( p2d_same(&p1, &p2) )
    {
      RTDEBUG(1, "p2d_same(p1,p2) returned true");
      RTDEBUGF(1, "First point of edge %" RTTFMT_ELEMID
                  " matches ring vertex %d", isoe->edge_id, from);
      /* first point matches, let's check next non-equal one */
      for ( j=1; j<epa->npoints; ++j )
      {
        getPoint2d_p(epa, j, &p2);
        RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " 'next' point %d is %g,%g",
                    isoe->edge_id, j, p2.x, p2.y);
        /* we won't check duplicated edge points */
        if ( p2d_same(&p1, &p2) ) continue;
        /* we assume there are no duplicated points in ring */
        getPoint2d_p(ring, from+1, &pt);
        RTDEBUGF(1, "Ring's point %d is %g,%g",
                    from+1, pt.x, pt.y);
        match = p2d_same(&pt, &p2);
        break; /* we want to check a single non-equal next vertex */
      }
#if RTGEOM_DEBUG_LEVEL > 0
      if ( match ) {
        RTDEBUGF(1, "Prev point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        RTDEBUGF(1, "Prev point of edge %" RTTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( ! match )
    {
      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " did not match as forward",
                 isoe->edge_id);
      getPoint2d_p(epa, epa->npoints-1, &p2);
      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " 'last' point is %g,%g",
                  isoe->edge_id, p2.x, p2.y);
      if ( p2d_same(&p1, &p2) )
      {
        RTDEBUGF(1, "Last point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from);
        /* last point matches, let's check next non-equal one */
        for ( j=epa->npoints-2; j>=0; --j )
        {
          getPoint2d_p(epa, j, &p2);
          RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " 'prev' point %d is %g,%g",
                      isoe->edge_id, j, p2.x, p2.y);
          /* we won't check duplicated edge points */
          if ( p2d_same(&p1, &p2) ) continue;
          /* we assume there are no duplicated points in ring */
          getPoint2d_p(ring, from+1, &pt);
          RTDEBUGF(1, "Ring's point %d is %g,%g",
                      from+1, pt.x, pt.y);
          match = p2d_same(&pt, &p2);
          break; /* we want to check a single non-equal next vertex */
        }
      }
#if RTGEOM_DEBUG_LEVEL > 0
      if ( match ) {
        RTDEBUGF(1, "Prev point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        RTDEBUGF(1, "Prev point of edge %" RTTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( match ) return i;

  }

  return -1;
}

/* Reverse values in array between "from" (inclusive)
 * and "to" (exclusive) indexes */
static void
_rtt_ReverseElemidArray(RTT_ELEMID *ary, int from, int to)
{
  RTT_ELEMID t;
  while (from < to)
  {
    t = ary[from];
    ary[from++] = ary[to];
    ary[to--] = t;
  }
}

/* Rotate values in array between "from" (inclusive)
 * and "to" (exclusive) indexes, so that "rotidx" is
 * the new value at "from" */
static void
_rtt_RotateElemidArray(RTT_ELEMID *ary, int from, int to, int rotidx)
{
  _rtt_ReverseElemidArray(ary, from, rotidx-1);
  _rtt_ReverseElemidArray(ary, rotidx, to-1);
  _rtt_ReverseElemidArray(ary, from, to-1);
}


int
rtt_GetFaceEdges(RTT_TOPOLOGY* topo, RTT_ELEMID face_id, RTT_ELEMID **out )
{
  RTGEOM *face;
  RTPOLY *facepoly;
  RTT_ISO_EDGE *edges;
  int numfaceedges;
  int fields, i;
  int nseid = 0; /* number of signed edge ids */
  int prevseid;
  RTT_ELEMID *seid; /* signed edge ids */

  /* Get list of face edges */
  numfaceedges = 1;
  fields = RTT_COL_EDGE_EDGE_ID |
           RTT_COL_EDGE_GEOM |
           RTT_COL_EDGE_FACE_LEFT |
           RTT_COL_EDGE_FACE_RIGHT
           ;
  edges = rtt_be_getEdgeByFace( topo, &face_id, &numfaceedges, fields, NULL );
  if ( numfaceedges == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! numfaceedges ) return 0; /* no edges in output */

  /* order edges by occurrence in face */

  face = _rtt_FaceByEdges(topo, edges, numfaceedges);
  if ( ! face )
  {
    /* _rtt_FaceByEdges should have already invoked rterror in this case */
    _rtt_release_edges(edges, numfaceedges);
    return -1;
  }

  if ( rtgeom_is_empty(face) )
  {
    /* no edges in output */
    _rtt_release_edges(edges, numfaceedges);
    rtgeom_free(face);
    return 0;
  }

  /* force_lhr, if the face is not the universe */
  /* _rtt_FaceByEdges seems to guaranteed RHR */
  /* rtgeom_force_clockwise(face); */
  if ( face_id ) rtgeom_reverse(face);

#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt(face, RTWKT_EXTENDED, 6, &sz);
  RTDEBUGF(1, "Geometry of face %" RTTFMT_ELEMID " is: %s",
              face_id, wkt);
  rtfree(wkt);
  }
#endif

  facepoly = rtgeom_as_rtpoly(face);
  if ( ! facepoly )
  {
    _rtt_release_edges(edges, numfaceedges);
    rtgeom_free(face);
    rterror("Geometry of face %" RTTFMT_ELEMID " is not a polygon", face_id);
    return -1;
  }

  nseid = prevseid = 0;
  seid = rtalloc( sizeof(RTT_ELEMID) * numfaceedges );

  /* for each ring of the face polygon... */
  for ( i=0; i<facepoly->nrings; ++i )
  {
    const POINTARRAY *ring = facepoly->rings[i];
    int j = 0;
    RTT_ISO_EDGE *nextedge;
    RTLINE *nextline;

    RTDEBUGF(1, "Ring %d has %d points", i, ring->npoints);

    while ( j < ring->npoints-1 )
    {
      RTDEBUGF(1, "Looking for edge covering ring %d from vertex %d",
                  i, j);

      int edgeno = _rtt_FindNextRingEdge(ring, j, edges, numfaceedges);
      if ( edgeno == -1 )
      {
        /* should never happen */
        _rtt_release_edges(edges, numfaceedges);
        rtgeom_free(face);
        rtfree(seid);
        rterror("No edge (among %d) found to be defining geometry of face %"
                RTTFMT_ELEMID, numfaceedges, face_id);
        return -1;
      }

      nextedge = &(edges[edgeno]);
      nextline = nextedge->geom;

      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID
                  " covers ring %d from vertex %d to %d",
                  nextedge->edge_id, i, j, j + nextline->points->npoints - 1);

#if 0
      {
      size_t sz;
      char *wkt = rtgeom_to_wkt(rtline_as_rtgeom(nextline), RTWKT_EXTENDED, 6, &sz);
      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " is %s",
                  nextedge->edge_id, wkt);
      rtfree(wkt);
      }
#endif

      j += nextline->points->npoints - 1;

      /* Add next edge to the output array */
      seid[nseid++] = nextedge->face_left == face_id ?
                          nextedge->edge_id :
                         -nextedge->edge_id;

      /* avoid checking again on next time turn */
      nextedge->face_left = nextedge->face_right = -1;
    }

    /* now "scroll" the list of edges so that the one
     * with smaller absolute edge_id is first */
    /* Range is: [prevseid, nseid) -- [inclusive, exclusive) */
    if ( (nseid - prevseid) > 1 )
    {{
      RTT_ELEMID minid = 0;
      int minidx = 0;
      RTDEBUGF(1, "Looking for smallest id among the %d edges "
                  "composing ring %d", (nseid-prevseid), i);
      for ( j=prevseid; j<nseid; ++j )
      {
        RTT_ELEMID id = llabs(seid[j]);
        RTDEBUGF(1, "Abs id of edge in pos %d is %" RTTFMT_ELEMID, j, id);
        if ( ! minid || id < minid )
        {
          minid = id;
          minidx = j;
        }
      }
      RTDEBUGF(1, "Smallest id is %" RTTFMT_ELEMID
                  " at position %d", minid, minidx);
      if ( minidx != prevseid )
      {
        _rtt_RotateElemidArray(seid, prevseid, nseid, minidx);
      }
    }}

    prevseid = nseid;
  }

  rtgeom_free(face);
  _rtt_release_edges(edges, numfaceedges);

  *out = seid;
  return nseid;
}

static GEOSGeometry *
_rtt_EdgeMotionArea(RTLINE *geom, int isclosed)
{
  GEOSGeometry *gg;
  POINT4D p4d;
  POINTARRAY *pa;
  POINTARRAY **pas;
  RTPOLY *poly;
  RTGEOM *g;

  pas = rtalloc(sizeof(POINTARRAY*));

  initGEOS(rtnotice, rtgeom_geos_error);

  if ( isclosed )
  {
    pas[0] = ptarray_clone_deep( geom->points );
    poly = rtpoly_construct(0, 0, 1, pas);
    gg = RTGEOM2GEOS( rtpoly_as_rtgeom(poly), 0 );
    rtpoly_free(poly); /* should also delete the pointarrays */
  }
  else
  {
    pa = geom->points;
    getPoint4d_p(pa, 0, &p4d);
    pas[0] = ptarray_clone_deep( pa );
    /* don't bother dup check */
    if ( RT_FAILURE == ptarray_append_point(pas[0], &p4d, RT_TRUE) )
    {
      ptarray_free(pas[0]);
      rtfree(pas);
      rterror("Could not append point to pointarray");
      return NULL;
    }
    poly = rtpoly_construct(0, NULL, 1, pas);
    /* make valid, in case the edge self-intersects on its first-last
     * vertex segment */
    g = rtgeom_make_valid(rtpoly_as_rtgeom(poly));
    rtpoly_free(poly); /* should also delete the pointarrays */
    if ( ! g )
    {
      rterror("Could not make edge motion area valid");
      return NULL;
    }
    gg = RTGEOM2GEOS(g, 0);
    rtgeom_free(g);
  }
  if ( ! gg )
  {
    rterror("Could not convert old edge area geometry to GEOS: %s",
            rtgeom_geos_errmsg);
    return NULL;
  }
  return gg;
}

int
rtt_ChangeEdgeGeom(RTT_TOPOLOGY* topo, RTT_ELEMID edge_id, RTLINE *geom)
{
  RTT_ISO_EDGE *oldedge;
  RTT_ISO_EDGE newedge;
  POINT2D p1, p2, pt;
  int i;
  int isclosed = 0;

  /* curve must be simple */
  if ( ! rtgeom_is_simple(rtline_as_rtgeom(geom)) )
  {
    rterror("SQL/MM Spatial exception - curve not simple");
    return -1;
  }

  i = 1;
  oldedge = rtt_be_getEdgeById(topo, &edge_id, &i, RTT_COL_EDGE_ALL);
  if ( ! oldedge )
  {
    RTDEBUGF(1, "rtt_ChangeEdgeGeom: "
                "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i == 0 )
    {
      rterror("SQL/MM Spatial exception - non-existent edge %"
              RTTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      rterror("Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return -1;
    }
  }

  RTDEBUGF(1, "rtt_ChangeEdgeGeom: "
              "old edge has %d points, new edge has %d points",
              oldedge->geom->points->npoints, geom->points->npoints);

  /*
   * e) Check StartPoint consistency
   */
  getPoint2d_p(oldedge->geom->points, 0, &p1);
  getPoint2d_p(geom->points, 0, &pt);
  if ( ! p2d_same(&p1, &pt) )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("SQL/MM Spatial exception - "
            "start node not geometry start point.");
    return -1;
  }

  /*
   * f) Check EndPoint consistency
   */
  if ( oldedge->geom->points->npoints < 2 )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("Corrupted topology: edge %" RTTFMT_ELEMID
            " has less than 2 vertices", oldedge->edge_id);
    return -1;
  }
  getPoint2d_p(oldedge->geom->points, oldedge->geom->points->npoints-1, &p2);
  if ( geom->points->npoints < 2 )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("Invalid edge: less than 2 vertices");
    return -1;
  }
  getPoint2d_p(geom->points, geom->points->npoints-1, &pt);
  if ( ! p2d_same(&pt, &p2) )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("SQL/MM Spatial exception - "
            "end node not geometry end point.");
    return -1;
  }

  /* Not in the specs:
   * if the edge is closed, check we didn't change winding !
   *       (should be part of isomorphism checking)
   */
  if ( oldedge->start_node == oldedge->end_node )
  {
    isclosed = 1;
#if 1 /* TODO: this is actually bogus as a test */
    /* check for valid edge (distinct vertices must exist) */
    if ( ! _rtt_GetInteriorEdgePoint(geom, &pt) )
    {
      _rtt_release_edges(oldedge, 1);
      rterror("Invalid edge (no two distinct vertices exist)");
      return -1;
    }
#endif

    if ( ptarray_isccw(oldedge->geom->points) !=
         ptarray_isccw(geom->points) )
    {
      _rtt_release_edges(oldedge, 1);
      rterror("Edge twist at node POINT(%g %g)", p1.x, p1.y);
      return -1;
    }
  }

  if ( _rtt_CheckEdgeCrossing(topo, oldedge->start_node,
                                    oldedge->end_node, geom, edge_id ) )
  {
    /* would have called rterror already, leaking :( */
    _rtt_release_edges(oldedge, 1);
    return -1;
  }

  RTDEBUG(1, "rtt_ChangeEdgeGeom: "
             "edge crossing check passed ");

  /*
   * Not in the specs:
   * Check topological isomorphism
   */

  /* Check that the "motion range" doesn't include any node */
  // 1. compute combined bbox of old and new edge
  GBOX mbox; /* motion box */
  rtgeom_add_bbox((RTGEOM*)oldedge->geom); /* just in case */
  rtgeom_add_bbox((RTGEOM*)geom); /* just in case */
  gbox_union(oldedge->geom->bbox, geom->bbox, &mbox);
  // 2. fetch all nodes in the combined box
  RTT_ISO_NODE *nodes;
  int numnodes;
  nodes = rtt_be_getNodeWithinBox2D(topo, &mbox, &numnodes,
                                          RTT_COL_NODE_ALL, 0);
  RTDEBUGF(1, "rtt_be_getNodeWithinBox2D returned %d nodes", numnodes);
  if ( numnodes == -1 ) {
    _rtt_release_edges(oldedge, 1);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  // 3. if any node beside endnodes are found:
  if ( numnodes > ( 1 + isclosed ? 0 : 1 ) )
  {{
    GEOSGeometry *oarea, *narea;
    const GEOSPreparedGeometry *oareap, *nareap;

    initGEOS(rtnotice, rtgeom_geos_error);

    oarea = _rtt_EdgeMotionArea(oldedge->geom, isclosed);
    if ( ! oarea )
    {
      _rtt_release_edges(oldedge, 1);
      rterror("Could not compute edge motion area for old edge");
      return -1;
    }

    narea = _rtt_EdgeMotionArea(geom, isclosed);
    if ( ! narea )
    {
      GEOSGeom_destroy(oarea);
      _rtt_release_edges(oldedge, 1);
      rterror("Could not compute edge motion area for new edge");
      return -1;
    }

    //   3.2. bail out if any node is in one and not the other
    oareap = GEOSPrepare( oarea );
    nareap = GEOSPrepare( narea );
    for (i=0; i<numnodes; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      GEOSGeometry *ngg;
      int ocont, ncont;
      size_t sz;
      char *wkt;
      if ( n->node_id == oldedge->start_node ) continue;
      if ( n->node_id == oldedge->end_node ) continue;
      ngg = RTGEOM2GEOS( rtpoint_as_rtgeom(n->geom) , 0);
      ocont = GEOSPreparedContains( oareap, ngg );
      ncont = GEOSPreparedContains( nareap, ngg );
      GEOSGeom_destroy(ngg);
      if (ocont == 2 || ncont == 2)
      {
        _rtt_release_nodes(nodes, numnodes);
        GEOSPreparedGeom_destroy(oareap);
        GEOSGeom_destroy(oarea);
        GEOSPreparedGeom_destroy(nareap);
        GEOSGeom_destroy(narea);
        rterror("GEOS exception on PreparedContains: %s", rtgeom_geos_errmsg);
        return -1;
      }
      if (ocont != ncont)
      {
        GEOSPreparedGeom_destroy(oareap);
        GEOSGeom_destroy(oarea);
        GEOSPreparedGeom_destroy(nareap);
        GEOSGeom_destroy(narea);
        wkt = rtgeom_to_wkt(rtpoint_as_rtgeom(n->geom), RTWKT_ISO, 6, &sz);
        _rtt_release_nodes(nodes, numnodes);
        rterror("Edge motion collision at %s", wkt);
        rtfree(wkt); /* would not necessarely reach this point */
        return -1;
      }
    }
    GEOSPreparedGeom_destroy(oareap);
    GEOSGeom_destroy(oarea);
    GEOSPreparedGeom_destroy(nareap);
    GEOSGeom_destroy(narea);
  }}
  if ( numnodes ) _rtt_release_nodes(nodes, numnodes);

  RTDEBUG(1, "nodes containment check passed");

  /*
   * Check edge adjacency before
   * TODO: can be optimized to gather azimuths of all edge ends once
   */

  edgeend span_pre, epan_pre;
  /* initialize span_pre.myaz and epan_pre.myaz with existing edge */
  i = _rtt_InitEdgeEndByLine(&span_pre, &epan_pre,
                             oldedge->geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  _rtt_FindAdjacentEdges( topo, oldedge->start_node, &span_pre,
                                  isclosed ? &epan_pre : NULL, edge_id );
  _rtt_FindAdjacentEdges( topo, oldedge->end_node, &epan_pre,
                                  isclosed ? &span_pre : NULL, edge_id );

  RTDEBUGF(1, "edges adjacent to old edge are %" RTTFMT_ELEMID
              " and % (first point), %" RTTFMT_ELEMID
              " and % (last point)" RTTFMT_ELEMID,
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);

  /* update edge geometry */
  newedge.edge_id = edge_id;
  newedge.geom = geom;
  i = rtt_be_updateEdgesById(topo, &newedge, 1, RTT_COL_EDGE_GEOM);
  if ( i == -1 )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! i )
  {
    _rtt_release_edges(oldedge, 1);
    rterror("Unexpected error: %d edges updated when expecting 1", i);
    return -1;
  }

  /*
   * Check edge adjacency after
   */
  edgeend span_post, epan_post;
  i = _rtt_InitEdgeEndByLine(&span_post, &epan_post, geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  /* initialize epan_post.myaz and epan_post.myaz */
  i = _rtt_InitEdgeEndByLine(&span_post, &epan_post,
                             geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  _rtt_FindAdjacentEdges( topo, oldedge->start_node, &span_post,
                          isclosed ? &epan_post : NULL, edge_id );
  _rtt_FindAdjacentEdges( topo, oldedge->end_node, &epan_post,
                          isclosed ? &span_post : NULL, edge_id );

  RTDEBUGF(1, "edges adjacent to new edge are %" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID
              " (first point), %" RTTFMT_ELEMID
              " and % (last point)" RTTFMT_ELEMID,
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);


  /* Bail out if next CW or CCW edge on start node changed */
  if ( span_pre.nextCW != span_post.nextCW ||
       span_pre.nextCCW != span_post.nextCCW )
  {{
    RTT_ELEMID nid = oldedge->start_node;
    _rtt_release_edges(oldedge, 1);
    rterror("Edge changed disposition around start node %"
            RTTFMT_ELEMID, nid);
    return -1;
  }}

  /* Bail out if next CW or CCW edge on end node changed */
  if ( epan_pre.nextCW != epan_post.nextCW ||
       epan_pre.nextCCW != epan_post.nextCCW )
  {{
    RTT_ELEMID nid = oldedge->end_node;
    _rtt_release_edges(oldedge, 1);
    rterror("Edge changed disposition around end node %"
            RTTFMT_ELEMID, nid);
    return -1;
  }}

  /*
  -- Update faces MBR of left and right faces
  -- TODO: think about ways to optimize this part, like see if
  --       the old edge geometry partecipated in the definition
  --       of the current MBR (for shrinking) or the new edge MBR
  --       would be larger than the old face MBR...
  --
  */
  int facestoupdate = 0;
  RTT_ISO_FACE faces[2];
  RTGEOM *nface1 = NULL;
  RTGEOM *nface2 = NULL;
  if ( oldedge->face_left != 0 )
  {
    nface1 = rtt_GetFaceGeometry(topo, oldedge->face_left);
#if 0
    {
    size_t sz;
    char *wkt = rtgeom_to_wkt(nface1, RTWKT_EXTENDED, 2, &sz);
    RTDEBUGF(1, "new geometry of face left (%d): %s", (int)oldedge->face_left, wkt);
    rtfree(wkt);
    }
#endif
    rtgeom_add_bbox(nface1);
    faces[facestoupdate].face_id = oldedge->face_left;
    /* ownership left to nface */
    faces[facestoupdate++].mbr = nface1->bbox;
  }
  if ( oldedge->face_right != 0
       /* no need to update twice the same face.. */
       && oldedge->face_right != oldedge->face_left )
  {
    nface2 = rtt_GetFaceGeometry(topo, oldedge->face_right);
#if 0
    {
    size_t sz;
    char *wkt = rtgeom_to_wkt(nface2, RTWKT_EXTENDED, 2, &sz);
    RTDEBUGF(1, "new geometry of face right (%d): %s", (int)oldedge->face_right, wkt);
    rtfree(wkt);
    }
#endif
    rtgeom_add_bbox(nface2);
    faces[facestoupdate].face_id = oldedge->face_right;
    faces[facestoupdate++].mbr = nface2->bbox; /* ownership left to nface */
  }
  RTDEBUGF(1, "%d faces to update", facestoupdate);
  if ( facestoupdate )
  {
    i = rtt_be_updateFacesById( topo, &(faces[0]), facestoupdate );
    if ( i != facestoupdate )
    {
      if ( nface1 ) rtgeom_free(nface1);
      if ( nface2 ) rtgeom_free(nface2);
      _rtt_release_edges(oldedge, 1);
      if ( i == -1 )
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      else
        rterror("Unexpected error: %d faces found when expecting 1", i);
      return -1;
    }
  }
  if ( nface1 ) rtgeom_free(nface1);
  if ( nface2 ) rtgeom_free(nface2);

  RTDEBUG(1, "all done, cleaning up edges");

  _rtt_release_edges(oldedge, 1);
  return 0; /* success */
}

/* Only return CONTAINING_FACE in the node object */
static RTT_ISO_NODE *
_rtt_GetIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid)
{
  RTT_ISO_NODE *node;
  int n = 1;

  node = rtt_be_getNodeById( topo, &nid, &n, RTT_COL_NODE_CONTAINING_FACE );
  if ( n < 0 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  if ( n < 1 ) {
    rterror("SQL/MM Spatial exception - non-existent node");
    return 0;
  }
  if ( node->containing_face == -1 )
  {
    rtfree(node);
    rterror("SQL/MM Spatial exception - not isolated node");
    return 0;
  }

  return node;
}

int
rtt_MoveIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid, RTPOINT *pt)
{
  RTT_ISO_NODE *node;
  int ret;

  node = _rtt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  if ( rtt_be_ExistsCoincidentNode(topo, pt) )
  {
    rtfree(node);
    rterror("SQL/MM Spatial exception - coincident node");
    return -1;
  }

  if ( rtt_be_ExistsEdgeIntersectingPoint(topo, pt) )
  {
    rtfree(node);
    rterror("SQL/MM Spatial exception - edge crosses node.");
    return -1;
  }

  /* TODO: check that the new point is in the same containing face !
   * See https://trac.osgeo.org/postgis/ticket/3232
   */

  node->node_id = nid;
  node->geom = pt;
  ret = rtt_be_updateNodesById(topo, node, 1,
                               RTT_COL_NODE_GEOM);
  if ( ret == -1 ) {
    rtfree(node);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  rtfree(node);
  return 0;
}

int
rtt_RemoveIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid)
{
  RTT_ISO_NODE *node;
  int n = 1;

  node = _rtt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  n = rtt_be_deleteNodesById( topo, &nid, n );
  if ( n == -1 )
  {
    rtfree(node);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( n != 1 )
  {
    rtfree(node);
    rterror("Unexpected error: %d nodes deleted when expecting 1", n);
    return -1;
  }

  /* TODO: notify to caller about node being removed ?
   * See https://trac.osgeo.org/postgis/ticket/3231
   */

  rtfree(node);
  return 0; /* success */
}

int
rtt_RemIsoEdge(RTT_TOPOLOGY* topo, RTT_ELEMID id)
{
  RTT_ISO_EDGE deledge;
  RTT_ISO_EDGE *edge;
  RTT_ELEMID nid[2];
  RTT_ISO_NODE upd_node[2];
  RTT_ELEMID containing_face;
  int n = 1;
  int i;

  edge = rtt_be_getEdgeById( topo, &id, &n, RTT_COL_EDGE_START_NODE|
                                            RTT_COL_EDGE_END_NODE |
                                            RTT_COL_EDGE_FACE_LEFT |
                                            RTT_COL_EDGE_FACE_RIGHT );
  if ( ! edge )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! n )
  {
    rterror("SQL/MM Spatial exception - non-existent edge");
    return -1;
  }
  if ( n > 1 )
  {
    rtfree(edge);
    rterror("Corrupted topology: more than a single edge have id %"
            RTTFMT_ELEMID, id);
    return -1;
  }

  if ( edge[0].face_left != edge[0].face_right )
  {
    rtfree(edge);
    rterror("SQL/MM Spatial exception - not isolated edge");
    return -1;
  }
  containing_face = edge[0].face_left;

  nid[0] = edge[0].start_node;
  nid[1] = edge[0].end_node;
  rtfree(edge);

  n = 2;
  edge = rtt_be_getEdgeByNode( topo, nid, &n, RTT_COL_EDGE_EDGE_ID );
  if ( n == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<n; ++i )
  {
    if ( edge[i].edge_id == id ) continue;
    rtfree(edge);
    rterror("SQL/MM Spatial exception - not isolated edge");
    return -1;
  }
  if ( edge ) rtfree(edge);

  deledge.edge_id = id;
  n = rtt_be_deleteEdges( topo, &deledge, RTT_COL_EDGE_EDGE_ID );
  if ( n == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( n != 1 )
  {
    rterror("Unexpected error: %d edges deleted when expecting 1", n);
    return -1;
  }

  upd_node[0].node_id = nid[0];
  upd_node[0].containing_face = containing_face;
  n = 1;
  if ( nid[1] != nid[0] ) {
    upd_node[1].node_id = nid[1];
    upd_node[1].containing_face = containing_face;
    ++n;
  }
  n = rtt_be_updateNodesById(topo, upd_node, n,
                               RTT_COL_NODE_CONTAINING_FACE);
  if ( n == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* TODO: notify to caller about edge being removed ?
   * See https://trac.osgeo.org/postgis/ticket/3248
   */

  return 0; /* success */
}

/* Used by _rtt_RemEdge to update edge face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_rtt_UpdateEdgeFaceRef( RTT_TOPOLOGY *topo, RTT_ELEMID of, RTT_ELEMID nf)
{
  RTT_ISO_EDGE sel_edge, upd_edge;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel_edge.face_left = of;
  upd_edge.face_left = nf;
  ret = rtt_be_updateEdges(topo, &sel_edge, RTT_COL_EDGE_FACE_LEFT,
                                 &upd_edge, RTT_COL_EDGE_FACE_LEFT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  /* Update face_right for all edges still referencing old face */
  sel_edge.face_right = of;
  upd_edge.face_right = nf;
  ret = rtt_be_updateEdges(topo, &sel_edge, RTT_COL_EDGE_FACE_RIGHT,
                                 &upd_edge, RTT_COL_EDGE_FACE_RIGHT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by _rtt_RemEdge to update node face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_rtt_UpdateNodeFaceRef( RTT_TOPOLOGY *topo, RTT_ELEMID of, RTT_ELEMID nf)
{
  RTT_ISO_NODE sel, upd;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel.containing_face = of;
  upd.containing_face = nf;
  ret = rtt_be_updateNodes(topo, &sel, RTT_COL_NODE_CONTAINING_FACE,
                                 &upd, RTT_COL_NODE_CONTAINING_FACE,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by rtt_RemEdgeModFace and rtt_RemEdgeNewFaces
 *
 * Returns -1 on error, identifier of the face that takes up the space
 * previously occupied by the removed edge if modFace is 1, identifier of
 * the created face (0 if none) if modFace is 0.
 */
static RTT_ELEMID
_rtt_RemEdge( RTT_TOPOLOGY* topo, RTT_ELEMID edge_id, int modFace )
{
  int i, nedges, nfaces, fields;
  RTT_ISO_EDGE *edge = NULL;
  RTT_ISO_EDGE *upd_edge = NULL;
  RTT_ISO_EDGE upd_edge_left[2];
  int nedge_left = 0;
  RTT_ISO_EDGE upd_edge_right[2];
  int nedge_right = 0;
  RTT_ISO_NODE upd_node[2];
  int nnode = 0;
  RTT_ISO_FACE *faces = NULL;
  RTT_ISO_FACE newface;
  RTT_ELEMID node_ids[2];
  RTT_ELEMID face_ids[2];
  int fnode_edges = 0; /* number of edges on the first node (excluded
                        * the one being removed ) */
  int lnode_edges = 0; /* number of edges on the last node (excluded
                        * the one being removed ) */

  newface.face_id = 0;

  i = 1;
  edge = rtt_be_getEdgeById(topo, &edge_id, &i, RTT_COL_EDGE_ALL);
  if ( ! edge )
  {
    RTDEBUGF(1, "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i == 0 )
    {
      rterror("SQL/MM Spatial exception - non-existent edge %"
              RTTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      rterror("Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return -1;
    }
  }

  if ( ! rtt_be_checkTopoGeomRemEdge(topo, edge_id,
                                     edge->face_left, edge->face_right) )
  {
    rterror("%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  RTDEBUG(1, "Updating next_{right,left}_face of ring edges...");

  /* Update edge linking */

  nedges = 0;
  node_ids[nedges++] = edge->start_node;
  if ( edge->end_node != edge->start_node )
  {
    node_ids[nedges++] = edge->end_node;
  }
  fields = RTT_COL_EDGE_EDGE_ID | RTT_COL_EDGE_START_NODE |
           RTT_COL_EDGE_END_NODE | RTT_COL_EDGE_NEXT_LEFT |
           RTT_COL_EDGE_NEXT_RIGHT;
  upd_edge = rtt_be_getEdgeByNode( topo, &(node_ids[0]), &nedges, fields );
  if ( nedges == -1 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  nedge_left = nedge_right = 0;
  for ( i=0; i<nedges; ++i )
  {
    RTT_ISO_EDGE *e = &(upd_edge[i]);
    if ( e->edge_id == edge_id ) continue;
    if ( e->start_node == edge->start_node || e->end_node == edge->start_node )
    {
      ++fnode_edges;
    }
    if ( e->start_node == edge->end_node || e->end_node == edge->end_node )
    {
      ++lnode_edges;
    }
    if ( e->next_left == -edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_left == edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }

    if ( e->next_right == -edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_right == edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }
  }

  if ( nedge_left )
  {
    RTDEBUGF(1, "updating %d 'next_left' edges", nedge_left);
    /* update edges in upd_edge_left set next_left */
    i = rtt_be_updateEdgesById(topo, &(upd_edge_left[0]), nedge_left,
                               RTT_COL_EDGE_NEXT_LEFT);
    if ( i == -1 )
    {
      _rtt_release_edges(edge, 1);
      rtfree(upd_edge);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  if ( nedge_right )
  {
    RTDEBUGF(1, "updating %d 'next_right' edges", nedge_right);
    /* update edges in upd_edge_right set next_right */
    i = rtt_be_updateEdgesById(topo, &(upd_edge_right[0]), nedge_right,
                               RTT_COL_EDGE_NEXT_RIGHT);
    if ( i == -1 )
    {
      _rtt_release_edges(edge, 1);
      rtfree(upd_edge);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  RTDEBUGF(1, "releasing %d updateable edges in %p", nedges, upd_edge);
  rtfree(upd_edge);

  /* Id of face that will take up all the space previously
   * taken by left and right faces of the edge */
  RTT_ELEMID floodface;

  /* Find floodface, and update its mbr if != 0 */
  if ( edge->face_left == edge->face_right )
  {
    floodface = edge->face_right;
  }
  else
  {
    /* Two faces healed */
    if ( edge->face_left == 0 || edge->face_right == 0 )
    {
      floodface = 0;
      RTDEBUG(1, "floodface is universe");
    }
    else
    {
      /* we choose right face as the face that will remain
       * to be symmetric with ST_AddEdgeModFace */
      floodface = edge->face_right;
      RTDEBUGF(1, "floodface is %" RTTFMT_ELEMID, floodface);
      /* update mbr of floodface as union of mbr of both faces */
      face_ids[0] = edge->face_left;
      face_ids[1] = edge->face_right;
      nfaces = 2;
      fields = RTT_COL_FACE_ALL;
      faces = rtt_be_getFaceById(topo, face_ids, &nfaces, fields);
      if ( nfaces == -1 ) {
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      GBOX *box1=NULL;
      GBOX *box2=NULL;
      for ( i=0; i<nfaces; ++i )
      {
        if ( faces[i].face_id == edge->face_left )
        {
          if ( ! box1 ) box1 = faces[i].mbr;
          else
          {
            i = edge->face_left;
            _rtt_release_edges(edge, 1);
            _rtt_release_faces(faces, nfaces);
            rterror("corrupted topology: more than 1 face have face_id=%"
                    RTTFMT_ELEMID, i);
            return -1;
          }
        }
        else if ( faces[i].face_id == edge->face_right )
        {
          if ( ! box2 ) box2 = faces[i].mbr;
          else
          {
            i = edge->face_right;
            _rtt_release_edges(edge, 1);
            _rtt_release_faces(faces, nfaces);
            rterror("corrupted topology: more than 1 face have face_id=%"
                    RTTFMT_ELEMID, i);
            return -1;
          }
        }
        else
        {
          i = faces[i].face_id;
          _rtt_release_edges(edge, 1);
          _rtt_release_faces(faces, nfaces);
          rterror("Backend coding error: getFaceById returned face "
                  "with non-requested id %" RTTFMT_ELEMID, i);
          return -1;
        }
      }
      if ( ! box1 ) {
        i = edge->face_left;
        _rtt_release_edges(edge, 1);
        _rtt_release_faces(faces, nfaces);
        rterror("corrupted topology: no face have face_id=%"
                RTTFMT_ELEMID " (left face for edge %"
                RTTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      if ( ! box2 ) {
        i = edge->face_right;
        _rtt_release_edges(edge, 1);
        _rtt_release_faces(faces, nfaces);
        rterror("corrupted topology: no face have face_id=%"
                RTTFMT_ELEMID " (right face for edge %"
                RTTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      gbox_merge(box2, box1); /* box1 is now the union of the two */
      newface.mbr = box1;
      if ( modFace )
      {
        newface.face_id = floodface;
        i = rtt_be_updateFacesById( topo, &newface, 1 );
        _rtt_release_faces(faces, 2);
        if ( i == -1 )
        {
          _rtt_release_edges(edge, 1);
          rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
          return -1;
        }
        if ( i != 1 )
        {
          _rtt_release_edges(edge, 1);
          rterror("Unexpected error: %d faces updated when expecting 1", i);
          return -1;
        }
      }
      else
      {
        /* New face replaces the old two faces */
        newface.face_id = -1;
        i = rtt_be_insertFaces( topo, &newface, 1 );
        _rtt_release_faces(faces, 2);
        if ( i == -1 )
        {
          _rtt_release_edges(edge, 1);
          rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
          return -1;
        }
        if ( i != 1 )
        {
          _rtt_release_edges(edge, 1);
          rterror("Unexpected error: %d faces inserted when expecting 1", i);
          return -1;
        }
        floodface = newface.face_id;
      }
    }

    /* Update face references for edges and nodes still referencing
     * the removed face(s) */

    if ( edge->face_left != floodface )
    {
      if ( -1 == _rtt_UpdateEdgeFaceRef(topo, edge->face_left, floodface) )
      {
        _rtt_release_edges(edge, 1);
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      if ( -1 == _rtt_UpdateNodeFaceRef(topo, edge->face_left, floodface) )
      {
        _rtt_release_edges(edge, 1);
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }

    if ( edge->face_right != floodface )
    {
      if ( -1 == _rtt_UpdateEdgeFaceRef(topo, edge->face_right, floodface) )
      {
        _rtt_release_edges(edge, 1);
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      if ( -1 == _rtt_UpdateNodeFaceRef(topo, edge->face_right, floodface) )
      {
        _rtt_release_edges(edge, 1);
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }

    /* Update topogeoms on heal */
    if ( ! rtt_be_updateTopoGeomFaceHeal(topo,
                                  edge->face_right, edge->face_left,
                                  floodface) )
    {
      _rtt_release_edges(edge, 1);
      rterror("%s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  } /* two faces healed */

  /* Delete the edge */
  i = rtt_be_deleteEdges(topo, edge, RTT_COL_EDGE_EDGE_ID);
  if ( i == -1 ) {
    _rtt_release_edges(edge, 1);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* If any of the edge nodes remained isolated, set
   * containing_face = floodface
   */
  if ( ! fnode_edges )
  {
    upd_node[nnode].node_id = edge->start_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( edge->end_node != edge->start_node && ! lnode_edges )
  {
    upd_node[nnode].node_id = edge->end_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( nnode )
  {
    i = rtt_be_updateNodesById(topo, upd_node, nnode,
                               RTT_COL_NODE_CONTAINING_FACE);
    if ( i == -1 ) {
      _rtt_release_edges(edge, 1);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  if ( edge->face_left != edge->face_right )
  /* or there'd be no face to remove */
  {
    RTT_ELEMID ids[2];
    int nids = 0;
    if ( edge->face_right != floodface )
      ids[nids++] = edge->face_right;
    if ( edge->face_left != floodface )
      ids[nids++] = edge->face_left;
    i = rtt_be_deleteFacesById(topo, ids, nids);
    if ( i == -1 ) {
      _rtt_release_edges(edge, 1);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  _rtt_release_edges(edge, 1);
  return modFace ? floodface : newface.face_id;
}

RTT_ELEMID
rtt_RemEdgeModFace( RTT_TOPOLOGY* topo, RTT_ELEMID edge_id )
{
  return _rtt_RemEdge( topo, edge_id, 1 );
}

RTT_ELEMID
rtt_RemEdgeNewFace( RTT_TOPOLOGY* topo, RTT_ELEMID edge_id )
{
  return _rtt_RemEdge( topo, edge_id, 0 );
}

static RTT_ELEMID
_rtt_HealEdges( RTT_TOPOLOGY* topo, RTT_ELEMID eid1, RTT_ELEMID eid2,
                int modEdge )
{
  RTT_ELEMID ids[2];
  RTT_ELEMID commonnode = -1;
  int caseno = 0;
  RTT_ISO_EDGE *node_edges;
  int num_node_edges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_EDGE *e1 = NULL;
  RTT_ISO_EDGE *e2 = NULL;
  RTT_ISO_EDGE newedge, updedge, seledge;
  int nedges, i;
  int e1freenode;
  int e2sign, e2freenode;
  POINTARRAY *pa;
  char buf[256];
  char *ptr;
  size_t bufleft = 256;

  ptr = buf;

  /* NOT IN THE SPECS: see if the same edge is given twice.. */
  if ( eid1 == eid2 )
  {
    rterror("Cannot heal edge %" RTTFMT_ELEMID
            " with itself, try with another", eid1);
    return -1;
  }
  ids[0] = eid1;
  ids[1] = eid2;
  nedges = 2;
  edges = rtt_be_getEdgeById(topo, ids, &nedges, RTT_COL_EDGE_ALL);
  if ( nedges == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<nedges; ++i )
  {
    if ( edges[i].edge_id == eid1 ) {
      if ( e1 ) {
        _rtt_release_edges(edges, nedges);
        rterror("Corrupted topology: multiple edges have id %"
                RTTFMT_ELEMID, eid1);
        return -1;
      }
      e1 = &(edges[i]);
    }
    else if ( edges[i].edge_id == eid2 ) {
      if ( e2 ) {
        _rtt_release_edges(edges, nedges);
        rterror("Corrupted topology: multiple edges have id %"
                RTTFMT_ELEMID, eid2);
        return -1;
      }
      e2 = &(edges[i]);
    }
  }
  if ( ! e1 )
  {
    if ( edges ) _rtt_release_edges(edges, nedges);
    rterror("SQL/MM Spatial exception - non-existent edge %"
            RTTFMT_ELEMID, eid1);
    return -1;
  }
  if ( ! e2 )
  {
    if ( edges ) _rtt_release_edges(edges, nedges);
    rterror("SQL/MM Spatial exception - non-existent edge %"
            RTTFMT_ELEMID, eid2);
    return -1;
  }

  /* NOT IN THE SPECS: See if any of the two edges are closed. */
  if ( e1->start_node == e1->end_node )
  {
    _rtt_release_edges(edges, nedges);
    rterror("Edge %" RTTFMT_ELEMID " is closed, cannot heal to edge %"
            RTTFMT_ELEMID, eid1, eid2);
    return -1;
  }
  if ( e2->start_node == e2->end_node )
  {
    _rtt_release_edges(edges, nedges);
    rterror("Edge %" RTTFMT_ELEMID " is closed, cannot heal to edge %"
            RTTFMT_ELEMID, eid2, eid1);
    return -1;
  }

  /* Find common node */

  if ( e1->end_node == e2->start_node )
  {
    commonnode = e1->end_node;
    caseno = 1;
  }
  else if ( e1->end_node == e2->end_node )
  {
    commonnode = e1->end_node;
    caseno = 2;
  }
  /* Check if any other edge is connected to the common node, if found */
  if ( commonnode != -1 )
  {
    num_node_edges = 1;
    node_edges = rtt_be_getEdgeByNode( topo, &commonnode,
                                       &num_node_edges, RTT_COL_EDGE_EDGE_ID );
    if ( num_node_edges == -1 ) {
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    for (i=0; i<num_node_edges; ++i)
    {
      int r;
      if ( node_edges[i].edge_id == eid1 ) continue;
      if ( node_edges[i].edge_id == eid2 ) continue;
      commonnode = -1;
      /* append to string, for error message */
      if ( bufleft ) {
        r = snprintf(ptr, bufleft, "%s%" RTTFMT_ELEMID,
                     ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
        if ( r >= bufleft )
        {
          bufleft = 0;
          buf[252] = '.';
          buf[253] = '.';
          buf[254] = '.';
          buf[255] = '\0';
        }
        else
        {
          bufleft -= r;
          ptr += r;
        }
      }
    }
    rtfree(node_edges);
  }

  if ( commonnode == -1 )
  {
    if ( e1->start_node == e2->start_node )
    {
      commonnode = e1->start_node;
      caseno = 3;
    }
    else if ( e1->start_node == e2->end_node )
    {
      commonnode = e1->start_node;
      caseno = 4;
    }
    /* Check if any other edge is connected to the common node, if found */
    if ( commonnode != -1 )
    {
      num_node_edges = 1;
      node_edges = rtt_be_getEdgeByNode( topo, &commonnode,
                                         &num_node_edges, RTT_COL_EDGE_EDGE_ID );
      if ( num_node_edges == -1 ) {
        _rtt_release_edges(edges, nedges);
        rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      for (i=0; i<num_node_edges; ++i)
      {
        int r;
        if ( node_edges[i].edge_id == eid1 ) continue;
        if ( node_edges[i].edge_id == eid2 ) continue;
        commonnode = -1;
        /* append to string, for error message */
        if ( bufleft ) {
          r = snprintf(ptr, bufleft, "%s%" RTTFMT_ELEMID,
                       ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
          if ( r >= bufleft )
          {
            bufleft = 0;
            buf[252] = '.';
            buf[253] = '.';
            buf[254] = '.';
            buf[255] = '\0';
          }
          else
          {
            bufleft -= r;
            ptr += r;
          }
        }
      }
      if ( num_node_edges ) rtfree(node_edges);
    }
  }

  if ( commonnode == -1 )
  {
    _rtt_release_edges(edges, nedges);
    if ( ptr != buf )
    {
      rterror("SQL/MM Spatial exception - other edges connected (%s)",
              buf);
    }
    else
    {
      rterror("SQL/MM Spatial exception - non-connected edges");
    }
    return -1;
  }

  if ( ! rtt_be_checkTopoGeomRemNode(topo, commonnode,
                                     eid1, eid2 ) )
  {
    _rtt_release_edges(edges, nedges);
    rterror("%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Construct the geometry of the new edge */
  switch (caseno)
  {
    case 1: /* e1.end = e2.start */
      pa = ptarray_clone_deep(e1->geom->points);
      //pa = ptarray_merge(pa, e2->geom->points);
      ptarray_append_ptarray(pa, e2->geom->points, 0);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->end_node;
      newedge.next_left = e2->next_left;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = -1;
      e2sign = 1;
      break;
    case 2: /* e1.end = e2.end */
    {
      POINTARRAY *pa2;
      pa2 = ptarray_clone_deep(e2->geom->points);
      ptarray_reverse(pa2);
      pa = ptarray_clone_deep(e1->geom->points);
      //pa = ptarray_merge(e1->geom->points, pa);
      ptarray_append_ptarray(pa, pa2, 0);
      ptarray_free(pa2);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->start_node;
      newedge.next_left = e2->next_right;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = 1;
      e2sign = -1;
      break;
    }
    case 3: /* e1.start = e2.start */
      pa = ptarray_clone_deep(e2->geom->points);
      ptarray_reverse(pa);
      //pa = ptarray_merge(pa, e1->geom->points);
      ptarray_append_ptarray(pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->end_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_left;
      e1freenode = -1;
      e2freenode = -1;
      e2sign = -1;
      break;
    case 4: /* e1.start = e2.end */
      pa = ptarray_clone_deep(e2->geom->points);
      //pa = ptarray_merge(pa, e1->geom->points);
      ptarray_append_ptarray(pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->start_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_right;
      e1freenode = -1;
      e2freenode = 1;
      e2sign = 1;
      break;
    default:
      pa = NULL;
      e1freenode = 0;
      e2freenode = 0;
      e2sign = 0;
      _rtt_release_edges(edges, nedges);
      rterror("Coding error: caseno=%d should never happen", caseno);
      break;
  }
  newedge.geom = rtline_construct(topo->srid, NULL, pa);

  if ( modEdge )
  {
    /* Update data of the first edge */
    newedge.edge_id = eid1;
    i = rtt_be_updateEdgesById(topo, &newedge, 1,
                               RTT_COL_EDGE_NEXT_LEFT|
                               RTT_COL_EDGE_NEXT_RIGHT |
                               RTT_COL_EDGE_START_NODE |
                               RTT_COL_EDGE_END_NODE |
                               RTT_COL_EDGE_GEOM);
    if ( i == -1 )
    {
      rtline_free(newedge.geom);
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i != 1 )
    {
      rtline_free(newedge.geom);
      if ( edges ) _rtt_release_edges(edges, nedges);
      rterror("Unexpected error: %d edges updated when expecting 1", i);
      return -1;
    }
  }
  else
  {
    /* Add new edge */
    newedge.edge_id = -1;
    newedge.face_left = e1->face_left;
    newedge.face_right = e1->face_right;
    i = rtt_be_insertEdges(topo, &newedge, 1);
    if ( i == -1 ) {
      rtline_free(newedge.geom);
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    } else if ( i == 0 ) {
      rtline_free(newedge.geom);
      _rtt_release_edges(edges, nedges);
      rterror("Insertion of split edge failed (no reason)");
      return -1;
    }
  }
  rtline_free(newedge.geom);

  /*
  -- Update next_left_edge/next_right_edge for
  -- any edge having them still pointing at the edge being removed
  -- (eid2 only when modEdge, or both otherwise)
  --
  -- NOTE:
  -- e#freenode is 1 when edge# end node was the common node
  -- and -1 otherwise. This gives the sign of possibly found references
  -- to its "free" (non connected to other edge) endnode.
  -- e2sign is -1 if edge1 direction is opposite to edge2 direction,
  -- or 1 otherwise.
  --
  */

  /* update edges connected to e2's boundary from their end node */
  seledge.next_left = e2freenode * eid2;
  updedge.next_left = e2freenode * newedge.edge_id * e2sign;
  i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_LEFT,
                               &updedge, RTT_COL_EDGE_NEXT_LEFT,
                               NULL, 0);
  if ( i == -1 )
  {
    _rtt_release_edges(edges, nedges);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* update edges connected to e2's boundary from their start node */
  seledge.next_right = e2freenode * eid2;
  updedge.next_right = e2freenode * newedge.edge_id * e2sign;
  i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_RIGHT,
                               &updedge, RTT_COL_EDGE_NEXT_RIGHT,
                               NULL, 0);
  if ( i == -1 )
  {
    _rtt_release_edges(edges, nedges);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  if ( ! modEdge )
  {
    /* update edges connected to e1's boundary from their end node */
    seledge.next_left = e1freenode * eid1;
    updedge.next_left = e1freenode * newedge.edge_id;
    i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_LEFT,
                                 &updedge, RTT_COL_EDGE_NEXT_LEFT,
                                 NULL, 0);
    if ( i == -1 )
    {
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    /* update edges connected to e1's boundary from their start node */
    seledge.next_right = e1freenode * eid1;
    updedge.next_right = e1freenode * newedge.edge_id;
    i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_RIGHT,
                                 &updedge, RTT_COL_EDGE_NEXT_RIGHT,
                                 NULL, 0);
    if ( i == -1 )
    {
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* delete the edges (only second on modEdge or both) */
  i = rtt_be_deleteEdges(topo, e2, RTT_COL_EDGE_EDGE_ID);
  if ( i == -1 )
  {
    _rtt_release_edges(edges, nedges);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! modEdge ) {
    i = rtt_be_deleteEdges(topo, e1, RTT_COL_EDGE_EDGE_ID);
    if ( i == -1 )
    {
      _rtt_release_edges(edges, nedges);
      rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  _rtt_release_edges(edges, nedges);

  /* delete the common node */
  i = rtt_be_deleteNodesById( topo, &commonnode, 1 );
  if ( i == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /*
  --
  -- NOT IN THE SPECS:
  -- Drop composition rows involving second
  -- edge, as the first edge took its space,
  -- and all affected TopoGeom have been previously checked
  -- for being composed by both edges.
  */
  if ( ! rtt_be_updateTopoGeomEdgeHeal(topo,
                                eid1, eid2, newedge.edge_id) )
  {
    rterror("%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return modEdge ? commonnode : newedge.edge_id;
}

RTT_ELEMID
rtt_ModEdgeHeal( RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2 )
{
  return _rtt_HealEdges( topo, e1, e2, 1 );
}

RTT_ELEMID
rtt_NewEdgeHeal( RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2 )
{
  return _rtt_HealEdges( topo, e1, e2, 0 );
}

RTT_ELEMID
rtt_GetNodeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ISO_NODE *elem;
  int num;
  int flds = RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM; /* geom not needed */
  RTT_ELEMID id = 0;
  POINT2D qp; /* query point */

  if ( ! getPoint2d_p(pt->point, 0, &qp) )
  {
    rterror("Empty query point");
    return -1;
  }
  elem = rtt_be_getNodeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  else if ( num )
  {
    if ( num > 1 )
    {
      _rtt_release_nodes(elem, num);
      rterror("Two or more nodes found");
      return -1;
    }
    id = elem[0].node_id;
    _rtt_release_nodes(elem, num);
  }

  return id;
}

RTT_ELEMID
rtt_GetEdgeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ISO_EDGE *elem;
  int num, i;
  int flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM; /* GEOM is not needed */
  RTT_ELEMID id = 0;
  RTGEOM *qp = rtpoint_as_rtgeom(pt); /* query point */

  if ( rtgeom_is_empty(qp) )
  {
    rterror("Empty query point");
    return -1;
  }
  elem = rtt_be_getEdgeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for (i=0; i<num;++i)
  {
    RTT_ISO_EDGE *e = &(elem[i]);
#if 0
    RTGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      _rtt_release_edges(elem, num);
      rtnotice("Corrupted topology: edge %" RTTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* Should we check for intersection not being on an endpoint
     * as documented ? */
    geom = rtline_as_rtgeom(e->geom);
    dist = rtgeom_mindistance2d_tolerance(geom, qp, tol);
    if ( dist > tol ) continue;
#endif

    if ( id )
    {
      _rtt_release_edges(elem, num);
      rterror("Two or more edges found");
      return -1;
    }
    else id = e->edge_id;
  }

  if ( num ) _rtt_release_edges(elem, num);

  return id;
}

RTT_ELEMID
rtt_GetFaceByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ELEMID id = 0;
  RTT_ISO_EDGE *elem;
  int num, i;
  int flds = RTT_COL_EDGE_EDGE_ID |
             RTT_COL_EDGE_GEOM |
             RTT_COL_EDGE_FACE_LEFT |
             RTT_COL_EDGE_FACE_RIGHT;
  RTGEOM *qp = rtpoint_as_rtgeom(pt);

  id = rtt_be_getFaceContainingPoint(topo, pt);
  if ( id == -2 ) {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  if ( id > 0 )
  {
    return id;
  }
  id = 0; /* or it'll be -1 for not found */

  RTDEBUG(1, "No face properly contains query point,"
             " looking for edges");

  /* Not in a face, may be in universe or on edge, let's check
   * for distance */
  /* NOTE: we never pass a tolerance of 0 to avoid ever using
   * ST_Within, which doesn't include endpoints matches */
  elem = rtt_be_getEdgeWithinDistance2D(topo, pt, tol?tol:1e-5, &num, flds, 0);
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for (i=0; i<num; ++i)
  {
    RTT_ISO_EDGE *e = &(elem[i]);
    RTT_ELEMID eface = 0;
    RTGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      _rtt_release_edges(elem, num);
      rtnotice("Corrupted topology: edge %" RTTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* don't consider dangling edges */
    if ( e->face_left == e->face_right )
    {
      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID
                  " is dangling, won't consider it", e->edge_id);
      continue;
    }

    geom = rtline_as_rtgeom(e->geom);
    dist = rtgeom_mindistance2d_tolerance(geom, qp, tol);

    RTDEBUGF(1, "Distance from edge %" RTTFMT_ELEMID
                " is %g (tol=%g)", e->edge_id, dist, tol);

    /* we won't consider edges too far */
    if ( dist > tol ) continue;
    if ( e->face_left == 0 ) {
      eface = e->face_right;
    }
    else if ( e->face_right == 0 ) {
      eface = e->face_left;
    }
    else {
      _rtt_release_edges(elem, num);
      rterror("Two or more faces found");
      return -1;
    }

    if ( id && id != eface )
    {
      _rtt_release_edges(elem, num);
      rterror("Two or more faces found"
#if 0 /* debugging */
              " (%" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID ")", id, eface
#endif
             );
      return -1;
    }
    else id = eface;
  }
  if ( num ) _rtt_release_edges(elem, num);

  return id;
}

/* Return the smallest delta that can perturbate
 * the maximum absolute value of a geometry ordinate
 */
static double
_rtt_minTolerance( RTGEOM *g )
{
  const GBOX* gbox;
  double max;
  double ret;

  gbox = rtgeom_get_bbox(g);
  if ( ! gbox ) return 0; /* empty */
  max = FP_ABS(gbox->xmin);
  if ( max < FP_ABS(gbox->xmax) ) max = FP_ABS(gbox->xmax);
  if ( max < FP_ABS(gbox->ymin) ) max = FP_ABS(gbox->ymin);
  if ( max < FP_ABS(gbox->ymax) ) max = FP_ABS(gbox->ymax);

  ret = 3.6 * pow(10,  - ( 15 - log10(max?max:1.0) ) );

  return ret;
}

#define _RTT_MINTOLERANCE( topo, geom ) ( \
  topo->precision ?  topo->precision : _rtt_minTolerance(geom) )

typedef struct scored_pointer_t {
  void *ptr;
  double score;
} scored_pointer;

static int
compare_scored_pointer(const void *si1, const void *si2)
{
	double a = ((scored_pointer *)si1)->score;
	double b = ((scored_pointer *)si2)->score;
	if ( a < b )
		return -1;
	else if ( a > b )
		return 1;
	else
		return 0;
}

RTT_ELEMID
rtt_AddPoint(RTT_TOPOLOGY* topo, RTPOINT* point, double tol)
{
  int num, i;
  double mindist = FLT_MAX;
  RTT_ISO_NODE *nodes, *nodes2;
  RTT_ISO_EDGE *edges, *edges2;
  RTGEOM *pt = rtpoint_as_rtgeom(point);
  int flds;
  RTT_ELEMID id = 0;
  scored_pointer *sorted;

  /* Get tolerance, if 0 was given */
  if ( ! tol ) tol = _RTT_MINTOLERANCE( topo, pt );

  RTDEBUGG(1, pt, "Adding point");

  /*
  -- 1. Check if any existing node is closer than the given precision
  --    and if so pick the closest
  TODO: use WithinBox2D
  */
  flds = RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM;
  nodes = rtt_be_getNodeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
    RTDEBUGF(1, "New point is within %.15g units of %d nodes", tol, num);
    /* Order by distance if there are more than a single return */
    if ( num > 1 )
    {{
      sorted= rtalloc(sizeof(scored_pointer)*num);
      for (i=0; i<num; ++i)
      {
        sorted[i].ptr = nodes+i;
        sorted[i].score = rtgeom_mindistance2d(rtpoint_as_rtgeom(nodes[i].geom), pt);
        RTDEBUGF(1, "Node %" RTTFMT_ELEMID " distance: %.15g",
          ((RTT_ISO_NODE*)(sorted[i].ptr))->node_id, sorted[i].score);
      }
      qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
      nodes2 = rtalloc(sizeof(RTT_ISO_NODE)*num);
      for (i=0; i<num; ++i)
      {
        nodes2[i] = *((RTT_ISO_NODE*)sorted[i].ptr);
      }
      rtfree(sorted);
      rtfree(nodes);
      nodes = nodes2;
    }}

    for ( i=0; i<num; ++i )
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      RTGEOM *g = rtpoint_as_rtgeom(n->geom);
      double dist = rtgeom_mindistance2d(g, pt);
      /* TODO: move this check in the previous sort scan ... */
      if ( dist >= tol ) continue; /* must be closer than tolerated */
      if ( ! id || dist < mindist )
      {
        id = n->node_id;
        mindist = dist;
      }
    }
    if ( id )
    {
      /* found an existing node */
      if ( nodes ) _rtt_release_nodes(nodes, num);
      return id;
    }
  }

  initGEOS(rtnotice, rtgeom_geos_error);

  /*
  -- 2. Check if any existing edge falls within tolerance
  --    and if so split it by a point projected on it
  TODO: use WithinBox2D
  */
  flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM;
  edges = rtt_be_getEdgeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
  RTDEBUGF(1, "New point is within %.15g units of %d edges", tol, num);

  /* Order by distance if there are more than a single return */
  if ( num > 1 )
  {{
    int j;
    sorted = rtalloc(sizeof(scored_pointer)*num);
    for (i=0; i<num; ++i)
    {
      sorted[i].ptr = edges+i;
      sorted[i].score = rtgeom_mindistance2d(rtline_as_rtgeom(edges[i].geom), pt);
      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID " distance: %.15g",
        ((RTT_ISO_EDGE*)(sorted[i].ptr))->edge_id, sorted[i].score);
    }
    qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
    edges2 = rtalloc(sizeof(RTT_ISO_EDGE)*num);
    for (j=0, i=0; i<num; ++i)
    {
      if ( sorted[i].score == sorted[0].score )
      {
        edges2[j++] = *((RTT_ISO_EDGE*)sorted[i].ptr);
      }
      else
      {
        rtline_free(((RTT_ISO_EDGE*)sorted[i].ptr)->geom);
      }
    }
    num = j;
    rtfree(sorted);
    rtfree(edges);
    edges = edges2;
  }}

  for (i=0; i<num; ++i)
  {
    /* The point is on or near an edge, split the edge */
    RTT_ISO_EDGE *e = &(edges[i]);
    RTGEOM *g = rtline_as_rtgeom(e->geom);
    RTGEOM *prj;
    int contains;
    GEOSGeometry *prjg, *gg;

    RTDEBUGF(1, "Splitting edge %" RTTFMT_ELEMID, e->edge_id);

    /* project point to line, split edge by point */
    prj = rtgeom_closest_point(g, pt);
    if ( rtgeom_has_z(pt) )
    {{
      /*
      -- This is a workaround for ClosestPoint lack of Z support:
      -- http://trac.osgeo.org/postgis/ticket/2033
      */
      RTGEOM *tmp;
      double z;
      POINT4D p4d;
      RTPOINT *prjpt;
      /* add Z to "prj" */
      tmp = rtgeom_force_3dz(prj);
      prjpt = rtgeom_as_rtpoint(tmp);
      getPoint4d_p(point->point, 0, &p4d);
      z = p4d.z;
      getPoint4d_p(prjpt->point, 0, &p4d);
      p4d.z = z;
      ptarray_set_point4d(prjpt->point, 0, &p4d);
      rtgeom_free(prj);
      prj = tmp;
    }}
    prjg = RTGEOM2GEOS(prj, 0);
    if ( ! prjg ) {
      rtgeom_free(prj);
      _rtt_release_edges(edges, num);
      rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
      return -1;
    }
    gg = RTGEOM2GEOS(g, 0);
    if ( ! gg ) {
      rtgeom_free(prj);
      _rtt_release_edges(edges, num);
      GEOSGeom_destroy(prjg);
      rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
      return -1;
    }
    contains = GEOSContains(gg, prjg);
    GEOSGeom_destroy(prjg);
    GEOSGeom_destroy(gg);
    if ( contains == 2 )
    {
      rtgeom_free(prj);
      _rtt_release_edges(edges, num);
      rterror("GEOS exception on Contains: %s", rtgeom_geos_errmsg);
      return -1;
    } 
    if ( ! contains )
    {{
      double snaptol;
      RTGEOM *snapedge;
      RTLINE *snapline;
      POINT4D p1, p2;

      RTDEBUGF(1, "Edge %" RTTFMT_ELEMID
                  " does not contain projected point to it",
                  e->edge_id);

      /* In order to reduce the robustness issues, we'll pick
       * an edge that contains the projected point, if possible */
      if ( i+1 < num )
      {
        RTDEBUG(1, "But there's another to check");
        rtgeom_free(prj);
        continue;
      }

      /*
      -- The tolerance must be big enough for snapping to happen
      -- and small enough to snap only to the projected point.
      -- Unfortunately ST_Distance returns 0 because it also uses
      -- a projected point internally, so we need another way.
      */
      snaptol = _rtt_minTolerance(prj);
      snapedge = rtgeom_snap(g, prj, snaptol);
      snapline = rtgeom_as_rtline(snapedge);

      RTDEBUGF(1, "Edge snapped with tolerance %g", snaptol);

      /* TODO: check if snapping did anything ? */
#if RTGEOM_DEBUG_LEVEL > 0
      {
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(g, RTWKT_EXTENDED, 15, &sz);
      char *wkt2 = rtgeom_to_wkt(snapedge, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(1, "Edge %s snapped became %s", wkt1, wkt2);
      rtfree(wkt1);
      rtfree(wkt2);
      }
#endif


      /*
      -- Snapping currently snaps the first point below tolerance
      -- so may possibly move first point. See ticket #1631
      */
      getPoint4d_p(e->geom->points, 0, &p1);
      getPoint4d_p(snapline->points, 0, &p2);
      RTDEBUGF(1, "Edge first point is %g %g, "
                  "snapline first point is %g %g",
                  p1.x, p1.y, p2.x, p2.y);
      if ( p1.x != p2.x || p1.y != p2.y )
      {
        RTDEBUG(1, "Snapping moved first point, re-adding it");
        if ( RT_SUCCESS != ptarray_insert_point(snapline->points, &p1, 0) )
        {
          rtgeom_free(prj);
          rtgeom_free(snapedge);
          _rtt_release_edges(edges, num);
          rterror("GEOS exception on Contains: %s", rtgeom_geos_errmsg);
          return -1;
        }
#if RTGEOM_DEBUG_LEVEL > 0
        {
        size_t sz;
        char *wkt1 = rtgeom_to_wkt(g, RTWKT_EXTENDED, 15, &sz);
        RTDEBUGF(1, "Tweaked snapline became %s", wkt1);
        rtfree(wkt1);
        }
#endif
      }
#if RTGEOM_DEBUG_LEVEL > 0
      else {
        RTDEBUG(1, "Snapping did not move first point");
      }
#endif

      if ( -1 == rtt_ChangeEdgeGeom( topo, e->edge_id, snapline ) )
      {
        /* TODO: should have invoked rterror already, leaking memory */
        rtgeom_free(prj);
        rtgeom_free(snapedge);
        _rtt_release_edges(edges, num);
        rterror("rtt_ChangeEdgeGeom failed");
        return -1;
      }
      rtgeom_free(snapedge);
    }}
#if RTGEOM_DEBUG_LEVEL > 0
    else
    {{
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(g, RTWKT_EXTENDED, 15, &sz);
      char *wkt2 = rtgeom_to_wkt(prj, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(1, "Edge %s contains projected point %s", wkt1, wkt2);
      rtfree(wkt1);
      rtfree(wkt2);
    }}
#endif

    /* TODO: pass 1 as last argument (skipChecks) ? */
    id = rtt_ModEdgeSplit( topo, e->edge_id, rtgeom_as_rtpoint(prj), 0 );
    if ( -1 == id )
    {
      /* TODO: should have invoked rterror already, leaking memory */
      rtgeom_free(prj);
      _rtt_release_edges(edges, num);
      rterror("rtt_ModEdgeSplit failed");
      return -1;
    }

    rtgeom_free(prj);
    break; /* we only want to snap a single edge */
  }
  _rtt_release_edges(edges, num);
  }
  else
  {
    /* The point is isolated, add it as such */
    /* TODO: pass 1 as last argument (skipChecks) ? */
    id = rtt_AddIsoNode(topo, -1, point, 0);
    if ( -1 == id )
    {
      /* should have invoked rterror already, leaking memory */
      rterror("rtt_AddIsoNode failed");
      return -1;
    }
  }

  return id;
}

/* Return identifier of an equal edge, 0 if none or -1 on error
 * (and rterror gets called on error)
 */
static RTT_ELEMID
_rtt_GetEqualEdge( RTT_TOPOLOGY *topo, RTLINE *edge )
{
  RTT_ELEMID id;
  RTT_ISO_EDGE *edges;
  int num, i;
  const GBOX *qbox = rtgeom_get_bbox( rtline_as_rtgeom(edge) );
  GEOSGeometry *edgeg;
  const int flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM;

  edges = rtt_be_getEdgeWithinBox2D( topo, qbox, &num, flds, 0 );
  if ( num == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
    initGEOS(rtnotice, rtgeom_geos_error);

    edgeg = RTGEOM2GEOS( rtline_as_rtgeom(edge), 0 );
    if ( ! edgeg )
    {
      _rtt_release_edges(edges, num);
      rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
      return -1;
    }
    for (i=0; i<num; ++i)
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      RTGEOM *g = rtline_as_rtgeom(e->geom);
      GEOSGeometry *gg;
      int equals;
      gg = RTGEOM2GEOS( g, 0 );
      if ( ! gg )
      {
        GEOSGeom_destroy(edgeg);
        _rtt_release_edges(edges, num);
        rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
        return -1;
      }
      equals = GEOSEquals(gg, edgeg);
      GEOSGeom_destroy(gg);
      if ( equals == 2 )
      {
        GEOSGeom_destroy(edgeg);
        _rtt_release_edges(edges, num);
        rterror("GEOSEquals exception: %s", rtgeom_geos_errmsg);
        return -1;
      }
      if ( equals )
      {
        id = e->edge_id;
        GEOSGeom_destroy(edgeg);
        _rtt_release_edges(edges, num);
        return id;
      }
    }
    GEOSGeom_destroy(edgeg);
    _rtt_release_edges(edges, num);
  }

  return 0;
}

/*
 * Add a pre-noded pre-split line edge. Used by rtt_AddLine
 * Return edge id, 0 if none added (empty edge), -1 on error
 */
static RTT_ELEMID
_rtt_AddLineEdge( RTT_TOPOLOGY* topo, RTLINE* edge, double tol )
{
  RTCOLLECTION *col;
  RTPOINT *start_point, *end_point;
  RTGEOM *tmp;
  RTT_ISO_NODE *node;
  RTT_ELEMID nid[2]; /* start_node, end_node */
  RTT_ELEMID id; /* edge id */
  POINT4D p4d;
  int nn, i;

  start_point = rtline_get_rtpoint(edge, 0);
  if ( ! start_point )
  {
    rtnotice("Empty component of noded line");
    return 0; /* must be empty */
  }
  nid[0] = rtt_AddPoint( topo, start_point, tol );
  rtpoint_free(start_point); /* too late if rtt_AddPoint calls rterror */
  if ( nid[0] == -1 ) return -1; /* rterror should have been called */

  end_point = rtline_get_rtpoint(edge, edge->points->npoints-1);
  if ( ! end_point )
  {
    rterror("could not get last point of line "
            "after successfully getting first point !?");
    return -1;
  }
  nid[1] = rtt_AddPoint( topo, end_point, tol );
  rtpoint_free(end_point); /* too late if rtt_AddPoint calls rterror */
  if ( nid[1] == -1 ) return -1; /* rterror should have been called */

  /*
    -- Added endpoints may have drifted due to tolerance, so
    -- we need to re-snap the edge to the new nodes before adding it
  */

  nn = nid[0] == nid[1] ? 1 : 2;
  node = rtt_be_getNodeById( topo, nid, &nn,
                             RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM );
  if ( nn == -1 )
  {
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  start_point = NULL; end_point = NULL;
  for (i=0; i<nn; ++i)
  {
    if ( node[i].node_id == nid[0] ) start_point = node[i].geom;
    if ( node[i].node_id == nid[1] ) end_point = node[i].geom;
  }
  if ( ! start_point  || ! end_point )
  {
    if ( nn ) _rtt_release_nodes(node, nn);
    rterror("Could not find just-added nodes % " RTTFMT_ELEMID
            " and %" RTTFMT_ELEMID, nid[0], nid[1]);
    return -1;
  }

  /* snap */

  getPoint4d_p( start_point->point, 0, &p4d );
  rtline_setPoint4d(edge, 0, &p4d);

  getPoint4d_p( end_point->point, 0, &p4d );
  rtline_setPoint4d(edge, edge->points->npoints-1, &p4d);

  if ( nn ) _rtt_release_nodes(node, nn);

  /* make valid, after snap (to handle collapses) */
  tmp = rtgeom_make_valid(rtline_as_rtgeom(edge));

  col = rtgeom_as_rtcollection(tmp);
  if ( col )
  {{
    RTGEOM *tmp2;

    col = rtcollection_extract(col, RTLINETYPE);

    /* Check if the so-snapped edge collapsed (see #1650) */
    if ( col->ngeoms == 0 )
    {
      rtcollection_free(col);
      rtgeom_free(tmp);
      RTDEBUG(1, "Made-valid snapped edge collapsed");
      return 0;
    }

    tmp2 = rtgeom_clone_deep( col->geoms[0] );
    rtgeom_free(tmp);
    tmp = tmp2;
    edge = rtgeom_as_rtline(tmp);
    rtcollection_free(col);
    if ( ! edge )
    {
      /* should never happen */
      rterror("rtcollection_extract(RTLINETYPE) returned a non-line?");
      return -1;
    }
  }}
  else
  {
    edge = rtgeom_as_rtline(tmp);
    if ( ! edge )
    {
      RTDEBUGF(1, "Made-valid snapped edge collapsed to %s",
                  rttype_name(rtgeom_get_type(tmp)));
      rtgeom_free(tmp);
      return 0;
    }
  }

  /* check if the so-snapped edge _now_ exists */
  id = _rtt_GetEqualEdge ( topo, edge );
  RTDEBUGF(1, "_rtt_GetEqualEdge returned %" RTTFMT_ELEMID, id);
  if ( id == -1 )
  {
    rtgeom_free(tmp); /* probably too late, due to internal rterror */
    return -1;
  }
  if ( id ) 
  {
    rtgeom_free(tmp); /* possibly takes "edge" down with it */
    return id;
  }

  /* No previously existing edge was found, we'll add one */
  /* TODO: skip checks, I guess ? */
  id = rtt_AddEdgeModFace( topo, nid[0], nid[1], edge, 0 );
  RTDEBUGF(1, "rtt_AddEdgeModFace returned %" RTTFMT_ELEMID, id);
  if ( id == -1 )
  {
    rtgeom_free(tmp); /* probably too late, due to internal rterror */
    return -1;
  }
  rtgeom_free(tmp); /* possibly takes "edge" down with it */

  return id;
}

/* Simulate split-loop as it was implemented in pl/pgsql version
 * of TopoGeo_addLinestring */
static RTGEOM *
_rtt_split_by_nodes(const RTGEOM *g, const RTGEOM *nodes)
{
  RTCOLLECTION *col = rtgeom_as_rtcollection(nodes);
  int i;
  RTGEOM *bg;

  bg = rtgeom_clone_deep(g);
  if ( ! col->ngeoms ) return bg;

  for (i=0; i<col->ngeoms; ++i)
  {
    RTGEOM *g2;
    g2 = rtgeom_split(bg, col->geoms[i]);
    rtgeom_free(bg);
    bg = g2;
  }
  bg->srid = nodes->srid;

  return bg;
}

RTT_ELEMID*
rtt_AddLine(RTT_TOPOLOGY* topo, RTLINE* line, double tol, int* nedges)
{
  RTGEOM *geomsbuf[1];
  RTGEOM **geoms;
  int ngeoms;
  RTGEOM *noded;
  RTCOLLECTION *col;
  RTT_ELEMID *ids;
  RTT_ISO_EDGE *edges;
  RTT_ISO_NODE *nodes;
  int num;
  int i;
  GBOX qbox;

  *nedges = -1; /* error condition, by default */

  /* Get tolerance, if 0 was given */
  if ( ! tol ) tol = _RTT_MINTOLERANCE( topo, (RTGEOM*)line );
  RTDEBUGF(1, "Working tolerance:%.15g", tol);
  RTDEBUGF(1, "Input line has srid=%d", line->srid);

  /* 1. Self-node */
  noded = rtgeom_node((RTGEOM*)line);
  if ( ! noded ) return NULL; /* should have called rterror already */
  RTDEBUGG(1, noded, "Noded");

  qbox = *rtgeom_get_bbox( rtline_as_rtgeom(line) );
  RTDEBUGF(1, "Line BOX is %.15g %.15g, %.15g %.15g", qbox.xmin, qbox.ymin,
                                          qbox.xmax, qbox.ymax);
  gbox_expand(&qbox, tol);
  RTDEBUGF(1, "BOX expanded by %g is %.15g %.15g, %.15g %.15g",
              tol, qbox.xmin, qbox.ymin, qbox.xmax, qbox.ymax);

  /* 2. Node to edges falling within tol distance */
  edges = rtt_be_getEdgeWithinBox2D( topo, &qbox, &num, RTT_COL_EDGE_ALL, 0 );
  if ( num == -1 )
  {
    rtgeom_free(noded);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }
  RTDEBUGF(1, "Line bbox intersects %d edges bboxes", num);
  if ( num )
  {{
    /* collect those whose distance from us is < tol */
    RTGEOM **nearby = rtalloc(sizeof(RTGEOM *)*num);
    int nn=0;
    for (i=0; i<num; ++i)
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      RTGEOM *g = rtline_as_rtgeom(e->geom);
      double dist = rtgeom_mindistance2d(g, noded);
      if ( dist >= tol ) continue; /* must be closer than tolerated */
      nearby[nn++] = g;
    }
    if ( nn )
    {{
      RTCOLLECTION *col;
      RTGEOM *iedges; /* just an alias for col */
      RTGEOM *snapped;
      RTGEOM *set1, *set2;

      RTDEBUGF(1, "Line intersects %d edges", nn);

      col = rtcollection_construct(RTCOLLECTIONTYPE, topo->srid,
                                   NULL, nn, nearby);
      iedges = rtcollection_as_rtgeom(col);
      RTDEBUGG(1, iedges, "Collected edges");
      RTDEBUGF(1, "Snapping noded, with srid=%d "
                  "to interesecting edges, with srid=%d",
                  noded->srid, iedges->srid);
      snapped = rtgeom_snap(noded, iedges, tol);
      rtgeom_free(noded);
      RTDEBUGG(1, snapped, "Snapped");
      RTDEBUGF(1, "Diffing snapped, with srid=%d "
                  "and interesecting edges, with srid=%d",
                  snapped->srid, iedges->srid);
      noded = rtgeom_difference(snapped, iedges);
      RTDEBUGG(1, noded, "Differenced");
      RTDEBUGF(1, "Intersecting snapped, with srid=%d "
                  "and interesecting edges, with srid=%d",
                  snapped->srid, iedges->srid);
      set1 = rtgeom_intersection(snapped, iedges);
      RTDEBUGG(1, set1, "Intersected");
      rtgeom_free(snapped);
      RTDEBUGF(1, "Linemerging set1, with srid=%d", set1->srid);
      set2 = rtgeom_linemerge(set1);
      RTDEBUGG(1, set2, "Linemerged");
      RTDEBUGG(1, noded, "Noded");
      rtgeom_free(set1);
      RTDEBUGF(1, "Unioning noded, with srid=%d "
                  "and set2, with srid=%d", noded->srid, set2->srid);
      set1 = rtgeom_union(noded, set2);
      rtgeom_free(set2);
      rtgeom_free(noded);
      noded = set1;
      RTDEBUGG(1, set1, "Unioned");

      /* will not release the geoms array */
      rtcollection_release(col);
    }}
    rtfree(nearby);
    _rtt_release_edges(edges, num);
  }}

  /* 2.1. Node with existing nodes within tol
   * TODO: check if we should be only considering _isolated_ nodes! */
  nodes = rtt_be_getNodeWithinBox2D( topo, &qbox, &num, RTT_COL_NODE_ALL, 0 );
  if ( num == -1 )
  {
    rtgeom_free(noded);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }
  RTDEBUGF(1, "Line bbox intersects %d nodes bboxes", num);
  if ( num )
  {{
    /* collect those whose distance from us is < tol */
    RTGEOM **nearby = rtalloc(sizeof(RTGEOM *)*num);
    int nn=0;
    for (i=0; i<num; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      RTGEOM *g = rtpoint_as_rtgeom(n->geom);
      double dist = rtgeom_mindistance2d(g, noded);
      if ( dist >= tol ) continue; /* must be closer than tolerated */
      nearby[nn++] = g;
    }
    if ( nn )
    {{
      RTCOLLECTION *col;
      RTGEOM *inodes; /* just an alias for col */
      RTGEOM *tmp;

      RTDEBUGF(1, "Line intersects %d nodes", nn);

      col = rtcollection_construct(RTMULTIPOINTTYPE, topo->srid,
                                   NULL, nn, nearby);
      inodes = rtcollection_as_rtgeom(col);

      RTDEBUGG(1, inodes, "Collected nodes");

      /* TODO: consider snapping once against all elements
       *      (rather than once with edges and once with nodes) */
      tmp = rtgeom_snap(noded, inodes, tol);
      rtgeom_free(noded);
      noded = tmp;
      RTDEBUGG(1, noded, "Node-snapped");

      tmp = _rtt_split_by_nodes(noded, inodes);
          /* rtgeom_split(noded, inodes); */
      rtgeom_free(noded);
      noded = tmp;
      RTDEBUGG(1, noded, "Node-split");

      /* will not release the geoms array */
      rtcollection_release(col);

      /*
      -- re-node to account for ST_Snap introduced self-intersections
      -- See http://trac.osgeo.org/postgis/ticket/1714
      -- TODO: consider running UnaryUnion once after all noding
      */
      tmp = rtgeom_unaryunion(noded);
      rtgeom_free(noded);
      noded = tmp;
      RTDEBUGG(1, noded, "Unary-unioned");

    }}
    rtfree(nearby);
    _rtt_release_nodes(nodes, num);
  }}

  RTDEBUGG(1, noded, "Finally-noded");

  /* 3. For each (now-noded) segment, insert an edge */
  col = rtgeom_as_rtcollection(noded);
  if ( col )
  {
    RTDEBUG(1, "Noded line was a collection");
    geoms = col->geoms;
    ngeoms = col->ngeoms;
  }
  else
  {
    RTDEBUG(1, "Noded line was a single geom");
    geomsbuf[0] = noded;
    geoms = geomsbuf;
    ngeoms = 1;
  }

  RTDEBUGF(1, "Line was split into %d edges", ngeoms);

  /* TODO: refactor to first add all nodes (re-snapping edges if
   * needed) and then check all edges for existing already
   * ( so to save a DB scan for each edge to be added )
   */
  ids = rtalloc(sizeof(RTT_ELEMID)*ngeoms);
  num = 0;
  for ( i=0; i<ngeoms; ++i )
  {
    RTT_ELEMID id;
    RTGEOM *g = geoms[i];
    g->srid = noded->srid;

#if RTGEOM_DEBUG_LEVEL > 0
    {
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(g, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(1, "Component %d of split line is: %s", i, wkt1);
      rtfree(wkt1);
    }
#endif

    id = _rtt_AddLineEdge( topo, rtgeom_as_rtline(g), tol );
    RTDEBUGF(1, "_rtt_AddLineEdge returned %" RTTFMT_ELEMID, id);
    if ( id < 0 )
    {
      rtgeom_free(noded);
      rtfree(ids);
      return NULL;
    }
    if ( ! id )
    {
      RTDEBUGF(1, "Component %d of split line collapsed", i);
      continue;
    }

    RTDEBUGF(1, "Component %d of split line is edge %" RTTFMT_ELEMID,
                  i, id);
    ids[num++] = id; /* TODO: skip duplicates */
  }

  RTDEBUGG(1, noded, "Noded before free");
  rtgeom_free(noded);

  /* TODO: XXX remove duplicated ids if not done before */

  *nedges = num;
  return ids;
}

RTT_ELEMID*
rtt_AddPolygon(RTT_TOPOLOGY* topo, RTPOLY* poly, double tol, int* nfaces)
{
  int i;
  *nfaces = -1; /* error condition, by default */
  int num;
  RTT_ISO_FACE *faces;
  int nfacesinbox;
  RTT_ELEMID *ids = NULL;
  GBOX qbox;
  const GEOSPreparedGeometry *ppoly;
  GEOSGeometry *polyg;

  /* Get tolerance, if 0 was given */
  if ( ! tol ) tol = _RTT_MINTOLERANCE( topo, (RTGEOM*)poly );
  RTDEBUGF(1, "Working tolerance:%.15g", tol);

  /* Add each ring as an edge */
  for ( i=0; i<poly->nrings; ++i )
  {
    RTLINE *line;
    POINTARRAY *pa;
    RTT_ELEMID *eids;
    int nedges;

    pa = ptarray_clone(poly->rings[i]);
    line = rtline_construct(topo->srid, NULL, pa);
    eids = rtt_AddLine( topo, line, tol, &nedges );
    if ( nedges < 0 ) {
      /* probably too late as rtt_AddLine invoked rterror */
      rtline_free(line);
      rterror("Error adding ring %d of polygon", i);
      return NULL;
    }
    rtline_free(line);
    rtfree(eids);
  }

  /*
  -- Find faces covered by input polygon
  -- NOTE: potential snapping changed polygon edges
  */
  qbox = *rtgeom_get_bbox( rtpoly_as_rtgeom(poly) );
  gbox_expand(&qbox, tol);
  faces = rtt_be_getFaceWithinBox2D( topo, &qbox, &nfacesinbox,
                                     RTT_COL_FACE_ALL, 0 );
  if ( nfacesinbox == -1 )
  {
    rtfree(ids);
    rterror("Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }

  num = 0;
  if ( nfacesinbox )
  {
    polyg = RTGEOM2GEOS(rtpoly_as_rtgeom(poly), 0);
    if ( ! polyg )
    {
      _rtt_release_faces(faces, nfacesinbox);
      rterror("Could not convert poly geometry to GEOS: %s", rtgeom_geos_errmsg);
      return NULL;
    }
    ppoly = GEOSPrepare(polyg);
    ids = rtalloc(sizeof(RTT_ELEMID)*nfacesinbox);
    for ( i=0; i<nfacesinbox; ++i )
    {
      RTT_ISO_FACE *f = &(faces[i]);
      RTGEOM *fg;
      GEOSGeometry *fgg, *sp;
      int covers;

      /* check if a point on this face surface is covered by our polygon */
      fg = rtt_GetFaceGeometry( topo, f->face_id );
      if ( ! fg )
      {
        i = f->face_id; /* so we can destroy faces */
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        rtfree(ids);
        _rtt_release_faces(faces, nfacesinbox);
        rterror("Could not get geometry of face %" RTTFMT_ELEMID, i);
        return NULL;
      }
      /* check if a point on this face's surface is covered by our polygon */
      fgg = RTGEOM2GEOS(fg, 0);
      rtgeom_free(fg);
      if ( ! fgg )
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _rtt_release_faces(faces, nfacesinbox);
        rterror("Could not convert edge geometry to GEOS: %s", rtgeom_geos_errmsg);
        return NULL;
      }
      sp = GEOSPointOnSurface(fgg);
      GEOSGeom_destroy(fgg);
      if ( ! sp )
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _rtt_release_faces(faces, nfacesinbox);
        rterror("Could not find point on face surface: %s", rtgeom_geos_errmsg);
        return NULL;
      }
      covers = GEOSPreparedCovers( ppoly, sp );
      GEOSGeom_destroy(sp);
      if (covers == 2)
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _rtt_release_faces(faces, nfacesinbox);
        rterror("PreparedCovers error: %s", rtgeom_geos_errmsg);
        return NULL;
      }
      if ( ! covers )
      {
        continue; /* we're not composed by this face */
      }

      /* TODO: avoid duplicates ? */
      ids[num++] = f->face_id;
    }
    GEOSPreparedGeom_destroy(ppoly);
    GEOSGeom_destroy(polyg);
    _rtt_release_faces(faces, nfacesinbox);
  }

  /* possibly 0 if non face's surface point was found
   * to be covered by input polygon */
  *nfaces = num;

  return ids;
}