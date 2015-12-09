/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"

void
rtmline_release(RTMLINE *rtmline)
{
	rtgeom_release(rtmline_as_rtgeom(rtmline));
}

RTMLINE *
rtmline_construct_empty(int srid, char hasz, char hasm)
{
	RTMLINE *ret = (RTMLINE*)rtcollection_construct_empty(RTMULTILINETYPE, srid, hasz, hasm);
	return ret;
}



RTMLINE* rtmline_add_rtline(RTMLINE *mobj, const RTLINE *obj)
{
	return (RTMLINE*)rtcollection_add_rtgeom((RTCOLLECTION*)mobj, (RTGEOM*)obj);
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
RTMLINE*
rtmline_measured_from_rtmline(const RTMLINE *rtmline, double m_start, double m_end)
{
	int i = 0;
	int hasm = 0, hasz = 0;
	double length = 0.0, length_so_far = 0.0;
	double m_range = m_end - m_start;
	RTGEOM **geoms = NULL;

	if ( rtmline->type != RTMULTILINETYPE )
	{
		rterror("rtmline_measured_from_lmwline: only multiline types supported");
		return NULL;
	}

	hasz = RTFLAGS_GET_Z(rtmline->flags);
	hasm = 1;

	/* Calculate the total length of the mline */
	for ( i = 0; i < rtmline->ngeoms; i++ )
	{
		RTLINE *rtline = (RTLINE*)rtmline->geoms[i];
		if ( rtline->points && rtline->points->npoints > 1 )
		{
			length += ptarray_length_2d(rtline->points);
		}
	}

	if ( rtgeom_is_empty((RTGEOM*)rtmline) )
	{
		return (RTMLINE*)rtcollection_construct_empty(RTMULTILINETYPE, rtmline->srid, hasz, hasm);
	}

	geoms = rtalloc(sizeof(RTGEOM*) * rtmline->ngeoms);

	for ( i = 0; i < rtmline->ngeoms; i++ )
	{
		double sub_m_start, sub_m_end;
		double sub_length = 0.0;
		RTLINE *rtline = (RTLINE*)rtmline->geoms[i];

		if ( rtline->points && rtline->points->npoints > 1 )
		{
			sub_length = ptarray_length_2d(rtline->points);
		}

		sub_m_start = (m_start + m_range * length_so_far / length);
		sub_m_end = (m_start + m_range * (length_so_far + sub_length) / length);

		geoms[i] = (RTGEOM*)rtline_measured_from_rtline(rtline, sub_m_start, sub_m_end);

		length_so_far += sub_length;
	}

	return (RTMLINE*)rtcollection_construct(rtmline->type, rtmline->srid, NULL, rtmline->ngeoms, geoms);
}

void rtmline_free(RTMLINE *mline)
{
	int i;
	if ( ! mline ) return;
	
	if ( mline->bbox )
		rtfree(mline->bbox);

	for ( i = 0; i < mline->ngeoms; i++ )
		if ( mline->geoms && mline->geoms[i] )
			rtline_free(mline->geoms[i]);

	if ( mline->geoms )
		rtfree(mline->geoms);

	rtfree(mline);
}
