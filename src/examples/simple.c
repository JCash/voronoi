// Contribution by: Abe Tusk https://github.com/abetusk
// To compile:
// gcc -Wall -Weverything -Wno-float-equal src/examples/simple.c -Isrc -o simple
//
// About:
//
// This example outputs 10 random 2D coordinates, and all the generated edges, to standard output.
// Note that the edges have duplicates, but you can easily filter them out.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
//#define JCV_CEIL ceil
//#define JCV_FLOOR floor
//#define JCV_FLT_MAX 1.7976931348623157E+308
#include "jc_voronoi.h"

#define NPOINT 10

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  int i;
  jcv_rect bounding_box = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
  jcv_diagram diagram;
  jcv_point points[NPOINT];
  const jcv_site* sites;
  jcv_graphedge* graph_edge;

  memset(&diagram, 0, sizeof(jcv_diagram));

  srand(0);
  for (i=0; i<NPOINT; i++) {
    points[i].x = ((float)rand()/(1.0f + (float)RAND_MAX));
    points[i].y = ((float)rand()/(1.0f + (float)RAND_MAX));
  }

  printf("# Seed sites\n");
  for (i=0; i<NPOINT; i++) {
    printf("%f %f\n", (double)points[i].x, (double)points[i].y);
  }

  jcv_diagram_generate(NPOINT, (const jcv_point *)points, &bounding_box, 0, &diagram);

  printf("# Edges\n");
  sites = jcv_diagram_get_sites(&diagram);
  for (i=0; i<diagram.numsites; i++) {

    graph_edge = sites[i].edges;
    while (graph_edge) {
      // This approach will potentially print shared edges twice
      printf("%f %f\n", (double)graph_edge->pos[0].x, (double)graph_edge->pos[0].y);
      printf("%f %f\n", (double)graph_edge->pos[1].x, (double)graph_edge->pos[1].y);
      graph_edge = graph_edge->next;
    }
  }

  jcv_diagram_free(&diagram);
}
