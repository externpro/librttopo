
/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2011 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "measures3d.h"
#include "rtgeom_log.h"


static inline int
get_3dvector_from_points(POINT3DZ *p1,POINT3DZ *p2, VECTOR3D *v)
{
	v->x=p2->x-p1->x;
	v->y=p2->y-p1->y;
	v->z=p2->z-p1->z;
	
	return RT_TRUE;
}

static inline int
get_3dcross_product(VECTOR3D *v1,VECTOR3D *v2, VECTOR3D *v)
{
	v->x=(v1->y*v2->z)-(v1->z*v2->y);
	v->y=(v1->z*v2->x)-(v1->x*v2->z);
	v->z=(v1->x*v2->y)-(v1->y*v2->x);

	return RT_TRUE;
}


/**
This function is used to create a vertical line used for cases where one if the 
geometries lacks z-values. The vertical line crosses the 2d point that is closest 
and the z-range is from maxz to minz in the geoemtrie that has z values.
*/
static 
RTGEOM* create_v_line(const RTGEOM *rtgeom,double x, double y, int srid)
{
	
	RTPOINT *rtpoints[2];
	GBOX gbox;
	int rv = rtgeom_calculate_gbox(rtgeom, &gbox);
	
	if ( rv == RT_FAILURE )
		return NULL;
	
	rtpoints[0] = rtpoint_make3dz(srid, x, y, gbox.zmin);
	rtpoints[1] = rtpoint_make3dz(srid, x, y, gbox.zmax);
	
	 return (RTGEOM *)rtline_from_ptarray(srid, 2, rtpoints);		
}

RTGEOM * 
rtgeom_closest_line_3d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	return rt_dist3d_distanceline(rt1, rt2, rt1->srid, DIST_MIN);
}

RTGEOM * 
rtgeom_furthest_line_3d(RTGEOM *rt1, RTGEOM *rt2)
{
	return rt_dist3d_distanceline(rt1, rt2, rt1->srid, DIST_MAX);
}

RTGEOM * 
rtgeom_closest_point_3d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	return rt_dist3d_distancepoint(rt1, rt2, rt1->srid, DIST_MIN);
}


/**
Function initializing 3dshortestline and 3dlongestline calculations.
*/
RTGEOM *
rt_dist3d_distanceline(const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode)
{
	RTDEBUG(2, "rt_dist3d_distanceline is called");
	double x1,x2,y1,y2, z1, z2, x, y;
	double initdistance = ( mode == DIST_MIN ? FLT_MAX : -1.0);
	DISTPTS3D thedl;
	RTPOINT *rtpoints[2];
	RTGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0.0;

	/*Check if we really have 3D geoemtries*/
	/*If not, send it to 2D-calculations which will give the same result*/
	/*as an infinite z-value at one or two of the geometries*/
	if(!rtgeom_has_z(rt1) || !rtgeom_has_z(rt2))
	{
		
		rtnotice("One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");
		
		if(!rtgeom_has_z(rt1) && !rtgeom_has_z(rt2))
			return rt_dist2d_distanceline(rt1, rt2, srid, mode);	
		
		DISTPTS thedl2d;
		thedl2d.mode = mode;
		thedl2d.distance = initdistance;
		thedl2d.tolerance = 0.0;
		if (!rt_dist2d_comp( rt1,rt2,&thedl2d))
		{
			/*should never get here. all cases ought to be error handled earlier*/
			rterror("Some unspecified error.");
			result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
		}
		RTGEOM *vertical_line;
		if(!rtgeom_has_z(rt1))
		{
			x=thedl2d.p1.x;
			y=thedl2d.p1.y;

			vertical_line = create_v_line(rt2,x,y,srid);
			if (!rt_dist3d_recursive(vertical_line, rt2, &thedl))
			{
				/*should never get here. all cases ought to be error handled earlier*/
				rtfree(vertical_line);
				rterror("Some unspecified error.");
				result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
			}			
			rtfree(vertical_line);	
		}	
		if(!rtgeom_has_z(rt2))
		{
			x=thedl2d.p2.x;
			y=thedl2d.p2.y;			
			
			vertical_line = create_v_line(rt1,x,y,srid);
			if (!rt_dist3d_recursive(rt1, vertical_line, &thedl))
			{
				/*should never get here. all cases ought to be error handled earlier*/
				rtfree(vertical_line);
				rterror("Some unspecified error.");
				return (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
			}	
			rtfree(vertical_line);		
		}			
				
	}
	else
	{		
		if (!rt_dist3d_recursive(rt1, rt2, &thedl))
		{
			/*should never get here. all cases ought to be error handled earlier*/
			rterror("Some unspecified error.");
			result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
		}
	}
	/*if thedl.distance is unchanged there where only empty geometries input*/
	if (thedl.distance == initdistance)
	{
		RTDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x1=thedl.p1.x;
		y1=thedl.p1.y;
		z1=thedl.p1.z;
		x2=thedl.p2.x;
		y2=thedl.p2.y;
		z2=thedl.p2.z;

		rtpoints[0] = rtpoint_make3dz(srid, x1, y1, z1);
		rtpoints[1] = rtpoint_make3dz(srid, x2, y2, z2);

		result = (RTGEOM *)rtline_from_ptarray(srid, 2, rtpoints);
	}

	return result;
}

/**
Function initializing 3dclosestpoint calculations.
*/
RTGEOM *
rt_dist3d_distancepoint(const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode)
{
	
	double x,y,z;
	DISTPTS3D thedl;
	double initdistance = FLT_MAX;
	RTGEOM *result;

	thedl.mode = mode;
	thedl.distance= initdistance;
	thedl.tolerance = 0;

	RTDEBUG(2, "rt_dist3d_distancepoint is called");
	
	/*Check if we really have 3D geoemtries*/
	/*If not, send it to 2D-calculations which will give the same result*/
	/*as an infinite z-value at one or two of the geometries*/
	if(!rtgeom_has_z(rt1) || !rtgeom_has_z(rt2))
	{		
		rtnotice("One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");
				
		if(!rtgeom_has_z(rt1) && !rtgeom_has_z(rt2))
			return rt_dist2d_distancepoint(rt1, rt2, srid, mode);
			
		
		DISTPTS thedl2d;
		thedl2d.mode = mode;
		thedl2d.distance = initdistance;
		thedl2d.tolerance = 0.0;
		if (!rt_dist2d_comp( rt1,rt2,&thedl2d))
		{
			/*should never get here. all cases ought to be error handled earlier*/
			rterror("Some unspecified error.");
			return (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
		}
		
		RTGEOM *vertical_line;
		if(!rtgeom_has_z(rt1))
		{
			x=thedl2d.p1.x;
			y=thedl2d.p1.y;
			
			vertical_line = create_v_line(rt2,x,y,srid);	
			if (!rt_dist3d_recursive(vertical_line, rt2, &thedl))
			{
				/*should never get here. all cases ought to be error handled earlier*/
				rtfree(vertical_line);	
				rterror("Some unspecified error.");
				return (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
			}		
			rtfree(vertical_line);		
		}	
				
		if(!rtgeom_has_z(rt2))
		{
			x=thedl2d.p2.x;
			y=thedl2d.p2.y;

			vertical_line = create_v_line(rt1,x,y,srid);
			if (!rt_dist3d_recursive(rt1, vertical_line, &thedl))
			{
				/*should never get here. all cases ought to be error handled earlier*/
				rtfree(vertical_line);	
				rterror("Some unspecified error.");
				result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
			}	
			rtfree(vertical_line);			
		}	
		
	}
	else
	{
		if (!rt_dist3d_recursive(rt1, rt2, &thedl))
		{
			/*should never get here. all cases ought to be error handled earlier*/
			rterror("Some unspecified error.");
			result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
		}
	}
	if (thedl.distance == initdistance)
	{
		RTDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (RTGEOM *)rtcollection_construct_empty(RTCOLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x=thedl.p1.x;
		y=thedl.p1.y;
		z=thedl.p1.z;
		result = (RTGEOM *)rtpoint_make3dz(srid, x, y, z);
	}

	return result;
}


/**
Function initializing 3d max distance calculation
*/
double
rtgeom_maxdistance3d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	RTDEBUG(2, "rtgeom_maxdistance3d is called");

	return rtgeom_maxdistance3d_tolerance( rt1, rt2, 0.0 );
}

/**
Function handling 3d max distance calculations and dfullywithin calculations.
The difference is just the tolerance.
*/
double
rtgeom_maxdistance3d_tolerance(const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
	if(!rtgeom_has_z(rt1) || !rtgeom_has_z(rt2))
	{
		rtnotice("One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");
		return rtgeom_maxdistance2d_tolerance(rt1, rt2, tolerance);	
	}
	/*double thedist;*/
	DISTPTS3D thedl;
	RTDEBUG(2, "rtgeom_maxdistance3d_tolerance is called");
	thedl.mode = DIST_MAX;
	thedl.distance= -1;
	thedl.tolerance = tolerance;
	if (rt_dist3d_recursive(rt1, rt2, &thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	rterror("Some unspecified error.");
	return -1;
}

/**
	Function initializing 3d min distance calculation
*/
double
rtgeom_mindistance3d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	RTDEBUG(2, "rtgeom_mindistance3d is called");
	return rtgeom_mindistance3d_tolerance( rt1, rt2, 0.0 );
}

/**
	Function handling 3d min distance calculations and dwithin calculations.
	The difference is just the tolerance.
*/
double
rtgeom_mindistance3d_tolerance(const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
	if(!rtgeom_has_z(rt1) || !rtgeom_has_z(rt2))
	{
		rtnotice("One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");
		
		return rtgeom_mindistance2d_tolerance(rt1, rt2, tolerance);	
	}
	DISTPTS3D thedl;
	RTDEBUG(2, "rtgeom_mindistance3d_tolerance is called");
	thedl.mode = DIST_MIN;
	thedl.distance= FLT_MAX;
	thedl.tolerance = tolerance;
	if (rt_dist3d_recursive(rt1, rt2, &thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	rterror("Some unspecified error.");
	return FLT_MAX;
}


/*------------------------------------------------------------------------------------------------------------
End of Initializing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Preprocessing functions
Functions preparing geometries for distance-calculations
--------------------------------------------------------------------------------------------------------------*/


/**
This is a recursive function delivering every possible combination of subgeometries
*/
int rt_dist3d_recursive(const RTGEOM *rtg1,const RTGEOM *rtg2, DISTPTS3D *dl)
{
	int i, j;
	int n1=1;
	int n2=1;
	RTGEOM *g1 = NULL;
	RTGEOM *g2 = NULL;
	RTCOLLECTION *c1 = NULL;
	RTCOLLECTION *c2 = NULL;

	RTDEBUGF(2, "rt_dist3d_recursive is called with type1=%d, type2=%d", rtg1->type, rtg2->type);

	if (rtgeom_is_collection(rtg1))
	{
		RTDEBUG(3, "First geometry is collection");
		c1 = rtgeom_as_rtcollection(rtg1);
		n1 = c1->ngeoms;
	}
	if (rtgeom_is_collection(rtg2))
	{
		RTDEBUG(3, "Second geometry is collection");
		c2 = rtgeom_as_rtcollection(rtg2);
		n2 = c2->ngeoms;
	}

	for ( i = 0; i < n1; i++ )
	{

		if (rtgeom_is_collection(rtg1))
		{
			g1 = c1->geoms[i];
		}
		else
		{
			g1 = (RTGEOM*)rtg1;
		}

		if (rtgeom_is_empty(g1)) return RT_TRUE;

		if (rtgeom_is_collection(g1))
		{
			RTDEBUG(3, "Found collection inside first geometry collection, recursing");
			if (!rt_dist3d_recursive(g1, rtg2, dl)) return RT_FALSE;
			continue;
		}
		for ( j = 0; j < n2; j++ )
		{
			if (rtgeom_is_collection(rtg2))
			{
				g2 = c2->geoms[j];
			}
			else
			{
				g2 = (RTGEOM*)rtg2;
			}
			if (rtgeom_is_collection(g2))
			{
				RTDEBUG(3, "Found collection inside second geometry collection, recursing");
				if (!rt_dist3d_recursive(g1, g2, dl)) return RT_FALSE;
				continue;
			}


			/*If one of geometries is empty, return. True here only means continue searching. False would have stoped the process*/
			if (rtgeom_is_empty(g1)||rtgeom_is_empty(g2)) return RT_TRUE;


			if (!rt_dist3d_distribute_bruteforce(g1, g2, dl)) return RT_FALSE;
			if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
		}
	}
	return RT_TRUE;
}



/**

This function distributes the brute-force for 3D so far the only type, tasks depending on type
*/
int
rt_dist3d_distribute_bruteforce(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS3D *dl)
{

	int	t1 = rtg1->type;
	int	t2 = rtg2->type;

	RTDEBUGF(2, "rt_dist3d_distribute_bruteforce is called with typ1=%d, type2=%d", rtg1->type, rtg2->type);

	if  ( t1 == RTPOINTTYPE )
	{
		if  ( t2 == RTPOINTTYPE )
		{
			dl->twisted=1;
			return rt_dist3d_point_point((RTPOINT *)rtg1, (RTPOINT *)rtg2, dl);
		}
		else if  ( t2 == RTLINETYPE )
		{
			dl->twisted=1;
			return rt_dist3d_point_line((RTPOINT *)rtg1, (RTLINE *)rtg2, dl);
		}
		else if  ( t2 == RTPOLYGONTYPE )
		{
			dl->twisted=1;
			return rt_dist3d_point_poly((RTPOINT *)rtg1, (RTPOLY *)rtg2,dl);
		}
		else
		{
			rterror("Unsupported geometry type: %s", rttype_name(t2));
			return RT_FALSE;
		}
	}
	else if ( t1 == RTLINETYPE )
	{
		if ( t2 == RTPOINTTYPE )
		{
			dl->twisted=(-1);
			return rt_dist3d_point_line((RTPOINT *)rtg2,(RTLINE *)rtg1,dl);
		}
		else if ( t2 == RTLINETYPE )
		{
			dl->twisted=1;
			return rt_dist3d_line_line((RTLINE *)rtg1,(RTLINE *)rtg2,dl);
		}
		else if ( t2 == RTPOLYGONTYPE )
		{
			dl->twisted=1;
			return rt_dist3d_line_poly((RTLINE *)rtg1,(RTPOLY *)rtg2,dl);
		}
		else
		{
			rterror("Unsupported geometry type: %s", rttype_name(t2));
			return RT_FALSE;
		}
	}
	else if ( t1 == RTPOLYGONTYPE )
	{
		if ( t2 == RTPOLYGONTYPE )
		{
			dl->twisted=1;
			return rt_dist3d_poly_poly((RTPOLY *)rtg1, (RTPOLY *)rtg2,dl);
		}
		else if ( t2 == RTPOINTTYPE )
		{
			dl->twisted=-1;
			return rt_dist3d_point_poly((RTPOINT *)rtg2, (RTPOLY *)rtg1,dl);
		}
		else if ( t2 == RTLINETYPE )
		{
			dl->twisted=-1;
			return rt_dist3d_line_poly((RTLINE *)rtg2,(RTPOLY *)rtg1,dl);
		}
		else
		{
			rterror("Unsupported geometry type: %s", rttype_name(t2));
			return RT_FALSE;
		}
	}
	else
	{
		rterror("Unsupported geometry type: %s", rttype_name(t1));
		return RT_FALSE;
	}
	/*You shouldn't being able to get here*/
	rterror("unspecified error in function rt_dist3d_distribute_bruteforce");
	return RT_FALSE;
}



/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Brute force functions
So far the only way to do 3D-calculations
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
rt_dist3d_point_point(RTPOINT *point1, RTPOINT *point2, DISTPTS3D *dl)
{
	POINT3DZ p1;
	POINT3DZ p2;
	RTDEBUG(2, "rt_dist3d_point_point is called");

	getPoint3dz_p(point1->point, 0, &p1);
	getPoint3dz_p(point2->point, 0, &p2);

	return rt_dist3d_pt_pt(&p1, &p2,dl);
}
/**

point to line calculation
*/
int
rt_dist3d_point_line(RTPOINT *point, RTLINE *line, DISTPTS3D *dl)
{
	POINT3DZ p;
	POINTARRAY *pa = line->points;
	RTDEBUG(2, "rt_dist3d_point_line is called");

	getPoint3dz_p(point->point, 0, &p);
	return rt_dist3d_pt_ptarray(&p, pa, dl);
}

/**

Computes point to polygon distance
For mindistance that means:
1)find the plane of the polygon 
2)projecting the point to the plane of the polygon 
3)finding if that projected point is inside the polygon, if so the distance is measured to that projected point
4) if not in polygon above, check the distance against the boundary of the polygon
for max distance it is artays point against boundary

*/
int
rt_dist3d_point_poly(RTPOINT *point, RTPOLY *poly, DISTPTS3D *dl)
{
	POINT3DZ p, projp;/*projp is "point projected on plane"*/
	PLANE3D plane;
	RTDEBUG(2, "rt_dist3d_point_poly is called");
	getPoint3dz_p(point->point, 0, &p);
	
	/*If we are lookig for max distance, longestline or dfullywithin*/
	if (dl->mode == DIST_MAX)
	{
		RTDEBUG(3, "looking for maxdistance");
		return rt_dist3d_pt_ptarray(&p, poly->rings[0], dl);
	}
	
	/*Find the plane of the polygon, the "holes" have to be on the same plane. so we only care about the boudary*/
	if(!define_plane(poly->rings[0], &plane))
		return RT_FALSE;
	
	/*get our point projected on the plane of the polygon*/
	project_point_on_plane(&p, &plane, &projp);
	
	return rt_dist3d_pt_poly(&p, poly,&plane, &projp, dl);
}


/**

line to line calculation
*/
int
rt_dist3d_line_line(RTLINE *line1, RTLINE *line2, DISTPTS3D *dl)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	RTDEBUG(2, "rt_dist3d_line_line is called");

	return rt_dist3d_ptarray_ptarray(pa1, pa2, dl);
}

/**

line to polygon calculation
*/
int rt_dist3d_line_poly(RTLINE *line, RTPOLY *poly, DISTPTS3D *dl)
{
	PLANE3D plane;	
	RTDEBUG(2, "rt_dist3d_line_poly is called");	
		
	if (dl->mode == DIST_MAX)
	{
		return rt_dist3d_ptarray_ptarray(line->points, poly->rings[0], dl);
	}
	
	if(!define_plane(poly->rings[0], &plane))
		return RT_FALSE;
	
	return rt_dist3d_ptarray_poly(line->points, poly,&plane, dl);
}

/**

polygon to polygon calculation
*/
int rt_dist3d_poly_poly(RTPOLY *poly1, RTPOLY *poly2, DISTPTS3D *dl)
{		
	PLANE3D plane;		
	RTDEBUG(2, "rt_dist3d_poly_poly is called");
	if (dl->mode == DIST_MAX)
	{
		return rt_dist3d_ptarray_ptarray(poly1->rings[0], poly2->rings[0], dl);
	}
	
	if(!define_plane(poly2->rings[0], &plane))
		return RT_FALSE;
	
	/*What we do here is to compare the bondary of one polygon with the other polygon 
	and then take the second boudary comparing with the first polygon*/
	dl->twisted=1;
	if(!rt_dist3d_ptarray_poly(poly1->rings[0], poly2,&plane, dl))
		return RT_FALSE;
	if(dl->distance==0.0) /*Just check if the answer already is given*/
		return RT_TRUE;
	
	if(!define_plane(poly1->rings[0], &plane))
		return RT_FALSE;
	dl->twisted=-1; /*because we swithc the order of geometries we swithch "twisted" to -1 which will give the right order of points in shortest line.*/
	return rt_dist3d_ptarray_poly(poly2->rings[0], poly1,&plane, dl);
}

/**

 * search all the segments of pointarray to see which one is closest to p
 * Returns distance between point and pointarray
 */
int
rt_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa,DISTPTS3D *dl)
{
	int t;
	POINT3DZ	start, end;
	int twist = dl->twisted;

	RTDEBUG(2, "rt_dist3d_pt_ptarray is called");

	getPoint3dz_p(pa, 0, &start);

	for (t=1; t<pa->npoints; t++)
	{
		dl->twisted=twist;
		getPoint3dz_p(pa, t, &end);
		if (!rt_dist3d_pt_seg(p, &start, &end,dl)) return RT_FALSE;

		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
		start = end;
	}

	return RT_TRUE;
}


/**

If searching for min distance, this one finds the closest point on segment A-B from p.
if searching for max distance it just sends p-A and p-B to pt-pt calculation
*/
int
rt_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl)
{
	POINT3DZ c;
	double	r;
	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) && (A->z == B->z)  )
	{
		return rt_dist3d_pt_pt(p,A,dl);
	}


	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y)  + ( p->z-A->z) * (B->z-A->z) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y)+(B->z-A->z)*(B->z-A->z) );

	/*This is for finding the 3Dmaxdistance.
	the maxdistance have to be between two vertexes,
	compared to mindistance which can be between
	tvo vertexes vertex.*/
	if (dl->mode == DIST_MAX)
	{
		if (r>=0.5)
		{
			return rt_dist3d_pt_pt(p,A,dl);
		}
		if (r<0.5)
		{
			return rt_dist3d_pt_pt(p,B,dl);
		}
	}

	if (r<0)	/*If the first vertex A is closest to the point p*/
	{
		return rt_dist3d_pt_pt(p,A,dl);
	}
	if (r>1)	/*If the second vertex B is closest to the point p*/
	{
		return rt_dist3d_pt_pt(p,B,dl);
	}

	/*else if the point p is closer to some point between a and b
	then we find that point and send it to rt_dist3d_pt_pt*/
	c.x=A->x + r * (B->x-A->x);
	c.y=A->y + r * (B->y-A->y);
	c.z=A->z + r * (B->z-A->z);

	return rt_dist3d_pt_pt(p,&c,dl);
}

double
distance3d_pt_pt(const POINT3D *p1, const POINT3D *p2)
{
  double dx = p2->x - p1->x;
  double dy = p2->y - p1->y;
  double dz = p2->z - p1->z;
  return sqrt ( dx*dx + dy*dy + dz*dz);
}


/**

Compares incomming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
rt_dist3d_pt_pt(POINT3DZ *thep1, POINT3DZ *thep2,DISTPTS3D *dl)
{
	double dx = thep2->x - thep1->x;
	double dy = thep2->y - thep1->y;
	double dz = thep2->z - thep1->z;
	double dist = sqrt ( dx*dx + dy*dy + dz*dz);
	RTDEBUGF(2, "rt_dist3d_pt_pt called (with points: p1.x=%f, p1.y=%f,p1.z=%f,p2.x=%f, p2.y=%f,p2.z=%f)",thep1->x,thep1->y,thep1->z,thep2->x,thep2->y,thep2->z );

	if (((dl->distance - dist)*(dl->mode))>0) /*multiplication with mode to handle mindistance (mode=1)  and maxdistance (mode = (-1)*/
	{
		dl->distance = dist;

		if (dl->twisted>0)	/*To get the points in right order. twisted is updated between 1 and (-1) every time the order is changed earlier in the chain*/
		{
			dl->p1 = *thep1;
			dl->p2 = *thep2;
		}
		else
		{
			dl->p1 = *thep2;
			dl->p2 = *thep1;
		}
	}
	return RT_TRUE;
}


/**

Finds all combinationes of segments between two pointarrays
*/
int
rt_dist3d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS3D *dl)
{
	int t,u;
	POINT3DZ	start, end;
	POINT3DZ	start2, end2;
	int twist = dl->twisted;
	RTDEBUGF(2, "rt_dist3d_ptarray_ptarray called (points: %d-%d)",l1->npoints, l2->npoints);



	if (dl->mode == DIST_MAX)/*If we are searching for maxdistance we go straight to point-point calculation since the maxdistance have to be between two vertexes*/
	{
		for (t=0; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &start);
			for (u=0; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &start2);
				rt_dist3d_pt_pt(&start,&start2,dl);
				RTDEBUGF(4, "maxdist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				RTDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
			}
		}
	}
	else
	{
		getPoint3dz_p(l1, 0, &start);
		for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &end);
			getPoint3dz_p(l2, 0, &start2);
			for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &end2);
				dl->twisted=twist;
				rt_dist3d_seg_seg(&start, &end, &start2, &end2,dl);
				RTDEBUGF(4, "mindist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				RTDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
				if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
				start2 = end2;
			}
			start = end;
		}
	}
	return RT_TRUE;
}

/**

Finds the two closest points on two linesegments
*/
int 
rt_dist3d_seg_seg(POINT3DZ *s1p1, POINT3DZ *s1p2, POINT3DZ *s2p1, POINT3DZ *s2p2, DISTPTS3D *dl)
{
	VECTOR3D v1, v2, vl;
	double s1k, s2k; /*two variables representing where on Line 1 (s1k) and where on Line 2 (s2k) a connecting line between the two lines is perpendicular to both lines*/
	POINT3DZ p1, p2;
	double a, b, c, d, e, D;
			
	/*s1p1 and s1p2 are the same point */
	if (  ( s1p1->x == s1p2->x) && (s1p1->y == s1p2->y) && (s1p1->z == s1p2->z) )
	{
		return rt_dist3d_pt_seg(s1p1,s2p1,s2p2,dl);
	}
	/*s2p1 and s2p2 are the same point */
	if (  ( s2p1->x == s2p2->x) && (s2p1->y == s2p2->y) && (s2p1->z == s2p2->z) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return rt_dist3d_pt_seg(s2p1,s1p1,s1p2,dl);
	}
		
/*
	Here we use algorithm from softsurfer.com
	that can be found here
	http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
*/
	
	if (!get_3dvector_from_points(s1p1, s1p2, &v1))
		return RT_FALSE;	

	if (!get_3dvector_from_points(s2p1, s2p2, &v2))
		return RT_FALSE;	

	if (!get_3dvector_from_points(s2p1, s1p1, &vl))
		return RT_FALSE;	

	a = DOT(v1,v1);
	b = DOT(v1,v2);
	c = DOT(v2,v2);
	d = DOT(v1,vl);
	e = DOT(v2,vl);
	D = a*c - b*b; 


	if (D <0.000000001) 
	{        /* the lines are almost parallel*/
		s1k = 0.0; /*If the lines are paralell we try by using the startpoint of first segment. If that gives a projected point on the second line outside segment 2 it wil be found that s2k is >1 or <0.*/
		if(b>c)   /* use the largest denominator*/
		{
			s2k=d/b;
		}
		else
		{
			s2k =e/c;
		}
	}
	else 
	{
		s1k = (b*e - c*d) / D;
		s2k = (a*e - b*d) / D;
	}

	/* Now we check if the projected closest point on the infinite lines is outside our segments. If so the combinations with start and end points will be tested*/
	if(s1k<0.0||s1k>1.0||s2k<0.0||s2k>1.0)
	{
		if(s1k<0.0) 
		{

			if (!rt_dist3d_pt_seg(s1p1, s2p1, s2p2, dl))
			{
				return RT_FALSE;
			}
		}
		if(s1k>1.0)
		{

			if (!rt_dist3d_pt_seg(s1p2, s2p1, s2p2, dl))
			{
				return RT_FALSE;
			}
		}
		if(s2k<0.0)
		{
			dl->twisted= ((dl->twisted) * (-1));
			if (!rt_dist3d_pt_seg(s2p1, s1p1, s1p2, dl))
			{
				return RT_FALSE;
			}
		}
		if(s2k>1.0)
		{
			dl->twisted= ((dl->twisted) * (-1));
			if (!rt_dist3d_pt_seg(s2p2, s1p1, s1p2, dl))
			{
				return RT_FALSE;
			}
		}
	}
	else
	{/*Find the closest point on the edges of both segments*/
		p1.x=s1p1->x+s1k*(s1p2->x-s1p1->x);
		p1.y=s1p1->y+s1k*(s1p2->y-s1p1->y);
		p1.z=s1p1->z+s1k*(s1p2->z-s1p1->z);

		p2.x=s2p1->x+s2k*(s2p2->x-s2p1->x);
		p2.y=s2p1->y+s2k*(s2p2->y-s2p1->y);
		p2.z=s2p1->z+s2k*(s2p2->z-s2p1->z);

		if (!rt_dist3d_pt_pt(&p1,&p2,dl))/* Send the closest points to point-point calculation*/
		{
			return RT_FALSE;
		}
	}
	return RT_TRUE;
}

/**

Checking if the point projected on the plane of the polygon actually is inside that polygon. 
If so the mindistance is between that projected point and our original point.
If not we check from original point to the bounadary.
If the projected point is inside a hole of the polygon we check the distance to the boudary of that hole.
*/
int
rt_dist3d_pt_poly(POINT3DZ *p, RTPOLY *poly, PLANE3D *plane,POINT3DZ *projp, DISTPTS3D *dl)
{	
	int i;
	
	RTDEBUG(2, "rt_dist3d_point_poly called");

	
	if(pt_in_ring_3d(projp, poly->rings[0], plane))
	{
		for (i=1; i<poly->nrings; i++)
		{
			/* Inside a hole. Distance = pt -> ring */
			if ( pt_in_ring_3d(projp, poly->rings[i], plane ))
			{
				RTDEBUG(3, " inside an hole");
				return rt_dist3d_pt_ptarray(p, poly->rings[i], dl);
			}
		}		
		
		return rt_dist3d_pt_pt(p,projp,dl);/* If the projected point is inside the polygon the shortest distance is between that point and the inputed point*/
	}
	else
	{
		return rt_dist3d_pt_ptarray(p, poly->rings[0], dl); /*If the projected point is outside the polygon we search for the closest distance against the boundarry instead*/
	}	
	
	return RT_TRUE;
	
}

/**

Computes pointarray to polygon distance
*/
int rt_dist3d_ptarray_poly(POINTARRAY *pa, RTPOLY *poly,PLANE3D *plane, DISTPTS3D *dl)
{
	

	int i,j,k;
	double f, s1, s2;
	VECTOR3D projp1_projp2;
	POINT3DZ p1, p2,projp1, projp2, intersectionp;
	
	getPoint3dz_p(pa, 0, &p1);
	
	s1=project_point_on_plane(&p1, plane, &projp1); /*the sign of s1 tells us on which side of the plane the point is. */
	rt_dist3d_pt_poly(&p1, poly, plane,&projp1, dl);	
	
	for (i=1;i<pa->npoints;i++)
	{		
		int intersects;
		getPoint3dz_p(pa, i, &p2);
		s2=project_point_on_plane(&p2, plane, &projp2);	
		rt_dist3d_pt_poly(&p2, poly, plane,&projp2, dl);
		
		/*If s1and s2 has different signs that means they are on different sides of the plane of the polygon.
		That means that the edge between the points crosses the plane and might intersect with the polygon*/
		if((s1*s2)<=0) 
		{
			f=fabs(s1)/(fabs(s1)+fabs(s2)); /*The size of s1 and s2 is the distance from the point to the plane.*/
			get_3dvector_from_points(&projp1, &projp2,&projp1_projp2);
			
			/*get the point where the line segment crosses the plane*/
			intersectionp.x=projp1.x+f*projp1_projp2.x;
			intersectionp.y=projp1.y+f*projp1_projp2.y;
			intersectionp.z=projp1.z+f*projp1_projp2.z;
			
			intersects = RT_TRUE; /*We set intersects to true until the opposite is proved*/
			
			if(pt_in_ring_3d(&intersectionp, poly->rings[0], plane)) /*Inside outer ring*/
			{
				for (k=1;k<poly->nrings; k++)
				{
					/* Inside a hole, so no intersection with the polygon*/
					if ( pt_in_ring_3d(&intersectionp, poly->rings[k], plane ))
					{
						intersects=RT_FALSE;
						break;
					}
				}		
				if(intersects) 
				{
					dl->distance=0.0;
					dl->p1.x=intersectionp.x;
					dl->p1.y=intersectionp.y;
					dl->p1.z=intersectionp.z;
					
					dl->p2.x=intersectionp.x;
					dl->p2.y=intersectionp.y;
					dl->p2.z=intersectionp.z;
					return RT_TRUE;
					
				}					
			}			
		}
		
		projp1=projp2;
		s1=s2;
		p1=p2;
	}	
	
	/*check or pointarray against boundary and inner boundaries of the polygon*/
	for (j=0;j<poly->nrings;j++)
	{
		rt_dist3d_ptarray_ptarray(pa, poly->rings[j], dl);
	}
	
return RT_TRUE;
}	


/**

Here we define the plane of a polygon (boundary pointarray of a polygon)
the plane is stored as a pont in plane (plane.pop) and a normal vector (plane.pv)
*/
int
define_plane(POINTARRAY *pa, PLANE3D *pl)
{
	int i,j, numberofvectors, pointsinslice;
	POINT3DZ p, p1, p2;

	double sumx=0;
	double sumy=0;
	double sumz=0;
	double vl; /*vector length*/

	VECTOR3D v1, v2, v;
	
	if((pa->npoints-1)==3) /*Triangle is special case*/
	{
		pointsinslice=1;		
	}
	else
	{
		pointsinslice=(int) floor((pa->npoints-1)/4); /*divide the pointarray into 4 slices*/
	}
	
	/*find the avg point*/
	for (i=0;i<(pa->npoints-1);i++)
	{
		getPoint3dz_p(pa, i, &p);
		sumx+=p.x;
		sumy+=p.y;
		sumz+=p.z;		
	}	
	pl->pop.x=(sumx/(pa->npoints-1));
	pl->pop.y=(sumy/(pa->npoints-1));
	pl->pop.z=(sumz/(pa->npoints-1));
	
	sumx=0;
	sumy=0;
	sumz=0;
	numberofvectors= floor((pa->npoints-1)/pointsinslice); /*the number of vectors we try can be 3, 4 or 5*/
	
	getPoint3dz_p(pa, 0, &p1);
	for (j=pointsinslice;j<pa->npoints;j+=pointsinslice)
	{
		getPoint3dz_p(pa, j, &p2);	
		
		if (!get_3dvector_from_points(&(pl->pop), &p1, &v1) || !get_3dvector_from_points(&(pl->pop), &p2, &v2))
			return RT_FALSE;	
		/*perpendicular vector is cross product of v1 and v2*/
		if (!get_3dcross_product(&v1,&v2, &v))
			return RT_FALSE;		
		vl=VECTORLENGTH(v);
		sumx+=(v.x/vl);
		sumy+=(v.y/vl);
		sumz+=(v.z/vl);	
		p1=p2;
	}
	pl->pv.x=(sumx/numberofvectors);
	pl->pv.y=(sumy/numberofvectors);
	pl->pv.z=(sumz/numberofvectors);
	
	return 1;
}

/**

Finds a point on a plane from where the original point is perpendicular to the plane
*/
double 
project_point_on_plane(POINT3DZ *p,  PLANE3D *pl, POINT3DZ *p0)
{
/*In our plane definition we have a point on the plane and a normal vektor (pl.pv), perpendicular to the plane
this vector will be paralell to the line between our inputted point above the plane and the point we are searching for on the plane.
So, we already have a direction from p to find p0, but we don't know the distance.
*/

	VECTOR3D v1;
	double f;
	
	if (!get_3dvector_from_points(&(pl->pop), p, &v1))
	return RT_FALSE;	
	
	f=-(DOT(pl->pv,v1)/DOT(pl->pv,pl->pv));
	
	p0->x=p->x+pl->pv.x*f;
	p0->y=p->y+pl->pv.y*f;
	p0->z=p->z+pl->pv.z*f;      
	
	return f;		
}




/**
 * pt_in_ring_3d(): crossing number test for a point in a polygon
 *      input:   p = a point,
 *               pa = vertex points of a ring V[n+1] with V[n]=V[0]
*		plane=the plane that the vertex points are lying on
 *      returns: 0 = outside, 1 = inside
 *
 *	Our polygons have first and last point the same,
 *
*	The difference in 3D variant is that we exclude the dimension that faces the plane least.
*	That is the dimension with the highest number in pv
 */
int
pt_in_ring_3d(const POINT3DZ *p, const POINTARRAY *ring,PLANE3D *plane)
{
	
	int cn = 0;    /* the crossing number counter */
	int i;
	POINT3DZ v1, v2;

	POINT3DZ	first, last;

	getPoint3dz_p(ring, 0, &first);
	getPoint3dz_p(ring, ring->npoints-1, &last);
	if ( memcmp(&first, &last, sizeof(POINT3DZ)) )
	{
		rterror("pt_in_ring_3d: V[n] != V[0] (%g %g %g!= %g %g %g)",
		        first.x, first.y, first.z, last.x, last.y, last.z);
		return RT_FALSE;
	}

	RTDEBUGF(2, "pt_in_ring_3d called with point: %g %g %g", p->x, p->y, p->z);
	/* printPA(ring); */

	/* loop through all edges of the polygon */
	getPoint3dz_p(ring, 0, &v1);
	
	
	if(fabs(plane->pv.z)>=fabs(plane->pv.x)&&fabs(plane->pv.z)>=fabs(plane->pv.y))	/*If the z vector of the normal vector to the plane is larger than x and y vector we project the ring to the xy-plane*/
	{
		for (i=0; i<ring->npoints-1; i++)
		{
			double vt;
			getPoint3dz_p(ring, i+1, &v2);

			/* edge from vertex i to vertex i+1 */
			if
			(
			    /* an upward crossing */
			    ((v1.y <= p->y) && (v2.y > p->y))
			    /* a downward crossing */
			    || ((v1.y > p->y) && (v2.y <= p->y))
			)
			{

				vt = (double)(p->y - v1.y) / (v2.y - v1.y);

				/* P.x <intersect */
				if (p->x < v1.x + vt * (v2.x - v1.x))
				{
					/* a valid crossing of y=p.y right of p.x */
					++cn;
				}
			}
			v1 = v2;
		}
	}
	else if(fabs(plane->pv.y)>=fabs(plane->pv.x)&&fabs(plane->pv.y)>=fabs(plane->pv.z))	/*If the y vector of the normal vector to the plane is larger than x and z vector we project the ring to the xz-plane*/
	{
		for (i=0; i<ring->npoints-1; i++)
			{
				double vt;
				getPoint3dz_p(ring, i+1, &v2);

				/* edge from vertex i to vertex i+1 */
				if
				(
				    /* an upward crossing */
				    ((v1.z <= p->z) && (v2.z > p->z))
				    /* a downward crossing */
				    || ((v1.z > p->z) && (v2.z <= p->z))
				)
				{

					vt = (double)(p->z - v1.z) / (v2.z - v1.z);

					/* P.x <intersect */
					if (p->x < v1.x + vt * (v2.x - v1.x))
					{
						/* a valid crossing of y=p.y right of p.x */
						++cn;
					}
				}
				v1 = v2;
			}
	}
	else	/*Hopefully we only have the cases where x part of the normal vector is largest left*/
	{
		for (i=0; i<ring->npoints-1; i++)
			{
				double vt;
				getPoint3dz_p(ring, i+1, &v2);

				/* edge from vertex i to vertex i+1 */
				if
				(
				    /* an upward crossing */
				    ((v1.z <= p->z) && (v2.z > p->z))
				    /* a downward crossing */
				    || ((v1.z > p->z) && (v2.z <= p->z))
				)
				{

					vt = (double)(p->z - v1.z) / (v2.z - v1.z);

					/* P.x <intersect */
					if (p->y < v1.y + vt * (v2.y - v1.y))
					{
						/* a valid crossing of y=p.y right of p.x */
						++cn;
					}
				}
				v1 = v2;
			}
	}
	RTDEBUGF(3, "pt_in_ring_3d returning %d", cn&1);

	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}



/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/


