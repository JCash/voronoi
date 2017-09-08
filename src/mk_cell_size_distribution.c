/*
  mknorps, 2017.09.07
  - computes cell areas 
  - creates histogram of cell areas
  - computes best fit gamma function G
  - compares G to gamma function for point distributed randomly (uniform distribution)
 */


#include "mk_cell_size_distribution.h"



double* mk_polygons_area ( int num_points, const jcv_point* points, jcv_diagram* diagram )
{
    double* result;
    double number;
    for (int i=0; i<num_points; i++)
    {
	    printf ("point number %d: (%f,%f) \n",i,points[i].x,points[i].y);


	    printf ("xxxxx %d: %lu \n",i,sizeof(diagram->sites[i].index), );



	    number = 0.01;
	    result = &number;
    }

   return result;
}
