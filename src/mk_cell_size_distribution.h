/*
 * mknorps, 2017.09.07
 * header file for computing
 * histograms of voronoi cell areas
 *
 *
 * input : a list of polygons (Voronoi cells)
 *
 */



#include <math.h>
#include <stddef.h>

#pragma once

#include <assert.h>

//#include "jc_voronoi.h"

#ifndef VORONOI_HIST
#define VORONOI_HIST


double* mk_polygons_area ( int num_points, const jcv_point* points, jcv_diagram* diagram );

double* mk_histogram (int num_hist_intervals, double* polygon_areas);


#endif
