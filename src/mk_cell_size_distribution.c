/*
  mknorps, 2017.09.07
  - computes cell areas 
  - creates histogram of cell areas
  - computes best fit gamma function G
  - compares G to gamma function for point distributed randomly (uniform distribution)
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h> // printf

#if defined(_MSC_VER)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif



#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

#include "mk_cell_size_distribution.h"
