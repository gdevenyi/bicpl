/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#include  <internal_volume_io.h>
#include  <geom.h>
#include  <numerical.h>

#define  MAX_POINTS    30
#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/libraries/bicpl/Geometry/ray_intersect.c,v 1.19 1996-08-06 19:05:21 david Exp $";
#endif


#define  TOLERANCE  1.0e-2

private  BOOLEAN  point_within_triangle_2d(
    Point   *pt,
    Point   points[] );
private  BOOLEAN  point_within_polygon_2d(
    Point   *pt,
    int     n_points,
    Point   points[],
    Vector  *polygon_normal );

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_polygon_points
@INPUT      : ray_origin
              ray_direction
              n_points
              points
@OUTPUT     : dist
@RETURNS    : TRUE if ray intersects
@DESCRIPTION: Tests if the ray intersects the polygon, passing back the
              distance.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN   intersect_ray_polygon_points(
    Point            *ray_origin,
    Vector           *ray_direction,
    int              n_points,
    Point            points[],
    Real             *dist )
{
    BOOLEAN  intersects;
    Vector   normal;
    Real     n_dot_d, n_dot_o, t, plane_const, nx, ny, nz;
    Real     cx, cy, cz;
    Point    pt;
    int      p;

    intersects = FALSE;

    find_polygon_normal_no_normalize( n_points, points, &nx, &ny, &nz );

    n_dot_d = nx * (Real) Vector_x(*ray_direction) +
              ny * (Real) Vector_y(*ray_direction) +
              nz * (Real) Vector_z(*ray_direction);

    if( n_dot_d != 0.0 )
    {
        cx = 0.0;
        cy = 0.0;
        cz = 0.0;

        for_less( p, 0, n_points )
        {
            cx += (Real) Point_x(points[p]);
            cy += (Real) Point_y(points[p]);
            cz += (Real) Point_z(points[p]);
        }

        plane_const = (nx * cx + ny * cy + nz * cz) / (Real) n_points;

        n_dot_o = nx * (Real) Point_x(*ray_origin) +
                  ny * (Real) Point_y(*ray_origin) +
                  nz * (Real) Point_z(*ray_origin);

        t = (plane_const - n_dot_o) / n_dot_d;

        if( t >= 0.0 )
        {
            GET_POINT_ON_RAY( pt, *ray_origin, *ray_direction, t );

            fill_Vector( normal, nx, ny, nz );

            if( point_within_polygon( &pt, n_points, points, &normal ) )
            {
                *dist = t;
                intersects = TRUE;
            }
        }
    }

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_polygon
@INPUT      : ray_origin
              ray_direction
              polygons
              poly_index
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if a ray intersects a given poly in the polygons.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN   intersect_ray_polygon(
    Point            *ray_origin,
    Vector           *ray_direction,
    Real             *dist,
    polygons_struct  *polygons,
    int              poly_index )
{
    BOOLEAN  intersects;
    Point    points[MAX_POINTS];
    int      ind, p, start_index, end_index, size;

    intersects = FALSE;

    if( polygons->visibilities == (Smallest_int *) 0 ||
        polygons->visibilities[poly_index] )
    {
        start_index = START_INDEX( polygons->end_indices, poly_index );
        end_index = polygons->end_indices[poly_index];

        size = end_index - start_index;

        if( size > MAX_POINTS )
        {
            print_error( "Warning: awfully big polygon, size = %d\n", size );
            size = MAX_POINTS;
            end_index = start_index + size - 1;
        }

        for_less( p, start_index, end_index )
        {
            ind = polygons->indices[p];
            points[p-start_index] = polygons->points[ind];
        }

        intersects = intersect_ray_polygon_points( ray_origin, ray_direction,
                                                   size, points, dist );
    }

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : point_within_polygon
@INPUT      : pt
              n_points
              points
              polygon_normal
@OUTPUT     : 
@RETURNS    : TRUE if inside
@DESCRIPTION: Tests if a point is within a 3d polygon.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  point_within_polygon(
    Point   *pt,
    int     n_points,
    Point   points[],
    Vector  *polygon_normal )
{
    BOOLEAN  intersects;

    if( n_points == 3 )
        intersects = point_within_triangle_2d( pt, points );
    else
        intersects = point_within_polygon_2d( pt, n_points, points,
                                              polygon_normal );

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : point_within_triangle_2d
@INPUT      : pt
              points
@OUTPUT     : 
@RETURNS    : TRUE if inside
@DESCRIPTION: Tests if a point is within a 3D triangle.  The point should
              be on or close to on the plane of the triangle.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN  point_within_triangle_2d(
    Point   *pt,
    Point   points[] )
{
    BOOLEAN  inside;
    int      i;
    Vector   edges[3];
    Vector   normal, offset, edge_normal;

    SUB_VECTORS( edges[0], points[1], points[0] );
    SUB_VECTORS( edges[1], points[2], points[1] );
    SUB_VECTORS( edges[2], points[0], points[2] );
    CROSS_VECTORS( normal, edges[2], edges[0] );
    NORMALIZE_VECTOR( normal, normal );

    inside = TRUE;

    for_less( i, 0, 3 )
    {
        SUB_POINTS( offset, *pt, points[i] );
        CROSS_VECTORS( edge_normal, edges[i], normal );
        NORMALIZE_VECTOR( edge_normal, edge_normal );
        if( DOT_VECTORS( offset, edge_normal ) > TOLERANCE )
        {
            inside = FALSE;
            break;
        }
    }

    return( inside );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : point_within_polygon_2d
@INPUT      : pt
              n_points
              points
              polygon_normal
@OUTPUT     : 
@RETURNS    : TRUE if inside
@DESCRIPTION: Tests if the point is within the 3D polygon, by projecting
              the problem to 2 dimensions.  The polygon may be convex or not.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN  point_within_polygon_2d(
    Point   *pt,
    int     n_points,
    Point   points[],
    Vector  *polygon_normal )
{
    BOOLEAN  intersects, cross;
    Real     x, y, x1, y1, x2, y2, x_inter, dx, dy, tx, ty, len, t;
    Real     nx, ny, nz, max_val;
    int      i1, i2;
    int      i;

    nx = FABS( (Real) Vector_x(*polygon_normal) );
    ny = FABS( (Real) Vector_y(*polygon_normal) );
    nz = FABS( (Real) Vector_z(*polygon_normal) );

    max_val = MAX3( nx, ny, nz );

    if( nx == max_val )
    {
        i1 = Y;
        i2 = Z;
    }
    else if( ny == max_val )
    {
        i1 = Z;
        i2 = X;
    }
    else
    {
        i1 = X;
        i2 = Y;
    }

    x = (Real) Point_coord( *pt, i1 );
    y = (Real) Point_coord( *pt, i2 );

    cross = FALSE;

    x2 = (Real) Point_coord(points[n_points-1],i1);
    y2 = (Real) Point_coord(points[n_points-1],i2);

    for_less( i, 0, n_points )
    {
        x1 = x2;
        y1 = y2;

        x2 = (Real) Point_coord(points[i],i1);
        y2 = (Real) Point_coord(points[i],i2);

        if( !( (y1 > y && y2 > y ) ||
               (y1 < y && y2 < y ) ||
               (x1 > x && x2 > x )) )
        {
            dy = y2 - y1;
            if( dy != 0.0 )
            {
                if( y1 == y )
                {
                    if( y2 > y1 && x1 <= x )
                    {
                        cross = !cross;
                    }
                }
                else if( y2 == y )
                {
                    if( y1 > y2 && x2 <= x )
                    {
                        cross = !cross;
                    }
                }
                else if( x1 <= x && x2 <= x )
                {
                    cross = !cross;
                }
                else
                {
                    x_inter = x1 + (y - y1) / dy * (x2 - x1);

                    if( x_inter < x )
                    {
                        cross = !cross;
                    }
                }
            }
        }
    }

    intersects = cross;

    if( !intersects )
    {
        x2 = (Real) Point_coord(points[n_points-1],i1);
        y2 = (Real) Point_coord(points[n_points-1],i2);

        for_less( i, 0, n_points )
        {
            x1 = x2;
            y1 = y2;

            x2 = (Real) Point_coord(points[i],i1);
            y2 = (Real) Point_coord(points[i],i2);

            if( x1 - TOLERANCE <= x && x <= x1 + TOLERANCE &&
                y1 - TOLERANCE <= y && y <= y1 + TOLERANCE )
            {
                intersects = TRUE;
                break;
            }

            dx = x2 - x1;
            dy = y2 - y1;
            tx = x - x1;
            ty = y - y1;

            len = dx * dx + dy * dy;
            if( len == 0.0 )
                continue;

            t = (tx * dx + ty * dy) / len;

            if( t < 0.0 || t > 1.0 )
                continue;

            tx = tx - t * dx;
            ty = ty - t * dy;

            if( tx * tx + ty * ty < TOLERANCE * TOLERANCE )
            {
                intersects = TRUE;
                break;
            }
        }
    }

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_quadmesh_patch
@INPUT      : ray_origin
              ray_direction
              dist
              quadmesh
              obj_index
@OUTPUT     : 
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if a ray intersects a given quadrilateral of a quadmesh.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN   intersect_ray_quadmesh_patch(
    Point            *ray_origin,
    Vector           *ray_direction,
    Real             *dist,
    quadmesh_struct  *quadmesh,
    int              obj_index )
{
    BOOLEAN  intersects;
    Point    points[4];
    int      i, j, m, n;

    get_quadmesh_n_objects( quadmesh, &m, &n );

    i = obj_index / n;
    j = obj_index % n;

    get_quadmesh_patch( quadmesh, i, j, points );

    intersects = intersect_ray_polygon_points( ray_origin, ray_direction,
                                               4, points, dist );

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : line_intersects_ellipsoid
@INPUT      : line_origin
              line_direction
              sphere_centre
              x_size
              y_size
              z_size
@OUTPUT     : t_min
              t_max
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if the line intersects the ellipsoid, and if so, passes
              back the enter and exit distance, as a multiple of line_direction.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  line_intersects_ellipsoid(
    Point    *line_origin,
    Vector   *line_direction,
    Point    *sphere_centre,
    Real     x_size,
    Real     y_size,
    Real     z_size,
    Real     *t_min,
    Real     *t_max )
{
    BOOLEAN   intersects;
    int       n_solutions;
    Real      a, b, c, ox, oy, oz, dx, dy, dz, t1, t2;

    ox = ((Real) Point_x(*line_origin) - (Real) Point_x(*sphere_centre)) /
         x_size;
    oy = ((Real) Point_y(*line_origin) - (Real) Point_y(*sphere_centre)) /
         y_size;
    oz = ((Real) Point_z(*line_origin) - (Real) Point_z(*sphere_centre)) /
         z_size;

    dx = (Real) Vector_x(*line_direction) / x_size;
    dy = (Real) Vector_y(*line_direction) / y_size;
    dz = (Real) Vector_z(*line_direction) / z_size;

    a = dx * dx + dy * dy + dz * dz;
    b = 2.0 * ( ox * dx + oy * dy + oz * dz);
    c = ox * ox + oy * oy + oz * oz - 1.0;

    n_solutions = solve_quadratic( a, b, c, &t1, &t2 );

    if( n_solutions == 0 )
        intersects = FALSE;
    else if( n_solutions == 1 )
    {
        intersects = TRUE;
        *t_min = t1;
        *t_max = t2;
    }
    else
    {
        intersects = TRUE;
        *t_min = MIN( t1, t2 );
        *t_max = MAX( t1, t2 );
    }

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : ray_intersects_sphere
@INPUT      : origin
              direction
              centre
              radius
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if a ray intersects a sphere.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  ray_intersects_sphere(
    Point       *origin,
    Vector      *direction,
    Point       *centre,
    Real        radius,
    Real        *dist )
{
    BOOLEAN  intersects;
    Real     t_min, t_max;

    intersects = line_intersects_ellipsoid( origin, direction, centre,
                                            radius, radius, radius,
                                            &t_min, &t_max );

    if( intersects )
    {
        if( t_min >= 0.0 )
            *dist = t_min;
        else if( t_max >= 0.0 )
            *dist = t_max;
        else
            intersects = FALSE;
    }

    return( intersects );
}

private  BOOLEAN  ray_intersects_box_bounding_tube(
    Point       *origin,
    Vector      *direction,
    Point       *p1,
    Point       *p2,
    Real        radius )
{
    Real       len_axis, x, y, z, t_min, t_max;
    Point      trans_origin;
    Vector     tube_axis, hor, vert, trans_direction;
    Transform  transform;

    SUB_POINTS( tube_axis, *p2, *p1 );
    create_two_orthogonal_vectors( &tube_axis, &hor, &vert );
    len_axis = MAGNITUDE( tube_axis );
    if( len_axis != 0.0 )
    {
        SCALE_VECTOR( tube_axis, tube_axis, 1.0 / len_axis );
        NORMALIZE_VECTOR( hor, hor );
        NORMALIZE_VECTOR( vert, vert );
        make_change_from_bases_transform( p1, &hor, &vert, &tube_axis,
                                          &transform );
    }
    else
    {
        make_translation_transform( -RPoint_x(*p1), -RPoint_y(*p1),
                                    -RPoint_z(*p1), &transform );
    }

    transform_point( &transform,
                     RPoint_x(*origin), RPoint_y(*origin), RPoint_z(*origin),
                     &x, &y, &z );
    fill_Point( trans_origin, x, y, z );

    transform_vector( &transform,
          RVector_x(*direction), RVector_y(*direction), RVector_z(*direction),
          &x, &y, &z );
    fill_Vector( trans_direction, x, y, z );

    return( clip_line_to_box( &trans_origin, &trans_direction,
                              -radius, radius, -radius, radius,
                              -radius, len_axis + radius, &t_min, &t_max ) );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : ray_intersects_tube
@INPUT      : origin
              direction
              p1
              p2
              radius
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if a ray intersects a cylinder from p1 to p2 with given
              radius.  Solution of a quadratic is required.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN  ray_intersects_tube(
    Point       *origin,
    Vector      *direction,
    Point       *p1,
    Point       *p2,
    Real        radius,
    Real        *dist )
{
    Real     o_dot_o, o_dot_v, v_dot_v, d_dot_d, d_dot_v, d_dot_o, d;
    Real     vx, vy, vz, ox, oy, oz, len_v;
    Point    point;
    Vector   offset;
    int      n_sols;
    Real     a, b, c, sols[2], t_min, t_max;

    if( !ray_intersects_box_bounding_tube( origin, direction, p1, p2, radius ) )
        return( FALSE );

    vx = (Real) Point_x(*p2) - (Real) Point_x(*p1);
    vy = (Real) Point_y(*p2) - (Real) Point_y(*p1);
    vz = (Real) Point_z(*p2) - (Real) Point_z(*p1);

    len_v = sqrt( vx*vx + vy*vy + vz*vz );
    if( len_v == 0.0 )
        len_v = 1.0;

    ox = (Real) Point_x(*origin) - (Real) Point_x(*p1);
    oy = (Real) Point_y(*origin) - (Real) Point_y(*p1);
    oz = (Real) Point_z(*origin) - (Real) Point_z(*p1);

    o_dot_o = ox * ox + oy * oy + oz * oz;
    o_dot_v = (ox * vx + oy * vy + oz * vz) / len_v;
    v_dot_v = 1.0;
    d_dot_d = DOT_VECTORS( *direction, *direction );
    d_dot_o = (Real) Vector_x(*direction) * ox +
              (Real) Vector_y(*direction) * oy +
              (Real) Vector_z(*direction) * oz;
    d_dot_v = ((Real) Vector_x(*direction) * vx +
               (Real) Vector_y(*direction) * vy +
               (Real) Vector_z(*direction) * vz) / len_v;
    a = d_dot_d - 2.0 * d_dot_v * d_dot_v + v_dot_v * d_dot_v * d_dot_v;
    b = 2.0 * d_dot_o - 4.0 * o_dot_v * d_dot_v +
        2.0 * v_dot_v * o_dot_v * d_dot_v;
    c = o_dot_o - 2.0 * o_dot_v * o_dot_v + v_dot_v * o_dot_v * o_dot_v -
         radius * radius;

    n_sols = solve_quadratic( a, b, c, &sols[0], &sols[1] );

    if( n_sols == 0 )
        *dist = -1.0;
    else if( n_sols == 1 )
        *dist = sols[0];
    else
    {
        t_min = MIN( sols[0], sols[1] );
        t_max = MAX( sols[0], sols[1] );
        if( t_min >= 0.0 )
            *dist = t_min;
        else if( t_max >= 0.0 )
            *dist = t_max;
        else
            *dist = -1.0;
    }

    /*--- if intersects infinitely long tube, check if it intersects
          between p1 and p2 */

    if( *dist >= 0.0 )
    {
        GET_POINT_ON_RAY( point, *origin, *direction, *dist );
        SUB_POINTS( offset, point, *p1 );
        d = (vx * (Real) Vector_x(offset) +
             vy * (Real) Vector_y(offset) +
             vz * (Real) Vector_z(offset)) / len_v;
        if( d < 0.0 || d > distance_between_points( p1, p2 ) )
            *dist = -1.0;
    }

    return( *dist >= 0.0 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_tube_segment
@INPUT      : origin
              direction
              lines
              obj_index
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if a ray intersects the tube segment corresponding to
              a line segment.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN   intersect_ray_tube_segment(
    Point            *origin,
    Vector           *direction,
    Real             *dist,
    lines_struct     *lines,
    int              obj_index )
{
    Real     a_dist;
    int      line, seg, p1, p2;
    BOOLEAN  found;

    get_line_segment_index( lines, obj_index, &line, &seg );

    p1 = lines->indices[POINT_INDEX(lines->end_indices,line,seg)];
    p2 = lines->indices[POINT_INDEX(lines->end_indices,line,seg+1)];
    
    found = ray_intersects_sphere( origin, direction, &lines->points[p1],
                                   (Real) lines->line_thickness, dist );

    if( ray_intersects_sphere( origin, direction, &lines->points[p2],
                               (Real) lines->line_thickness, &a_dist ) &&
        (!found || a_dist < *dist ) )
    {
        found = TRUE;
        *dist = a_dist;
    }

    if( ray_intersects_tube( origin, direction,
                             &lines->points[p1],
                             &lines->points[p2],
                             (Real) lines->line_thickness, &a_dist ) &&
        (!found || a_dist < *dist ) )
    {
        found = TRUE;
        *dist = a_dist;
    }

    return( found );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_with_cube
@INPUT      : ray_origin
              ray_direction
              centre
              size
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if the ray intersects the cube.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN  intersect_ray_with_cube(
    Point            *ray_origin,
    Vector           *ray_direction,
    Point            *centre,
    Real             size,
    Real             *dist )
{
    Real      t_min, t_max;
    BOOLEAN   intersects;

    intersects = clip_line_to_box( ray_origin, ray_direction,
                                   (Real) Point_x(*centre) - size / 2.0,
                                   (Real) Point_x(*centre) + size / 2.0,
                                   (Real) Point_y(*centre) - size / 2.0,
                                   (Real) Point_y(*centre) + size / 2.0,
                                   (Real) Point_z(*centre) - size / 2.0,
                                   (Real) Point_z(*centre) + size / 2.0,
                                   &t_min, &t_max );

    if( intersects )
    {
        if( t_min >= 0.0 )
            *dist = t_min;
        else if( t_max >= 0.0 )
            *dist = t_max;
        else
            intersects = FALSE;
    }

    return( intersects );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_with_marker
@INPUT      : ray_origin
              ray_direction
              marker
@OUTPUT     : dist
@RETURNS    : TRUE if intersects
@DESCRIPTION: Tests if the ray intersects the marker, which can be a cube
              or a sphere.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  BOOLEAN  intersect_ray_with_marker(
    Point            *ray_origin,
    Vector           *ray_direction,
    marker_struct    *marker,
    Real             *dist )
{
    if( marker->type == BOX_MARKER )
    {
        return( intersect_ray_with_cube( ray_origin, ray_direction,
                                        &marker->position, marker->size, dist));
    }
    else
    {
        return( ray_intersects_sphere( ray_origin, ray_direction,
                                       &marker->position, marker->size, dist) );
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_object
@INPUT      : origin
              direction
              object
              obj_index
@OUTPUT     : closest_obj_index
              closest_dist
              n_intersections
              distances
@RETURNS    : 
@DESCRIPTION: Tests if the ray intersects the given object.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  intersect_ray_object(
    Point                 *origin,
    Vector                *direction,
    object_struct         *object,
    int                   obj_index,
    int                   *closest_obj_index,
    Real                  *closest_dist,
    int                   *n_intersections,
    Real                  *distances[] )
{
    BOOLEAN               found;
    Real                  dist;

    if( get_object_type( object ) == POLYGONS )
    {
        found = intersect_ray_polygon( origin, direction, &dist,
                                       get_polygons_ptr(object), obj_index );
    }
    else if( get_object_type( object ) == QUADMESH )
    {
        found = intersect_ray_quadmesh_patch( origin, direction, &dist,
                                        get_quadmesh_ptr(object), obj_index );
    }
    else if( get_object_type( object ) == LINES )
    {
        found = intersect_ray_tube_segment( origin, direction, &dist,
                                            get_lines_ptr(object), obj_index );
    }
    else if( get_object_type( object ) == MARKER )
    {
        found = intersect_ray_with_marker( origin, direction,
                                           get_marker_ptr(object), &dist );
    }

    if( found )
    {
        if( distances != (Real **) NULL )
        {
            SET_ARRAY_SIZE( *distances, *n_intersections,
                            *n_intersections + 1, DEFAULT_CHUNK_SIZE );
            (*distances)[*n_intersections] = dist;
        }

        if( closest_obj_index != (int *) NULL &&
            ((*n_intersections == 0) || dist < *closest_dist ) )
        {
            *closest_obj_index = obj_index;
            *closest_dist = dist;
        }

        ++(*n_intersections);
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : intersect_ray_with_object
@INPUT      : origin
              direction
              object
@OUTPUT     : obj_index
              dist
              distances
@RETURNS    : n_intersections
@DESCRIPTION: Tests for intersection of the ray with an object.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  int  intersect_ray_with_object(
    Point           *origin,
    Vector          *direction,
    object_struct   *object,
    int             *obj_index,
    Real            *dist,
    Real            *distances[] )
{
    lines_struct     *lines;
    polygons_struct  *polygons;
    quadmesh_struct  *quadmesh;
    int              i, n_intersections, m, n, n_objects;

    n_intersections = 0;
    if( obj_index != (int *) NULL )
        *obj_index = -1;

    switch( get_object_type( object ) )
    {
    case LINES:
        lines = get_lines_ptr( object );
        if( lines->n_items == 0 )
        {
            n_objects = 0;
            break;
        }

        if( lines->bintree != (bintree_struct_ptr) NULL )
        {
            return( intersect_ray_with_bintree( origin, direction,
                                                lines->bintree, object,
                                                obj_index, dist, distances ) );
        }
   
        n_objects = lines->end_indices[lines->n_items-1] - lines->n_items;
        break;

    case POLYGONS:
        polygons = get_polygons_ptr( object );
        if( polygons->bintree != (bintree_struct_ptr) NULL )
        {
            return( intersect_ray_with_bintree( origin, direction,
                                                polygons->bintree, object,
                                                obj_index, dist, distances ) );
        }
        n_objects = polygons->n_items;
        break;

    case QUADMESH:
        quadmesh = get_quadmesh_ptr( object );
        if( quadmesh->bintree != (bintree_struct_ptr) NULL )
        {
            return( intersect_ray_with_bintree( origin, direction,
                                                quadmesh->bintree, object,
                                                obj_index, dist, distances ) );
        }
        get_quadmesh_n_objects( quadmesh, &m, &n );
        n_objects = m * n;
        break;

    case MARKER:
        n_objects = 1;
        break;

    default:
        n_objects = 0;
        break;
    }

    for_less( i, 0, n_objects )
    {
        intersect_ray_object( origin, direction, object, i, obj_index,
                              dist, &n_intersections, distances );
    }

    return( n_intersections );
}
