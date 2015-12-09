/**********************************************************************
 *
 * rttopo - topology library
 *
 * Copyright (C) 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "rtout_twkb.h"

/*
* GeometryType, and dimensions
*/
static uint8_t rtgeom_twkb_type(const RTGEOM *geom)
{
	uint8_t twkb_type = 0;

	RTDEBUGF(2, "Entered  rtgeom_twkb_type",0);

	switch ( geom->type )
	{
		case RTPOINTTYPE:
			twkb_type = RTWKB_POINT_TYPE;
			break;
		case RTLINETYPE:
			twkb_type = RTWKB_LINESTRING_TYPE;
			break;
		case RTPOLYGONTYPE:
			twkb_type = RTWKB_POLYGON_TYPE;
			break;
		case RTMULTIPOINTTYPE:
			twkb_type = RTWKB_MULTIPOINT_TYPE;
			break;
		case RTMULTILINETYPE:
			twkb_type = RTWKB_MULTILINESTRING_TYPE;
			break;
		case RTMULTIPOLYGONTYPE:
			twkb_type = RTWKB_MULTIPOLYGON_TYPE;
			break;
		case RTCOLLECTIONTYPE:
			twkb_type = RTWKB_GEOMETRYCOLLECTION_TYPE;
			break;
		default:
			rterror("Unsupported geometry type: %s [%d]",
				rttype_name(geom->type), geom->type);
	}
	return twkb_type;
}


/**
* Calculates the size of the bbox in varints in the form:
* xmin, xdelta, ymin, ydelta
*/
static size_t sizeof_bbox(TWKB_STATE *ts, int ndims)
{
	int i;
	uint8_t buf[16];
	size_t size = 0;
	RTDEBUGF(2, "Entered %s", __func__);
	for ( i = 0; i < ndims; i++ )
	{
		size += varint_s64_encode_buf(ts->bbox_min[i], buf);
		size += varint_s64_encode_buf((ts->bbox_max[i] - ts->bbox_min[i]), buf);
	}
	return size;
}
/**
* Writes the bbox in varints in the form:
* xmin, xdelta, ymin, ydelta
*/
static void write_bbox(TWKB_STATE *ts, int ndims)
{
	int i;
	RTDEBUGF(2, "Entered %s", __func__);
	for ( i = 0; i < ndims; i++ )
	{
		bytebuffer_append_varint(ts->header_buf, ts->bbox_min[i]);
		bytebuffer_append_varint(ts->header_buf, (ts->bbox_max[i] - ts->bbox_min[i]));
	}
}


/**
* Stores a pointarray as varints in the buffer
* @register_npoints, controls whether an npoints entry is added to the buffer (used to skip npoints for point types)
* @dimension, states the dimensionality of object this array is part of (0 = point, 1 = linear, 2 = areal)
*/
static int ptarray_to_twkb_buf(const POINTARRAY *pa, TWKB_GLOBALS *globals, TWKB_STATE *ts, int register_npoints, int minpoints)
{
	int ndims = FLAGS_NDIMS(pa->flags);
	int i, j;
	bytebuffer_t b;
	bytebuffer_t *b_p;
	int64_t nextdelta[MAX_N_DIMS];
	int npoints = 0;
	size_t npoints_offset = 0;

	RTDEBUGF(2, "Entered %s", __func__);

	/* Dispense with the empty case right away */
	if ( pa->npoints == 0 && register_npoints )	
	{		
		RTDEBUGF(4, "Register npoints:%d", pa->npoints);
		bytebuffer_append_uvarint(ts->geom_buf, pa->npoints);
		return 0;
	}

	/* If npoints is more than 127 it is unpredictable how many bytes npoints will need */
	/* Then we have to store the deltas in a temp buffer to later add them after npoints */
	/* If noints is below 128 we know 1 byte will be needed */
	/* Then we can make room for that 1 byte at once and write to */
	/* ordinary buffer */
	if( pa->npoints > 127 )
	{
		/* Independent buffer to hold the coordinates, so we can put the npoints */
		/* into the stream once we know how many points we actually have */
		bytebuffer_init_with_size(&b, 3 * ndims * pa->npoints);
		b_p = &b;
	}
	else
	{
		/* We give an alias to our ordinary buffer */
		b_p = ts->geom_buf;
		if ( register_npoints )		
		{		
			/* We do not store a pointer to the place where we want the npoints value */
			/* Instead we store how far from the beginning of the buffer we want the value */
			/* That is because we otherwise will get in trouble if the buffer is reallocated */			
			npoints_offset = b_p->writecursor - b_p->buf_start;
			
			/* We just move the cursor 1 step to make room for npoints byte */
			/* We use the function append_byte even if we have no value yet, */
			/* since that gives us the check for big enough buffer and moves the cursor */
			bytebuffer_append_byte(b_p, 0);
		}
	}
	
	for ( i = 0; i < pa->npoints; i++ )
	{
		double *dbl_ptr = (double*)getPoint_internal(pa, i);
		int diff = 0;

		/* Write this coordinate to the buffer as a varint */
		for ( j = 0; j < ndims; j++ )
		{
			/* To get the relative coordinate we don't get the distance */
			/* from the last point but instead the distance from our */
			/* last accumulated point. This is important to not build up an */
			/* accumulated error when rounding the coordinates */
			nextdelta[j] = (int64_t) llround(globals->factor[j] * dbl_ptr[j]) - ts->accum_rels[j];
			RTDEBUGF(4, "deltavalue: %d, ", nextdelta[j]);
			diff += llabs(nextdelta[j]);
		}
		
		/* Skipping the first point is not allowed */
		/* If the sum(abs()) of all the deltas was zero, */
		/* then this was a duplicate point, so we can ignore it */
		if ( i > minpoints && diff == 0 )
			continue;
		
		/* We really added a point, so... */
		npoints++;
		
		/* Write this vertex to the temporary buffer as varints */
		for ( j = 0; j < ndims; j++ )
		{
			ts->accum_rels[j] += nextdelta[j];
			bytebuffer_append_varint(b_p, nextdelta[j]);
		}

		/* See if this coordinate expands the bounding box */
		if( globals->variant & TWKB_BBOX )
		{
			for ( j = 0; j < ndims; j++ )
			{
				if( ts->accum_rels[j] > ts->bbox_max[j] )
					ts->bbox_max[j] = ts->accum_rels[j];

				if( ts->accum_rels[j] < ts->bbox_min[j] )
					ts->bbox_min[j] = ts->accum_rels[j];
			}
		}

	}	

	if ( pa->npoints > 127 )
	{		
		/* Now write the temporary results into the main buffer */
		/* First the npoints */
		if ( register_npoints )	
			bytebuffer_append_uvarint(ts->geom_buf, npoints);
		/* Now the coordinates */
		bytebuffer_append_bytebuffer(ts->geom_buf, b_p);
		
		/* Clear our temporary buffer */
		rtfree(b.buf_start);
	}
	else
	{
		/* If we didn't use a temp buffer, we just write that npoints value */
		/* to where it belongs*/
		if ( register_npoints )	
			varint_u64_encode_buf(npoints, b_p->buf_start + npoints_offset);
	}
	
	return 0;
}

/******************************************************************
* POINTS
*******************************************************************/

static int rtpoint_to_twkb_buf(const RTPOINT *pt, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	RTDEBUGF(2, "Entered %s", __func__);

	/* Set the coordinates (don't write npoints) */
	ptarray_to_twkb_buf(pt->point, globals, ts, 0, 1);
	return 0;
}

/******************************************************************
* LINESTRINGS
*******************************************************************/

static int rtline_to_twkb_buf(const RTLINE *line, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	RTDEBUGF(2, "Entered %s", __func__);

	/* Set the coordinates (do write npoints) */
	ptarray_to_twkb_buf(line->points, globals, ts, 1, 2);
	return 0;
}

/******************************************************************
* POLYGONS
*******************************************************************/

static int rtpoly_to_twkb_buf(const RTPOLY *poly, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	int i;

	/* Set the number of rings */
	bytebuffer_append_uvarint(ts->geom_buf, (uint64_t) poly->nrings);

	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Set the coordinates (do write npoints) */
		ptarray_to_twkb_buf(poly->rings[i], globals, ts, 1, 4);
	}

	return 0;
}



/******************************************************************
* MULTI-GEOMETRYS (MultiPoint, MultiLinestring, MultiPolygon)
*******************************************************************/

static int rtmulti_to_twkb_buf(const RTCOLLECTION *col, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	int i;
	int nempty = 0;

	RTDEBUGF(2, "Entered %s", __func__);
	RTDEBUGF(4, "Number of geometries in multi is %d", col->ngeoms);

	/* Deal with special case for MULTIPOINT: skip any empty points */
	if ( col->type == RTMULTIPOINTTYPE )
	{
		for ( i = 0; i < col->ngeoms; i++ )
			if ( rtgeom_is_empty(col->geoms[i]) )
				nempty++;
	}

	/* Set the number of geometries */
	bytebuffer_append_uvarint(ts->geom_buf, (uint64_t) (col->ngeoms - nempty));

	/* We've been handed an idlist, so write it in */
	if ( ts->idlist )
	{
		for ( i = 0; i < col->ngeoms; i++ )
		{
			/* Skip empty points in multipoints, we can't represent them */
			if ( col->type == RTMULTIPOINTTYPE && rtgeom_is_empty(col->geoms[i]) )
				continue;
			
			bytebuffer_append_varint(ts->geom_buf, ts->idlist[i]);
		}
		
		/* Empty it out to nobody else uses it now */
		ts->idlist = NULL;
	}

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* Skip empty points in multipoints, we can't represent them */
		if ( col->type == RTMULTIPOINTTYPE && rtgeom_is_empty(col->geoms[i]) )
			continue;

		rtgeom_to_twkb_buf(col->geoms[i], globals, ts);
	}
	return 0;
}

/******************************************************************
* GEOMETRYCOLLECTIONS
*******************************************************************/

static int rtcollection_to_twkb_buf(const RTCOLLECTION *col, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	int i;

	RTDEBUGF(2, "Entered %s", __func__);
	RTDEBUGF(4, "Number of geometries in collection is %d", col->ngeoms);

	/* Set the number of geometries */
	bytebuffer_append_uvarint(ts->geom_buf, (uint64_t) col->ngeoms);

	/* We've been handed an idlist, so write it in */
	if ( ts->idlist )
	{
		for ( i = 0; i < col->ngeoms; i++ )
			bytebuffer_append_varint(ts->geom_buf, ts->idlist[i]);
		
		/* Empty it out to nobody else uses it now */
		ts->idlist = NULL;
	}

	/* Write in the sub-geometries */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		rtgeom_write_to_buffer(col->geoms[i], globals, ts);
	}
	return 0;
}


/******************************************************************
* Handle whole TWKB
*******************************************************************/

static int rtgeom_to_twkb_buf(const RTGEOM *geom, TWKB_GLOBALS *globals, TWKB_STATE *ts)
{
	RTDEBUGF(2, "Entered %s", __func__);

	switch ( geom->type )
	{
		case RTPOINTTYPE:
		{
			RTDEBUGF(4,"Type found is Point, %d", geom->type);
			return rtpoint_to_twkb_buf((RTPOINT*) geom, globals, ts);
		}
		case RTLINETYPE:
		{
			RTDEBUGF(4,"Type found is Linestring, %d", geom->type);
			return rtline_to_twkb_buf((RTLINE*) geom, globals, ts);
		}
		/* Polygon has 'nrings' and 'rings' elements */
		case RTPOLYGONTYPE:
		{
			RTDEBUGF(4,"Type found is Polygon, %d", geom->type);
			return rtpoly_to_twkb_buf((RTPOLY*)geom, globals, ts);
		}

		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		{
			RTDEBUGF(4,"Type found is Multi, %d", geom->type);
			return rtmulti_to_twkb_buf((RTCOLLECTION*)geom, globals, ts);
		}
		case RTCOLLECTIONTYPE:
		{
			RTDEBUGF(4,"Type found is collection, %d", geom->type);
			return rtcollection_to_twkb_buf((RTCOLLECTION*) geom, globals, ts);
		}
		/* Unknown type! */
		default:
			rterror("Unsupported geometry type: %s [%d]", rttype_name((geom)->type), (geom)->type);
	}

	return 0;
}


static int rtgeom_write_to_buffer(const RTGEOM *geom, TWKB_GLOBALS *globals, TWKB_STATE *parent_state)
{
	int i, is_empty, has_z, has_m, ndims;
	size_t bbox_size = 0, optional_precision_byte = 0;
	uint8_t flag = 0, type_prec = 0;

	TWKB_STATE child_state;
	memset(&child_state, 0, sizeof(TWKB_STATE));
	child_state.header_buf = bytebuffer_create_with_size(16);
	child_state.geom_buf = bytebuffer_create_with_size(64);
	child_state.idlist = parent_state->idlist;

	/* Read dimensionality from input */
	has_z = rtgeom_has_z(geom);
	has_m = rtgeom_has_m(geom);
	ndims = rtgeom_ndims(geom);
	is_empty = rtgeom_is_empty(geom);

	/* Do we need extended precision? If we have a Z or M we do. */
	optional_precision_byte = (has_z || has_m);

	/* Both X and Y dimension use the same precision */
	globals->factor[0] = pow(10, globals->prec_xy);
	globals->factor[1] = globals->factor[0];

	/* Z and M dimensions have their own precisions */
	if ( has_z )
		globals->factor[2] = pow(10, globals->prec_z);
	if ( has_m )
		globals->factor[2 + has_z] = pow(10, globals->prec_m);

	/* Reset stats */
	for ( i = 0; i < MAX_N_DIMS; i++ )
	{
		/* Reset bbox calculation */
		child_state.bbox_max[i] = INT64_MIN;
		child_state.bbox_min[i] = INT64_MAX;
		/* Reset acumulated delta values to get absolute values on next point */
		child_state.accum_rels[i] = 0;
	}

	/* RTTYPE/PRECISION BYTE */
	if ( abs(globals->prec_xy) > 7 )
		rterror("%s: X/Z precision cannot be greater than 7 or less than -7", __func__);
	
	/* Read the TWKB type number from the geometry */
	RTTYPE_PREC_SET_TYPE(type_prec, rtgeom_twkb_type(geom));
	/* Zig-zag the precision value before encoding it since it is a signed value */
	TYPE_PREC_SET_PREC(type_prec, zigzag8(globals->prec_xy));
	/* Write the type and precision byte */
	bytebuffer_append_byte(child_state.header_buf, type_prec);

	/* METADATA BYTE */
	/* Set first bit if we are going to store bboxes */
	FIRST_BYTE_SET_BBOXES(flag, (globals->variant & TWKB_BBOX) && ! is_empty);
	/* Set second bit if we are going to store resulting size */
	FIRST_BYTE_SET_SIZES(flag, globals->variant & TWKB_SIZE);
	/* There will be no ID-list (for now) */
	FIRST_BYTE_SET_IDLIST(flag, parent_state->idlist && ! is_empty);
	/* Are there higher dimensions */
	FIRST_BYTE_SET_EXTENDED(flag, optional_precision_byte);
	/* Empty? */
	FIRST_BYTE_SET_EMPTY(flag, is_empty);
	/* Write the header byte */
	bytebuffer_append_byte(child_state.header_buf, flag);

	/* EXTENDED PRECISION BYTE (OPTIONAL) */
	/* If needed, write the extended dim byte */
	if( optional_precision_byte )
	{
		uint8_t flag = 0;

		if ( has_z && ( globals->prec_z > 7 || globals->prec_z < 0 ) )
			rterror("%s: Z precision cannot be negative or greater than 7", __func__);

		if ( has_m && ( globals->prec_m > 7 || globals->prec_m < 0 ) )
			rterror("%s: M precision cannot be negative or greater than 7", __func__);

		HIGHER_DIM_SET_HASZ(flag, has_z);
		HIGHER_DIM_SET_HASM(flag, has_m);
		HIGHER_DIM_SET_PRECZ(flag, globals->prec_z);
		HIGHER_DIM_SET_PRECM(flag, globals->prec_m);
		bytebuffer_append_byte(child_state.header_buf, flag);
	}

	/* It the geometry is empty, we're almost done */
	if ( is_empty )
	{
		/* If this output is sized, write the size of */
		/* all following content, which is zero because */
		/* there is none */
		if ( globals->variant & TWKB_SIZE )
			bytebuffer_append_byte(child_state.header_buf, 0);

		bytebuffer_append_bytebuffer(parent_state->geom_buf, child_state.header_buf);
		bytebuffer_destroy(child_state.header_buf);
		bytebuffer_destroy(child_state.geom_buf);
		return 0;
	}

	/* Write the TWKB into the output buffer */
	rtgeom_to_twkb_buf(geom, globals, &child_state);

	/*If we have a header_buf, we know that this function is called inside a collection*/
	/*and then we have to merge the bboxes of the included geometries*/
	/*and put the result to the parent (the collection)*/
	if( (globals->variant & TWKB_BBOX) && parent_state->header_buf )
	{
		RTDEBUG(4,"Merge bboxes");
		for ( i = 0; i < ndims; i++ )
		{
			if(child_state.bbox_min[i]<parent_state->bbox_min[i])
				parent_state->bbox_min[i] = child_state.bbox_min[i];
			if(child_state.bbox_max[i]>parent_state->bbox_max[i])
				parent_state->bbox_max[i] = child_state.bbox_max[i];
		}
	}
	
	/* Did we have a box? If so, how big? */
	bbox_size = 0;
	if( globals->variant & TWKB_BBOX )
	{
		RTDEBUG(4,"We want boxes and will calculate required size");
		bbox_size = sizeof_bbox(&child_state, ndims);
	}

	/* Write the size if wanted */
	if( globals->variant & TWKB_SIZE )
	{
		/* Here we have to add what we know will be written to header */
		/* buffer after size value is written */
		size_t size_to_register = bytebuffer_getlength(child_state.geom_buf);
		size_to_register += bbox_size;
		bytebuffer_append_uvarint(child_state.header_buf, size_to_register);
	}

	if( globals->variant & TWKB_BBOX )
		write_bbox(&child_state, ndims);

	bytebuffer_append_bytebuffer(parent_state->geom_buf,child_state.header_buf);
	bytebuffer_append_bytebuffer(parent_state->geom_buf,child_state.geom_buf);

	bytebuffer_destroy(child_state.header_buf);
	bytebuffer_destroy(child_state.geom_buf);
	return 0;
}


/**
* Convert RTGEOM to a char* in TWKB format. Caller is responsible for freeing
* the returned array.
*/
uint8_t*
rtgeom_to_twkb_with_idlist(const RTGEOM *geom, int64_t *idlist, uint8_t variant,
               int8_t precision_xy, int8_t precision_z, int8_t precision_m,
               size_t *twkb_size)
{
	RTDEBUGF(2, "Entered %s", __func__);
	RTDEBUGF(2, "variant value %x", variant);

	TWKB_GLOBALS tg;
	TWKB_STATE ts;

	uint8_t *twkb;

	memset(&ts, 0, sizeof(TWKB_STATE));
	memset(&tg, 0, sizeof(TWKB_GLOBALS));
	
	tg.variant = variant;
	tg.prec_xy = precision_xy;
	tg.prec_z = precision_z;
	tg.prec_m = precision_m;

	if ( idlist && ! rtgeom_is_collection(geom) )
	{
		rterror("Only collections can support ID lists");
		return NULL;
	}

	if ( ! geom )
	{
		RTDEBUG(4,"Cannot convert NULL into TWKB.");
		rterror("Cannot convert NULL into TWKB");
		return NULL;
	}
	
	ts.idlist = idlist;
	ts.header_buf = NULL;
	ts.geom_buf = bytebuffer_create();
	rtgeom_write_to_buffer(geom, &tg, &ts);

	if ( twkb_size )
		*twkb_size = bytebuffer_getlength(ts.geom_buf);

	twkb = ts.geom_buf->buf_start;
	rtfree(ts.geom_buf);
	return twkb;
}


uint8_t*
rtgeom_to_twkb(const RTGEOM *geom, uint8_t variant,
               int8_t precision_xy, int8_t precision_z, int8_t precision_m,
               size_t *twkb_size)
{
	return rtgeom_to_twkb_with_idlist(geom, NULL, variant, precision_xy, precision_z, precision_m, twkb_size);
}

