/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librtgeom.h"

/*
** Public prototypes for GEOS utility functions.
*/
RTGEOM *GEOS2RTGEOM(const RTCTX *ctx, const GEOSGeometry *geom, char want3d);
GEOSGeometry * RTGEOM2GEOS(const RTCTX *ctx, const RTGEOM *g, int autofix);
GEOSGeometry * GBOX2GEOS(const RTCTX *ctx, const RTGBOX *g);
GEOSGeometry * RTGEOM_GEOS_buildArea(const RTCTX *ctx, const GEOSGeometry* geom_in);

RTPOINTARRAY *ptarray_from_GEOSCoordSeq(const RTCTX *ctx, const GEOSCoordSequence *cs, char want3d);

/* Return (read-only) last geos error message */
const char *rtgeom_get_last_geos_error(const RTCTX *ctx);

extern void rtgeom_geos_error(const char *msg, void *ctx);

extern void rtgeom_geos_ensure_init(const RTCTX *ctx);

