/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2011-2014 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "rtgeom_geos.h"
#include "librtgeom.h"
#include "librtgeom_internal.h"
#include "rtgeom_log.h"

#include <stdlib.h>

RTTIN *rttin_from_geos(const GEOSGeometry *geom, int want3d);

#undef RTGEOM_PROFILE_BUILDAREA

#define RTGEOM_GEOS_ERRMSG_MAXSIZE 256
char rtgeom_geos_errmsg[RTGEOM_GEOS_ERRMSG_MAXSIZE];

extern void
rtgeom_geos_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	if ( RTGEOM_GEOS_ERRMSG_MAXSIZE-1 < vsnprintf(rtgeom_geos_errmsg, RTGEOM_GEOS_ERRMSG_MAXSIZE-1, fmt, ap) )
	{
		rtgeom_geos_errmsg[RTGEOM_GEOS_ERRMSG_MAXSIZE-1] = '\0';
	}

	va_end(ap);
}


/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY *
ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d)
{
	uint32_t dims=2;
	uint32_t size, i;
	POINTARRAY *pa;
	POINT4D point;

	RTDEBUG(2, "ptarray_fromGEOSCoordSeq called");

	if ( ! GEOSCoordSeq_getSize(cs, &size) )
		rterror("Exception thrown");

	RTDEBUGF(4, " GEOSCoordSeq size: %d", size);

	if ( want3d )
	{
		if ( ! GEOSCoordSeq_getDimensions(cs, &dims) )
			rterror("Exception thrown");

		RTDEBUGF(4, " GEOSCoordSeq dimensions: %d", dims);

		/* forget higher dimensions (if any) */
		if ( dims > 3 ) dims = 3;
	}

	RTDEBUGF(4, " output dimensions: %d", dims);

	pa = ptarray_construct((dims==3), 0, size);

	for (i=0; i<size; i++)
	{
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if ( dims >= 3 ) GEOSCoordSeq_getZ(cs, i, &(point.z));
		ptarray_set_point4d(pa,i,&point);
	}

	return pa;
}

/* Return an RTGEOM from a Geometry */
RTGEOM *
GEOS2RTGEOM(const GEOSGeometry *geom, char want3d)
{
	int type = GEOSGeomTypeId(geom) ;
	int hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if ( SRID == 0 ) SRID = SRID_UNKNOWN;

	if ( want3d )
	{
		hasZ = GEOSHasZ(geom);
		if ( ! hasZ )
		{
			RTDEBUG(3, "Geometry has no Z, won't provide one");

			want3d = 0;
		}
	}

/*
	if ( GEOSisEmpty(geom) )
	{
		return (RTGEOM*)rtcollection_construct_empty(RTCOLLECTIONTYPE, SRID, want3d, 0);
	}
*/

	switch (type)
	{
		const GEOSCoordSequence *cs;
		POINTARRAY *pa, **ppaa;
		const GEOSGeometry *g;
		RTGEOM **geoms;
		uint32_t i, ngeoms;

	case GEOS_POINT:
		RTDEBUG(4, "rtgeom_from_geometry: it's a Point");
		cs = GEOSGeom_getCoordSeq(geom);
		if ( GEOSisEmpty(geom) )
		  return (RTGEOM*)rtpoint_construct_empty(SRID, want3d, 0);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (RTGEOM *)rtpoint_construct(SRID, NULL, pa);

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		RTDEBUG(4, "rtgeom_from_geometry: it's a LineString or LinearRing");
		if ( GEOSisEmpty(geom) )
		  return (RTGEOM*)rtline_construct_empty(SRID, want3d, 0);

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (RTGEOM *)rtline_construct(SRID, NULL, pa);

	case GEOS_POLYGON:
		RTDEBUG(4, "rtgeom_from_geometry: it's a Polygon");
		if ( GEOSisEmpty(geom) )
		  return (RTGEOM*)rtpoly_construct_empty(SRID, want3d, 0);
		ngeoms = GEOSGetNumInteriorRings(geom);
		ppaa = rtalloc(sizeof(POINTARRAY *)*(ngeoms+1));
		g = GEOSGetExteriorRing(geom);
		cs = GEOSGeom_getCoordSeq(g);
		ppaa[0] = ptarray_from_GEOSCoordSeq(cs, want3d);
		for (i=0; i<ngeoms; i++)
		{
			g = GEOSGetInteriorRingN(geom, i);
			cs = GEOSGeom_getCoordSeq(g);
			ppaa[i+1] = ptarray_from_GEOSCoordSeq(cs,
			                                      want3d);
		}
		return (RTGEOM *)rtpoly_construct(SRID, NULL,
		                                  ngeoms+1, ppaa);

	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
		RTDEBUG(4, "rtgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if ( ngeoms )
		{
			geoms = rtalloc(sizeof(RTGEOM *)*ngeoms);
			for (i=0; i<ngeoms; i++)
			{
				g = GEOSGetGeometryN(geom, i);
				geoms[i] = GEOS2RTGEOM(g, want3d);
			}
		}
		return (RTGEOM *)rtcollection_construct(type,
		                                        SRID, NULL, ngeoms, geoms);

	default:
		rterror("GEOS2RTGEOM: unknown geometry type: %d", type);
		return NULL;

	}

}



GEOSCoordSeq ptarray_to_GEOSCoordSeq(const POINTARRAY *);


GEOSCoordSeq
ptarray_to_GEOSCoordSeq(const POINTARRAY *pa)
{
	uint32_t dims = 2;
	uint32_t i;
	const POINT3DZ *p3d;
	const POINT2D *p2d;
	GEOSCoordSeq sq;

	if ( FLAGS_GET_Z(pa->flags) ) 
		dims = 3;

	if ( ! (sq = GEOSCoordSeq_create(pa->npoints, dims)) ) 
		rterror("Error creating GEOS Coordinate Sequence");

	for ( i=0; i < pa->npoints; i++ )
	{
		if ( dims == 3 )
		{
			p3d = getPoint3dz_cp(pa, i);
			p2d = (const POINT2D *)p3d;
			RTDEBUGF(4, "Point: %g,%g,%g", p3d->x, p3d->y, p3d->z);
		}
		else
		{
			p2d = getPoint2d_cp(pa, i);
			RTDEBUGF(4, "Point: %g,%g", p2d->x, p2d->y);
		}

#if RTGEOM_GEOS_VERSION < 33
		/* Make sure we don't pass any infinite values down into GEOS */
		/* GEOS 3.3+ is supposed to  handle this stuff OK */
		if ( isinf(p2d->x) || isinf(p2d->y) || (dims == 3 && isinf(p3d->z)) )
			rterror("Infinite coordinate value found in geometry.");
		if ( isnan(p2d->x) || isnan(p2d->y) || (dims == 3 && isnan(p3d->z)) )
			rterror("NaN coordinate value found in geometry.");
#endif

		GEOSCoordSeq_setX(sq, i, p2d->x);
		GEOSCoordSeq_setY(sq, i, p2d->y);
		
		if ( dims == 3 ) 
			GEOSCoordSeq_setZ(sq, i, p3d->z);
	}
	return sq;
}

static GEOSGeometry *
ptarray_to_GEOSLinearRing(const POINTARRAY *pa, int autofix)
{
	GEOSCoordSeq sq;
	GEOSGeom g;
	POINTARRAY *npa = 0;

	if ( autofix )
	{
		/* check ring for being closed and fix if not */
		if ( ! ptarray_is_closed_2d(pa) ) 
		{
			npa = ptarray_addPoint(pa, getPoint_internal(pa, 0), FLAGS_NDIMS(pa->flags), pa->npoints);
			pa = npa;
		}
		/* TODO: check ring for having at least 4 vertices */
#if 0
		while ( pa->npoints < 4 ) 
		{
			npa = ptarray_addPoint(npa, getPoint_internal(pa, 0), FLAGS_NDIMS(pa->flags), pa->npoints);
		}
#endif
	}

	sq = ptarray_to_GEOSCoordSeq(pa);
	if ( npa ) ptarray_free(npa);
	g = GEOSGeom_createLinearRing(sq);
	return g;
}

GEOSGeometry *
GBOX2GEOS(const GBOX *box)
{
	GEOSGeometry* envelope;
	GEOSGeometry* ring;
	GEOSCoordSequence* seq = GEOSCoordSeq_create(5, 2);
	if (!seq) 
	{
		return NULL;
	}

	GEOSCoordSeq_setX(seq, 0, box->xmin);
	GEOSCoordSeq_setY(seq, 0, box->ymin);

	GEOSCoordSeq_setX(seq, 1, box->xmax);
	GEOSCoordSeq_setY(seq, 1, box->ymin);

	GEOSCoordSeq_setX(seq, 2, box->xmax);
	GEOSCoordSeq_setY(seq, 2, box->ymax);

	GEOSCoordSeq_setX(seq, 3, box->xmin);
	GEOSCoordSeq_setY(seq, 3, box->ymax);

	GEOSCoordSeq_setX(seq, 4, box->xmin);
	GEOSCoordSeq_setY(seq, 4, box->ymin);

	ring = GEOSGeom_createLinearRing(seq);
	if (!ring) 
	{
		GEOSCoordSeq_destroy(seq);
		return NULL;
	}

	envelope = GEOSGeom_createPolygon(ring, NULL, 0);
	if (!envelope) 
	{
		GEOSGeom_destroy(ring);
		return NULL;
	}

	return envelope;
}

GEOSGeometry *
RTGEOM2GEOS(const RTGEOM *rtgeom, int autofix)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell;
	GEOSGeom *geoms = NULL;
	/*
	RTGEOM *tmp;
	*/
	uint32_t ngeoms, i;
	int geostype;
#if RTDEBUG_LEVEL >= 4
	char *wkt;
#endif

	RTDEBUGF(4, "RTGEOM2GEOS got a %s", rttype_name(rtgeom->type));

	if (rtgeom_has_arc(rtgeom))
	{
		RTGEOM *rtgeom_stroked = rtgeom_stroke(rtgeom, 32);
		GEOSGeometry *g = RTGEOM2GEOS(rtgeom_stroked, autofix);
		rtgeom_free(rtgeom_stroked);
		return g;
	}
	
	switch (rtgeom->type)
	{
		RTPOINT *rtp = NULL;
		RTPOLY *rtpoly = NULL;
		RTLINE *rtl = NULL;
		RTCOLLECTION *rtc = NULL;
#if RTGEOM_GEOS_VERSION < 33
		POINTARRAY *pa = NULL;
#endif
		
	case RTPOINTTYPE:
		rtp = (RTPOINT *)rtgeom;
		
		if ( rtgeom_is_empty(rtgeom) )
		{
#if RTGEOM_GEOS_VERSION < 33
			pa = ptarray_construct_empty(rtgeom_has_z(rtgeom), rtgeom_has_m(rtgeom), 2);
			sq = ptarray_to_GEOSCoordSeq(pa);
			shell = GEOSGeom_createLinearRing(sq);
			g = GEOSGeom_createPolygon(shell, NULL, 0);
#else
			g = GEOSGeom_createEmptyPolygon();
#endif
		}
		else
		{
			sq = ptarray_to_GEOSCoordSeq(rtp->point);
			g = GEOSGeom_createPoint(sq);
		}
		if ( ! g )
		{
			/* rtnotice("Exception in RTGEOM2GEOS"); */
			return NULL;
		}
		break;
	case RTLINETYPE:
		rtl = (RTLINE *)rtgeom;
		/* TODO: if (autofix) */
		if ( rtl->points->npoints == 1 ) {
			/* Duplicate point, to make geos-friendly */
			rtl->points = ptarray_addPoint(rtl->points,
		                           getPoint_internal(rtl->points, 0),
		                           FLAGS_NDIMS(rtl->points->flags),
		                           rtl->points->npoints);
		}
		sq = ptarray_to_GEOSCoordSeq(rtl->points);
		g = GEOSGeom_createLineString(sq);
		if ( ! g )
		{
			/* rtnotice("Exception in RTGEOM2GEOS"); */
			return NULL;
		}
		break;

	case RTPOLYGONTYPE:
		rtpoly = (RTPOLY *)rtgeom;
		if ( rtgeom_is_empty(rtgeom) )
		{
#if RTGEOM_GEOS_VERSION < 33
			POINTARRAY *pa = ptarray_construct_empty(rtgeom_has_z(rtgeom), rtgeom_has_m(rtgeom), 2);
			sq = ptarray_to_GEOSCoordSeq(pa);
			shell = GEOSGeom_createLinearRing(sq);
			g = GEOSGeom_createPolygon(shell, NULL, 0);
#else
			g = GEOSGeom_createEmptyPolygon();
#endif
		}
		else
		{
			shell = ptarray_to_GEOSLinearRing(rtpoly->rings[0], autofix);
			if ( ! shell ) return NULL;
			/*rterror("RTGEOM2GEOS: exception during polygon shell conversion"); */
			ngeoms = rtpoly->nrings-1;
			if ( ngeoms > 0 )
				geoms = malloc(sizeof(GEOSGeom)*ngeoms);

			for (i=1; i<rtpoly->nrings; ++i)
			{
				geoms[i-1] = ptarray_to_GEOSLinearRing(rtpoly->rings[i], autofix);
				if ( ! geoms[i-1] )
				{
					--i;
					while (i) GEOSGeom_destroy(geoms[--i]);
					free(geoms);
					GEOSGeom_destroy(shell);
					return NULL;
				}
				/*rterror("RTGEOM2GEOS: exception during polygon hole conversion"); */
			}
			g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
			if (geoms) free(geoms);
		}
		if ( ! g ) return NULL;
		break;
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
		if ( rtgeom->type == RTMULTIPOINTTYPE )
			geostype = GEOS_MULTIPOINT;
		else if ( rtgeom->type == RTMULTILINETYPE )
			geostype = GEOS_MULTILINESTRING;
		else if ( rtgeom->type == RTMULTIPOLYGONTYPE )
			geostype = GEOS_MULTIPOLYGON;
		else
			geostype = GEOS_GEOMETRYCOLLECTION;

		rtc = (RTCOLLECTION *)rtgeom;

		ngeoms = rtc->ngeoms;
		if ( ngeoms > 0 )
			geoms = malloc(sizeof(GEOSGeom)*ngeoms);

		for (i=0; i<ngeoms; ++i)
		{
			GEOSGeometry* g = RTGEOM2GEOS(rtc->geoms[i], 0);
			if ( ! g )
			{
				while (i) GEOSGeom_destroy(geoms[--i]);
				free(geoms);
				return NULL;
			}
			geoms[i] = g;
		}
		g = GEOSGeom_createCollection(geostype, geoms, ngeoms);
		if ( geoms ) free(geoms);
		if ( ! g ) return NULL;
		break;

	default:
		rterror("Unknown geometry type: %d - %s", rtgeom->type, rttype_name(rtgeom->type));
		return NULL;
	}

	GEOSSetSRID(g, rtgeom->srid);

#if RTDEBUG_LEVEL >= 4
	wkt = GEOSGeomToWKT(g);
	RTDEBUGF(4, "RTGEOM2GEOS: GEOSGeom: %s", wkt);
	free(wkt);
#endif

	return g;
}

const char*
rtgeom_geos_version()
{
	const char *ver = GEOSversion();
	return ver;
}

RTGEOM *
rtgeom_normalize(const RTGEOM *geom1)
{
	RTGEOM *result ;
	GEOSGeometry *g1;
	int is3d ;
	int srid ;

	srid = (int)(geom1->srid);
	is3d = FLAGS_GET_Z(geom1->flags);

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL ;
	}

	if ( -1 == GEOSNormalize(g1) )
	{
	  rterror("Error in GEOSNormalize: %s", rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSSetSRID(g1, srid); /* needed ? */
	result = GEOS2RTGEOM(g1, is3d);
	GEOSGeom_destroy(g1);

	if (result == NULL)
	{
	  rterror("Error performing intersection: GEOS2RTGEOM: %s",
	                rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	return result ;
}

RTGEOM *
rtgeom_intersection(const RTGEOM *geom1, const RTGEOM *geom2)
{
	RTGEOM *result ;
	GEOSGeometry *g1, *g2, *g3 ;
	int is3d ;
	int srid ;

	/* A.Intersection(Empty) == Empty */
	if ( rtgeom_is_empty(geom2) )
		return rtgeom_clone_deep(geom2);

	/* Empty.Intersection(A) == Empty */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	RTDEBUG(3, "intersection() START");

	g1 = RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL ;
	}

	g2 = RTGEOM2GEOS(geom2, 0);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		rterror("Second argument geometry could not be converted to GEOS.");
		GEOSGeom_destroy(g1);
		return NULL ;
	}

	RTDEBUG(3, " constructed geometrys - calling geos");
	RTDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	RTDEBUGF(3, " g2 = %s", GEOSGeomToWKT(g2));
	/*RTDEBUGF(3, "g2 is valid = %i",GEOSisvalid(g2)); */
	/*RTDEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	g3 = GEOSIntersection(g1,g2);

	RTDEBUG(3, " intersection finished");

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		rterror("Error performing intersection: %s",
		        rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		rterror("Error performing intersection: GEOS2RTGEOM: %s",
		        rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	return result ;
}

RTGEOM *
rtgeom_linemerge(const RTGEOM *geom1)
{
	RTGEOM *result ;
	GEOSGeometry *g1, *g3 ;
	int is3d = FLAGS_GET_Z(geom1->flags);
	int srid = geom1->srid;

	/* Empty.Linemerge() == Empty */
	if ( rtgeom_is_empty(geom1) )
		return (RTGEOM*)rtcollection_construct_empty( RTCOLLECTIONTYPE, srid, is3d,
                                         rtgeom_has_m(geom1) );

	initGEOS(rtnotice, rtgeom_geos_error);

	RTDEBUG(3, "linemerge() START");

	g1 = RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL ;
	}

	RTDEBUG(3, " constructed geometrys - calling geos");
	RTDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	/*RTDEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	g3 = GEOSLineMerge(g1);

	RTDEBUG(3, " linemerge finished");

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		rterror("Error performing linemerge: %s",
		        rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		rterror("Error performing linemerge: GEOS2RTGEOM: %s",
		        rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	return result ;
}

RTGEOM *
rtgeom_unaryunion(const RTGEOM *geom1)
{
	RTGEOM *result ;
	GEOSGeometry *g1, *g3 ;
	int is3d = FLAGS_GET_Z(geom1->flags);
	int srid = geom1->srid;

	/* Empty.UnaryUnion() == Empty */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom1);

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL ;
	}

	g3 = GEOSUnaryUnion(g1);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		rterror("Error performing unaryunion: %s",
		        rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		rterror("Error performing unaryunion: GEOS2RTGEOM: %s",
		        rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	return result ;
}

RTGEOM *
rtgeom_difference(const RTGEOM *geom1, const RTGEOM *geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	RTGEOM *result;
	int is3d;
	int srid;

	/* A.Difference(Empty) == A */
	if ( rtgeom_is_empty(geom2) )
		return rtgeom_clone_deep(geom1);

	/* Empty.Intersection(A) == Empty */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = RTGEOM2GEOS(geom2, 0);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		rterror("Second argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g3 = GEOSDifference(g1,g2);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		rterror("GEOSDifference: %s", rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		rterror("Error performing difference: GEOS2RTGEOM: %s",
		        rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	return result;
}

RTGEOM *
rtgeom_symdifference(const RTGEOM* geom1, const RTGEOM* geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	RTGEOM *result;
	int is3d;
	int srid;

	/* A.SymDifference(Empty) == A */
	if ( rtgeom_is_empty(geom2) )
		return rtgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom2);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = RTGEOM2GEOS(geom1, 0);

	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = RTGEOM2GEOS(geom2, 0);

	if ( 0 == g2 )   /* exception thrown at construction */
	{
		rterror("Second argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSymDifference(g1,g2);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		rterror("GEOSSymDifference: %s", rtgeom_geos_errmsg);
		return NULL; /*never get here */
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		rterror("GEOS symdifference() threw an error (result postgis geometry formation)!");
		return NULL ; /*never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	return result;
}

RTGEOM*
rtgeom_union(const RTGEOM *geom1, const RTGEOM *geom2)
{
	int is3d;
	int srid;
	GEOSGeometry *g1, *g2, *g3;
	RTGEOM *result;

	RTDEBUG(2, "in geomunion");

	/* A.Union(empty) == A */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom2);

	/* B.Union(empty) == B */
	if ( rtgeom_is_empty(geom2) )
		return rtgeom_clone_deep(geom1);


	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = RTGEOM2GEOS(geom1, 0);

	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = RTGEOM2GEOS(geom2, 0);

	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		rterror("Second argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	RTDEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));
	RTDEBUGF(3, "g2=%s", GEOSGeomToWKT(g2));

	g3 = GEOSUnion(g1,g2);

	RTDEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		rterror("GEOSUnion: %s", rtgeom_geos_errmsg);
		return NULL; /* never get here */
	}


	GEOSSetSRID(g3, srid);

	result = GEOS2RTGEOM(g3, is3d);

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		rterror("Error performing union: GEOS2RTGEOM: %s",
		        rtgeom_geos_errmsg);
		return NULL; /*never get here */
	}

	return result;
}

RTGEOM *
rtgeom_clip_by_rect(const RTGEOM *geom1, double x0, double y0, double x1, double y1)
{
#if RTGEOM_GEOS_VERSION < 35
	rterror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSClipByRect' function (3.3.5+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 35 */
	RTGEOM *result ;
	GEOSGeometry *g1, *g3 ;
	int is3d ;

	/* A.Intersection(Empty) == Empty */
	if ( rtgeom_is_empty(geom1) )
		return rtgeom_clone_deep(geom1);

	is3d = FLAGS_GET_Z(geom1->flags);

	initGEOS(rtnotice, rtgeom_geos_error);

	RTDEBUG(3, "clip_by_rect() START");

	g1 = RTGEOM2GEOS(geom1, 1); /* auto-fix structure */
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL ;
	}

	RTDEBUG(3, " constructed geometrys - calling geos");
	RTDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	/*RTDEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	g3 = GEOSClipByRect(g1,x0,y0,x1,y1);
	GEOSGeom_destroy(g1);

	RTDEBUG(3, " clip_by_rect finished");

	if (g3 == NULL)
	{
		rtnotice("Error performing rectangular clipping: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	result = GEOS2RTGEOM(g3, is3d);
	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		rterror("Error performing intersection: GEOS2RTGEOM: %s", rtgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	result->srid = geom1->srid;

	return result ;
#endif /* RTGEOM_GEOS_VERSION >= 35 */
}


/* ------------ BuildArea stuff ---------------------------------------------------------------------{ */

typedef struct Face_t {
  const GEOSGeometry* geom;
  GEOSGeometry* env;
  double envarea;
  struct Face_t* parent; /* if this face is an hole of another one, or NULL */
} Face;

static Face* newFace(const GEOSGeometry* g);
static void delFace(Face* f);
static unsigned int countParens(const Face* f);
static void findFaceHoles(Face** faces, int nfaces);

static Face*
newFace(const GEOSGeometry* g)
{
  Face* f = rtalloc(sizeof(Face));
  f->geom = g;
  f->env = GEOSEnvelope(f->geom);
  GEOSArea(f->env, &f->envarea);
  f->parent = NULL;
  /* rtnotice("Built Face with area %g and %d holes", f->envarea, GEOSGetNumInteriorRings(f->geom)); */
  return f;
}

static unsigned int
countParens(const Face* f)
{
  unsigned int pcount = 0;
  while ( f->parent ) {
    ++pcount;
    f = f->parent;
  }
  return pcount;
}

/* Destroy the face and release memory associated with it */
static void
delFace(Face* f)
{
  GEOSGeom_destroy(f->env);
  rtfree(f);
}


static int
compare_by_envarea(const void* g1, const void* g2)
{
  Face* f1 = *(Face**)g1;
  Face* f2 = *(Face**)g2;
  double n1 = f1->envarea;
  double n2 = f2->envarea;

  if ( n1 < n2 ) return 1;
  if ( n1 > n2 ) return -1;
  return 0;
}

/* Find holes of each face */
static void
findFaceHoles(Face** faces, int nfaces)
{
  int i, j, h;

  /* We sort by envelope area so that we know holes are only
   * after their shells */
  qsort(faces, nfaces, sizeof(Face*), compare_by_envarea);
  for (i=0; i<nfaces; ++i) {
    Face* f = faces[i];
    int nholes = GEOSGetNumInteriorRings(f->geom);
    RTDEBUGF(2, "Scanning face %d with env area %g and %d holes", i, f->envarea, nholes);
    for (h=0; h<nholes; ++h) {
      const GEOSGeometry *hole = GEOSGetInteriorRingN(f->geom, h);
      RTDEBUGF(2, "Looking for hole %d/%d of face %d among %d other faces", h+1, nholes, i, nfaces-i-1);
      for (j=i+1; j<nfaces; ++j) {
		const GEOSGeometry *f2er;
        Face* f2 = faces[j];
        if ( f2->parent ) continue; /* hole already assigned */
        f2er = GEOSGetExteriorRing(f2->geom); 
        /* TODO: can be optimized as the ring would have the
         *       same vertices, possibly in different order.
         *       maybe comparing number of points could already be
         *       useful.
         */
        if ( GEOSEquals(f2er, hole) ) {
          RTDEBUGF(2, "Hole %d/%d of face %d is face %d", h+1, nholes, i, j);
          f2->parent = f;
          break;
        }
      }
    }
  }
}

static GEOSGeometry*
collectFacesWithEvenAncestors(Face** faces, int nfaces)
{
  GEOSGeometry **geoms = rtalloc(sizeof(GEOSGeometry*)*nfaces);
  GEOSGeometry *ret;
  unsigned int ngeoms = 0;
  int i;

  for (i=0; i<nfaces; ++i) {
    Face *f = faces[i];
    if ( countParens(f) % 2 ) continue; /* we skip odd parents geoms */
    geoms[ngeoms++] = GEOSGeom_clone(f->geom);
  }

  ret = GEOSGeom_createCollection(GEOS_MULTIPOLYGON, geoms, ngeoms);
  rtfree(geoms);
  return ret;
}

GEOSGeometry*
RTGEOM_GEOS_buildArea(const GEOSGeometry* geom_in)
{
  GEOSGeometry *tmp;
  GEOSGeometry *geos_result, *shp;
  GEOSGeometry const *vgeoms[1];
  uint32_t i, ngeoms;
  int srid = GEOSGetSRID(geom_in);
  Face ** geoms;

  vgeoms[0] = geom_in;
#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Polygonizing");
#endif
  geos_result = GEOSPolygonize(vgeoms, 1);

  RTDEBUGF(3, "GEOSpolygonize returned @ %p", geos_result);

  /* Null return from GEOSpolygonize (an exception) */
  if ( ! geos_result ) return 0;

  /*
   * We should now have a collection
   */
#if PARANOIA_LEVEL > 0
  if ( GEOSGeometryTypeId(geos_result) != RTCOLLECTIONTYPE )
  {
    GEOSGeom_destroy(geos_result);
    rterror("Unexpected return from GEOSpolygonize");
    return 0;
  }
#endif

  ngeoms = GEOSGetNumGeometries(geos_result);
#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Num geometries from polygonizer: %d", ngeoms);
#endif


  RTDEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
  RTDEBUGF(3, "GEOSpolygonize: polygonized:%s",
              rtgeom_to_ewkt(GEOS2RTGEOM(geos_result, 0)));

  /*
   * No geometries in collection, early out
   */
  if ( ngeoms == 0 )
  {
    GEOSSetSRID(geos_result, srid);
    return geos_result;
  }

  /*
   * Return first geometry if we only have one in collection,
   * to avoid the unnecessary Geometry clone below.
   */
  if ( ngeoms == 1 )
  {
    tmp = (GEOSGeometry *)GEOSGetGeometryN(geos_result, 0);
    if ( ! tmp )
    {
      GEOSGeom_destroy(geos_result);
      return 0; /* exception */
    }
    shp = GEOSGeom_clone(tmp);
    GEOSGeom_destroy(geos_result); /* only safe after the clone above */
    GEOSSetSRID(shp, srid);
    return shp;
  }

  RTDEBUGF(2, "Polygonize returned %d geoms", ngeoms);

  /*
   * Polygonizer returns a polygon for each face in the built topology.
   *
   * This means that for any face with holes we'll have other faces
   * representing each hole. We can imagine a parent-child relationship
   * between these faces.
   *
   * In order to maximize the number of visible rings in output we
   * only use those faces which have an even number of parents.
   *
   * Example:
   *
   *   +---------------+
   *   |     L0        |  L0 has no parents 
   *   |  +---------+  |
   *   |  |   L1    |  |  L1 is an hole of L0
   *   |  |  +---+  |  |
   *   |  |  |L2 |  |  |  L2 is an hole of L1 (which is an hole of L0)
   *   |  |  |   |  |  |
   *   |  |  +---+  |  |
   *   |  +---------+  |
   *   |               |
   *   +---------------+
   * 
   * See http://trac.osgeo.org/postgis/ticket/1806
   *
   */

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Preparing face structures");
#endif

  /* Prepare face structures for later analysis */
  geoms = rtalloc(sizeof(Face**)*ngeoms);
  for (i=0; i<ngeoms; ++i)
    geoms[i] = newFace(GEOSGetGeometryN(geos_result, i));

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Finding face holes");
#endif

  /* Find faces representing other faces holes */
  findFaceHoles(geoms, ngeoms);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Colletting even ancestor faces");
#endif

  /* Build a MultiPolygon composed only by faces with an
   * even number of ancestors */
  tmp = collectFacesWithEvenAncestors(geoms, ngeoms);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Cleaning up");
#endif

  /* Cleanup face structures */
  for (i=0; i<ngeoms; ++i) delFace(geoms[i]);
  rtfree(geoms);

  /* Faces referenced memory owned by geos_result.
   * It is safe to destroy geos_result after deleting them. */
  GEOSGeom_destroy(geos_result);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Self-unioning");
#endif

  /* Run a single overlay operation to dissolve shared edges */
  shp = GEOSUnionCascaded(tmp);
  if ( ! shp )
  {
    GEOSGeom_destroy(tmp);
    return 0; /* exception */
  }

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice("Final cleanup");
#endif

  GEOSGeom_destroy(tmp);

  GEOSSetSRID(shp, srid);

  return shp;
}

RTGEOM*
rtgeom_buildarea(const RTGEOM *geom)
{
	GEOSGeometry* geos_in;
	GEOSGeometry* geos_out;
	RTGEOM* geom_out;
	int SRID = (int)(geom->srid);
	int is3d = FLAGS_GET_Z(geom->flags);

	/* Can't build an area from an empty! */
	if ( rtgeom_is_empty(geom) )
	{
		return (RTGEOM*)rtpoly_construct_empty(SRID, is3d, 0);
	}

	RTDEBUG(3, "buildarea called");

	RTDEBUGF(3, "ST_BuildArea got geom @ %p", geom);

	initGEOS(rtnotice, rtgeom_geos_error);

	geos_in = RTGEOM2GEOS(geom, 0);
	
	if ( 0 == geos_in )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}
	geos_out = RTGEOM_GEOS_buildArea(geos_in);
	GEOSGeom_destroy(geos_in);

	if ( ! geos_out ) /* exception thrown.. */
	{
		rterror("RTGEOM_GEOS_buildArea: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* If no geometries are in result collection, return NULL */
	if ( GEOSGetNumGeometries(geos_out) == 0 )
	{
		GEOSGeom_destroy(geos_out);
		return NULL;
	}

	geom_out = GEOS2RTGEOM(geos_out, is3d);
	GEOSGeom_destroy(geos_out);

#if PARANOIA_LEVEL > 0
	if ( geom_out == NULL )
	{
		rterror("serialization error");
		return NULL;
	}

#endif

	return geom_out;
}

int
rtgeom_is_simple(const RTGEOM *geom)
{
	GEOSGeometry* geos_in;
	int simple;

	/* Empty is artays simple */
	if ( rtgeom_is_empty(geom) )
	{
		return 1;
	}

	initGEOS(rtnotice, rtgeom_geos_error);

	geos_in = RTGEOM2GEOS(geom, 0);
	if ( 0 == geos_in )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return -1;
	}
	simple = GEOSisSimple(geos_in);
	GEOSGeom_destroy(geos_in);

	if ( simple == 2 ) /* exception thrown */
	{
		rterror("rtgeom_is_simple: %s", rtgeom_geos_errmsg);
		return -1;
	}

	return simple ? 1 : 0;
}

/* ------------ end of BuildArea stuff ---------------------------------------------------------------------} */

RTGEOM*
rtgeom_geos_noop(const RTGEOM* geom_in)
{
	GEOSGeometry *geosgeom;
	RTGEOM* geom_out;

	int is3d = FLAGS_GET_Z(geom_in->flags);

	initGEOS(rtnotice, rtgeom_geos_error);
	geosgeom = RTGEOM2GEOS(geom_in, 0);
	if ( ! geosgeom ) {
		rterror("Geometry could not be converted to GEOS: %s",
			rtgeom_geos_errmsg);
		return NULL;
	}
	geom_out = GEOS2RTGEOM(geosgeom, is3d);
	GEOSGeom_destroy(geosgeom);
	if ( ! geom_out ) {
		rterror("GEOS Geometry could not be converted to RTGEOM: %s",
			rtgeom_geos_errmsg);
	}
	return geom_out;
	
}

RTGEOM*
rtgeom_snap(const RTGEOM* geom1, const RTGEOM* geom2, double tolerance)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror("The GEOS version this rtgeom library "
	        "was compiled against (%d) doesn't support "
	        "'Snap' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */

	int srid, is3d;
	GEOSGeometry *g1, *g2, *g3;
	RTGEOM* out;

	srid = geom1->srid;
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = (GEOSGeometry *)RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = (GEOSGeometry *)RTGEOM2GEOS(geom2, 0);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		rterror("Second argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSnap(g1, g2, tolerance);
	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		rterror("GEOSSnap: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	GEOSSetSRID(g3, srid);
	out = GEOS2RTGEOM(g3, is3d);
	if (out == NULL)
	{
		GEOSGeom_destroy(g3);
		rterror("GEOSSnap() threw an error (result RTGEOM geometry formation)!");
		return NULL;
	}
	GEOSGeom_destroy(g3);

	return out;

#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

RTGEOM*
rtgeom_sharedpaths(const RTGEOM* geom1, const RTGEOM* geom2)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'SharedPaths' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */
	GEOSGeometry *g1, *g2, *g3;
	RTGEOM *out;
	int is3d, srid;

	srid = geom1->srid;
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = (GEOSGeometry *)RTGEOM2GEOS(geom1, 0);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		rterror("First argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = (GEOSGeometry *)RTGEOM2GEOS(geom2, 0);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		rterror("Second argument geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSharedPaths(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		rterror("GEOSSharedPaths: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	GEOSSetSRID(g3, srid);
	out = GEOS2RTGEOM(g3, is3d);
	GEOSGeom_destroy(g3);

	if (out == NULL)
	{
		rterror("GEOS2RTGEOM threw an error");
		return NULL;
	}

	return out;
#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

RTGEOM*
rtgeom_offsetcurve(const RTLINE *rtline, double size, int quadsegs, int joinStyle, double mitreLimit)
{
#if RTGEOM_GEOS_VERSION < 32
	rterror("rtgeom_offsetcurve: GEOS 3.2 or higher required");
#else
	GEOSGeometry *g1, *g3;
	RTGEOM *rtgeom_result;
	RTGEOM *rtgeom_in = rtline_as_rtgeom(rtline);

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = (GEOSGeometry *)RTGEOM2GEOS(rtgeom_in, 0);
	if ( ! g1 ) 
	{
		rterror("rtgeom_offsetcurve: Geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

#if RTGEOM_GEOS_VERSION < 33
	/* Size is artays positive for GEOSSingleSidedBuffer, and a flag determines left/right */
	g3 = GEOSSingleSidedBuffer(g1, size < 0 ? -size : size,
	                           quadsegs, joinStyle, mitreLimit,
	                           size < 0 ? 0 : 1);
#else
	g3 = GEOSOffsetCurve(g1, size, quadsegs, joinStyle, mitreLimit);
#endif
	/* Don't need input geometry anymore */
	GEOSGeom_destroy(g1);

	if (g3 == NULL)
	{
		rterror("GEOSOffsetCurve: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, rtgeom_get_srid(rtgeom_in));
	rtgeom_result = GEOS2RTGEOM(g3, rtgeom_has_z(rtgeom_in));
	GEOSGeom_destroy(g3);

	if (rtgeom_result == NULL)
	{
		rterror("rtgeom_offsetcurve: GEOS2RTGEOM returned null");
		return NULL;
	}

	return rtgeom_result;
	
#endif /* RTGEOM_GEOS_VERSION < 32 */
}

RTTIN *rttin_from_geos(const GEOSGeometry *geom, int want3d) {
	int type = GEOSGeomTypeId(geom);
	int hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if ( SRID == 0 ) SRID = SRID_UNKNOWN;

	if ( want3d ) {
		hasZ = GEOSHasZ(geom);
		if ( ! hasZ ) {
			RTDEBUG(3, "Geometry has no Z, won't provide one");
			want3d = 0;
		}
	}

	switch (type) {
		RTTRIANGLE **geoms;
		uint32_t i, ngeoms;
	case GEOS_GEOMETRYCOLLECTION:
		RTDEBUG(4, "rtgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if ( ngeoms ) {
			geoms = rtalloc(ngeoms * sizeof *geoms);
			if (!geoms) {
				rterror("rttin_from_geos: can't allocate geoms");
				return NULL;
			}
			for (i=0; i<ngeoms; i++) {
				const GEOSGeometry *poly, *ring;
				const GEOSCoordSequence *cs;
				POINTARRAY *pa;

				poly = GEOSGetGeometryN(geom, i);
				ring = GEOSGetExteriorRing(poly);
				cs = GEOSGeom_getCoordSeq(ring);
				pa = ptarray_from_GEOSCoordSeq(cs, want3d);

				geoms[i] = rttriangle_construct(SRID, NULL, pa);
			}
		}
		return (RTTIN *)rtcollection_construct(RTTINTYPE, SRID, NULL, ngeoms, (RTGEOM **)geoms);
	case GEOS_POLYGON:
	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
	case GEOS_POINT:
		rterror("rttin_from_geos: invalid geometry type for tin: %d", type);
		break;

	default:
		rterror("GEOS2RTGEOM: unknown geometry type: %d", type);
		return NULL;
	}

	/* shouldn't get here */
	return NULL;
}
/*
 * output = 1 for edges, 2 for TIN, 0 for polygons
 */
RTGEOM* rtgeom_delaunay_triangulation(const RTGEOM *rtgeom_in, double tolerance, int output) {
#if RTGEOM_GEOS_VERSION < 34
	rterror("rtgeom_delaunay_triangulation: GEOS 3.4 or higher required");
	return NULL;
#else
	GEOSGeometry *g1, *g3;
	RTGEOM *rtgeom_result;

	if (output < 0 || output > 2) {
		rterror("rtgeom_delaunay_triangulation: invalid output type specified %d", output);
		return NULL;
	}

	initGEOS(rtnotice, rtgeom_geos_error);

	g1 = (GEOSGeometry *)RTGEOM2GEOS(rtgeom_in, 0);
	if ( ! g1 ) 
	{
		rterror("rtgeom_delaunay_triangulation: Geometry could not be converted to GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* if output != 1 we want polys */
	g3 = GEOSDelaunayTriangulation(g1, tolerance, output == 1);

	/* Don't need input geometry anymore */
	GEOSGeom_destroy(g1);

	if (g3 == NULL)
	{
		rterror("GEOSDelaunayTriangulation: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* RTDEBUGF(3, "result: %s", GEOSGeomToWKT(g3)); */

	GEOSSetSRID(g3, rtgeom_get_srid(rtgeom_in));

	if (output == 2) {
		rtgeom_result = (RTGEOM *)rttin_from_geos(g3, rtgeom_has_z(rtgeom_in));
	} else {
		rtgeom_result = GEOS2RTGEOM(g3, rtgeom_has_z(rtgeom_in));
	}

	GEOSGeom_destroy(g3);

	if (rtgeom_result == NULL) {
		if (output != 2) {
			rterror("rtgeom_delaunay_triangulation: GEOS2RTGEOM returned null");
		} else {
			rterror("rtgeom_delaunay_triangulation: rttin_from_geos returned null");
		}
		return NULL;
	}

	return rtgeom_result;
	
#endif /* RTGEOM_GEOS_VERSION < 34 */
}