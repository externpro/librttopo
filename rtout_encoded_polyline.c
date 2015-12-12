/**********************************************************************
*
* rttopo - topology library
* http://gitlab.com/rttopo/rttopo
*
* Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com> and
*                Shoaib Burq <saburq@gmail.com>
*
* This is free software; you can redistribute and/or modify it under
* the terms of the GNU General Public Licence. See the COPYING file.
*
**********************************************************************/

#include "stringbuffer.h"
#include "librtgeom_internal.h"

static char * rtline_to_encoded_polyline(const RTCTX *ctx, const RTLINE*, int precision);
static char * rtmmpoint_to_encoded_polyline(const RTCTX *ctx, const RTMPOINT*, int precision);
static char * pointarray_to_encoded_polyline(const RTCTX *ctx, const RTPOINTARRAY*, int precision);

/* takes a GEOMETRY and returns an Encoded Polyline representation */
extern char *
rtgeom_to_encoded_polyline(const RTCTX *ctx, const RTGEOM *geom, int precision)
{
	int type = geom->type;
	switch (type)
	{
	case RTLINETYPE:
		return rtline_to_encoded_polyline(ctx, (RTLINE*)geom, precision);
	case RTMULTIPOINTTYPE:
		return rtmmpoint_to_encoded_polyline(ctx, (RTMPOINT*)geom, precision);
	default:
		rterror(ctx, "rtgeom_to_encoded_polyline: '%s' geometry type not supported", rttype_name(ctx, type));
		return NULL;
	}
}

static
char * rtline_to_encoded_polyline(const RTCTX *ctx, const RTLINE *line, int precision)
{
	return pointarray_to_encoded_polyline(ctx, line->points, precision);
}

static
char * rtmmpoint_to_encoded_polyline(const RTCTX *ctx, const RTMPOINT *mpoint, int precision)
{
	RTLINE *line = rtline_from_rtmpoint(ctx, mpoint->srid, mpoint);
	char *encoded_polyline = rtline_to_encoded_polyline(ctx, line, precision);

	rtline_free(ctx, line);
	return encoded_polyline;
}

static
char * pointarray_to_encoded_polyline(const RTCTX *ctx, const RTPOINTARRAY *pa, int precision)
{
	int i;
	const RTPOINT2D *prevPoint;
	int *delta = rtalloc(ctx, 2*sizeof(int)*pa->npoints);
	char *encoded_polyline = NULL;
	stringbuffer_t *sb;
	double scale = pow(10,precision);

	/* Take the double value and multiply it by 1x10^percision, rounding the result */
	prevPoint = rt_getPoint2d_cp(ctx, pa, 0);
	delta[0] = round(prevPoint->y*scale);
	delta[1] = round(prevPoint->x*scale);

	/*  points only include the offset from the previous point */
	for (i=1; i<pa->npoints; i++)
	{
		const RTPOINT2D *point = rt_getPoint2d_cp(ctx, pa, i);
		delta[2*i] = round(point->y*scale) - round(prevPoint->y*scale);
		delta[(2*i)+1] = round(point->x*scale) - round(prevPoint->x*scale);
		prevPoint = point;
	}

	/* value to binary: a negative value must be calculated using its two's complement */
	for (i=0; i<pa->npoints*2; i++)
	{
		/* Left-shift the binary value one bit */
		delta[i] <<= 1;
		/* if value is negative, invert this encoding */
		if (delta[i] < 0) {
			delta[i] = ~(delta[i]);
		}
	}

	sb = stringbuffer_create(ctx);
	for (i=0; i<pa->npoints*2; i++)
	{
		int numberToEncode = delta[i];

		while (numberToEncode >= 0x20) {
			/* Place the 5-bit chunks into reverse order or
			 each value with 0x20 if another bit chunk follows and add 63*/
			int nextValue = (0x20 | (numberToEncode & 0x1f)) + 63;
			stringbuffer_aprintf(ctx, sb, "%c", (char)nextValue);
			if(92 == nextValue)
				stringbuffer_aprintf(ctx, sb, "%c", (char)nextValue);

			/* Break the binary value out into 5-bit chunks */
			numberToEncode >>= 5;
		}

		numberToEncode += 63;
		stringbuffer_aprintf(ctx, sb, "%c", (char)numberToEncode);
		if(92 == numberToEncode)
			stringbuffer_aprintf(ctx, sb, "%c", (char)numberToEncode);
	}

	rtfree(ctx, delta);
	encoded_polyline = stringbuffer_getstringcopy(ctx, sb);
	stringbuffer_destroy(ctx, sb);

	return encoded_polyline;
}
