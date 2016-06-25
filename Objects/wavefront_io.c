#include "bicpl_internal.h"
#include <ctype.h>

/**
 * \file wavefront_io.c
 * \brief Input Wavefront surface files.
 *
 * \copyright
              Copyright 1993-2016 David MacDonald and Robert D. Vincent
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
 */
#define DFS_MAGIC_N_BYTES 12
#define N_PER_TRIANGLE 3

#define SIZEOF_POINT ( sizeof( float ) * N_PER_TRIANGLE )
#define SIZEOF_TRIANGLE (sizeof( int32_t ) * N_PER_TRIANGLE)

/**
 * Read a WaveFront .obj format surface file, saving it as a
 * POLYGONS object in our internal format.
 * \param fp An open file pointer set to the beginning of the file.
 * \param object_ptr A POLYGONS object pointer that has already been created
 * but not necessarily initialized.
 * \returns TRUE if successful.
 */
static VIO_BOOL
input_wavefront_surface_file( FILE *fp, object_struct *object_ptr )
{
  char line[1024];
  int n_indices = 0;
  int n_normals = 0;
  char kw[1024];
  polygons_struct *poly_ptr = get_polygons_ptr( object_ptr );

  initialize_polygons( poly_ptr, WHITE, NULL );

  while (fgets( line, sizeof(line), fp ))
  {
    char *p = kw;
    char *s = line;

    if (*s == '#')
    {
      continue;
    }

    while (isspace(*s))
    {
      s++;
    }

    if (*s == '\n' || *s == '\0')
    {
      continue;
    }

    while (isalpha(*s))
    {
      *p++ = *s++;
    }
    *p = 0;

    while (isspace(*s))
    {
      s++;
    }

    if (!strcmp(kw, "v"))
    {
      VIO_Point pt;
      sscanf(s, "%f %f %f", &Point_x(pt), &Point_y(pt), &Point_z(pt));
      ADD_ELEMENT_TO_ARRAY( poly_ptr->points, poly_ptr->n_points,
                            pt, DEFAULT_CHUNK_SIZE );
    }
    else if (!strcmp(kw, "vn"))
    {
      VIO_Vector vc;
      sscanf(s, "%f %f %f", &Vector_x(vc), &Vector_y(vc), &Vector_z(vc));
      ADD_ELEMENT_TO_ARRAY(poly_ptr->normals, n_normals,
                           vc, DEFAULT_CHUNK_SIZE);
    }
    else if (!strcmp(kw, "vt"))
    {
      /* just ignore */
    }
    else if (!strcmp(kw, "vp"))
    {
      /* just ignore */
    }
    else if (!strcmp(kw, "f"))
    {
      while (*s != '\0')
      {
        while (isspace(*s))
        {
          s++;
        }
        if (isdigit(*s))
        {
          int n = 0;
          while (isdigit(*s))
          {
            n = n * 10 + *s++ - '0';
          }

          if (n < 0)
          {
            /* handle relative indices? */
            printf("%d\n", n);
            return FALSE;
          }
          n = (n - 1) % (poly_ptr->n_points);
          ADD_ELEMENT_TO_ARRAY( poly_ptr->indices, n_indices, n,
                                DEFAULT_CHUNK_SIZE );
        }
        if (*s == '/')
        {
          s++;
          while (isdigit(*s))
          {
            s++;
          }
          if (*s == '/')
          {
            s++;
            while (isdigit(*s))
            {
              s++;
            }
          }
        }
      }
      ADD_ELEMENT_TO_ARRAY(poly_ptr->end_indices, poly_ptr->n_items,
                           n_indices, DEFAULT_CHUNK_SIZE );
    }
    else if (!strcmp(kw, "usemtl") ||
             !strcmp(kw, "mtllib") ||
             !strcmp(kw, "o") ||
             !strcmp(kw, "s") ||
             !strcmp(kw, "g"))
    {
      /* ignore */
    }
    else
    {
      printf("Unknown record type '%s'\n", kw);
      return FALSE;
    }
  }

  if (n_normals == 0)
  {
    ALLOC( poly_ptr->normals, poly_ptr->n_points );
    compute_polygon_normals( poly_ptr );
  }
  return TRUE;
}

/**
 * Function to read BrainSuite geometric files into the internal
 * object representation.
 * \param filename The path to the file.
 * \param n_objects A pointer to the object count field of the model.
 * \param object_list A pointer to the object list field of the model.
 * \returns TRUE if successful.
 */
BICAPI VIO_BOOL
input_wavefront_graphics_file(const char *filename,
                               int *n_objects,
                               object_struct **object_list[])
{
  VIO_BOOL result = FALSE;
  FILE *fp;

  if ( ( fp = fopen( filename, "r" )) != NULL )
  {
    object_struct *object_ptr = create_object( POLYGONS );

    if ( !input_wavefront_surface_file( fp, object_ptr ) )
    {
      delete_object( object_ptr );
    }
    else
    {
      add_object_to_list( n_objects, object_list, object_ptr );
      result = TRUE;
    }
    fclose( fp );
  }
  return result;
}
