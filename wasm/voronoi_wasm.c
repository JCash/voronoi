#include <stddef.h>
#include <stdlib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

/* Benchmark entry points keep all measured work on the Wasm side. The input
 * array is allocated and populated by JavaScript before the timer starts. */
static volatile float jcv_benchmark_sink;

static int jcv_benchmark_generate_diagram(const float* xy,
                                          int num_points,
                                          float min_x,
                                          float min_y,
                                          float max_x,
                                          float max_y,
                                          int delauney_only,
                                          jcv_diagram* diagram)
{
    jcv_rect rect;

    if (diagram == NULL || num_points < 0 || max_x <= min_x || max_y <= min_y ||
        (num_points > 0 && xy == NULL))
        return 0;

    rect.min.x = min_x;
    rect.min.y = min_y;
    rect.max.x = max_x;
    rect.max.y = max_y;
    if (delauney_only)
        jcv_delauney_generate(num_points, (const jcv_point*)xy, &rect, NULL, diagram);
    else
        jcv_diagram_generate(num_points, (const jcv_point*)xy, &rect, NULL, diagram);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int jcv_benchmark_generate(const float* xy, int num_points,
                           float min_x, float min_y, float max_x, float max_y)
{
    jcv_diagram diagram = {0};
    int result;

    if (!jcv_benchmark_generate_diagram(xy, num_points, min_x, min_y, max_x, max_y, 0, &diagram))
        return -1;
    result = diagram.numsites;
    jcv_diagram_free(&diagram);
    return result;
}

EMSCRIPTEN_KEEPALIVE
int jcv_benchmark_generate_sites(const float* xy, int num_points,
                                 float min_x, float min_y, float max_x, float max_y)
{
    jcv_diagram diagram = {0};
    const jcv_site* sites;
    int count;

    if (!jcv_benchmark_generate_diagram(xy, num_points, min_x, min_y, max_x, max_y, 0, &diagram))
        return -1;
    sites = jcv_diagram_get_sites(&diagram);
    count = diagram.numsites;
    if (count > 0)
        jcv_benchmark_sink = sites[0].p.x;
    jcv_diagram_free(&diagram);
    return count;
}

EMSCRIPTEN_KEEPALIVE
int jcv_benchmark_generate_edges(const float* xy, int num_points,
                                 float min_x, float min_y, float max_x, float max_y)
{
    jcv_diagram diagram = {0};
    jcv_edge_iter iter;
    jcv_edge edge;
    float checksum = 0.0f;
    int count = 0;

    if (!jcv_benchmark_generate_diagram(xy, num_points, min_x, min_y, max_x, max_y, 0, &diagram))
        return -1;
    jcv_diagram_get_edges(&diagram, &iter);
    while (jcv_edge_next(&iter, &edge))
    {
        checksum += edge.pos[0].x + edge.pos[0].y + edge.pos[1].x + edge.pos[1].y;
        ++count;
    }
    jcv_benchmark_sink = checksum;
    jcv_diagram_free(&diagram);
    return count;
}

EMSCRIPTEN_KEEPALIVE
int jcv_benchmark_generate_delauney(const float* xy, int num_points,
                                    float min_x, float min_y, float max_x, float max_y)
{
    jcv_diagram diagram = {0};
    jcv_delauney_iter iter;
    jcv_delauney_edge edge;
    float checksum = 0.0f;
    int count = 0;

    if (!jcv_benchmark_generate_diagram(xy, num_points, min_x, min_y, max_x, max_y, 1, &diagram))
        return -1;
    jcv_delauney_begin(&diagram, &iter);
    while (jcv_delauney_next(&iter, &edge))
    {
        checksum += edge.pos[0].x + edge.pos[0].y + edge.pos[1].x + edge.pos[1].y;
        ++count;
    }
    jcv_benchmark_sink = checksum;
    jcv_diagram_free(&diagram);
    return count;
}

/* xy contains x/y pairs; each returned edge is x0, y0, x1, y1. */
EMSCRIPTEN_KEEPALIVE
float* jcv_voronoi_edges(const float* xy,
                         int num_points,
                         float width,
                         float height,
                         int* output_count)
{
    jcv_diagram diagram = {0};
    jcv_rect rect;
    jcv_edge_iter iter;
    jcv_edge edge;
    int edge_count;
    int edge_index = 0;
    float* edges;

    if (output_count == NULL)
        return NULL;
    *output_count = -1;
    if (num_points < 0 || width <= 0.0f || height <= 0.0f ||
        (num_points > 0 && xy == NULL))
        return NULL;
    if (num_points == 0)
    {
        *output_count = 0;
        return NULL;
    }

    rect.min.x = 0.0f;
    rect.min.y = 0.0f;
    rect.max.x = width;
    rect.max.y = height;
    jcv_diagram_generate(num_points, (const jcv_point*)xy, &rect, NULL, &diagram);
    edge_count = jcv_diagram_get_edge_count(&diagram);
    edges = edge_count > 0 ? (float*)malloc(sizeof(float) * (size_t)edge_count * 4) : NULL;
    if (edge_count > 0 && edges == NULL)
    {
        jcv_diagram_free(&diagram);
        *output_count = -2;
        return NULL;
    }

    jcv_diagram_get_edges(&diagram, &iter);
    while (jcv_edge_next(&iter, &edge))
    {
        const int offset = edge_index * 4;
        edges[offset + 0] = edge.pos[0].x;
        edges[offset + 1] = edge.pos[0].y;
        edges[offset + 2] = edge.pos[1].x;
        edges[offset + 3] = edge.pos[1].y;
        ++edge_index;
    }
    jcv_diagram_free(&diagram);
    *output_count = edge_count;
    return edges;
}

/* Each Delauney edge connects the two sites separated by a Voronoi edge. */
EMSCRIPTEN_KEEPALIVE
float* jcv_delauney_edges(const float* xy,
                          int num_points,
                          float width,
                          float height,
                          int* output_count)
{
    jcv_diagram diagram = {0};
    jcv_rect rect;
    jcv_delauney_iter iter;
    jcv_delauney_edge edge;
    int edge_count = 0;
    int edge_index = 0;
    float* edges;

    if (output_count == NULL)
        return NULL;
    *output_count = -1;
    if (num_points < 0 || width <= 0.0f || height <= 0.0f ||
        (num_points > 0 && xy == NULL))
        return NULL;
    if (num_points == 0)
    {
        *output_count = 0;
        return NULL;
    }

    rect.min.x = 0.0f;
    rect.min.y = 0.0f;
    rect.max.x = width;
    rect.max.y = height;
    jcv_delauney_generate(num_points, (const jcv_point*)xy, &rect, NULL, &diagram);
    edge_count = jcv_delauney_get_edge_count(&diagram);

    edges = edge_count > 0 ? (float*)malloc(sizeof(float) * (size_t)edge_count * 4) : NULL;
    if (edge_count > 0 && edges == NULL)
    {
        jcv_diagram_free(&diagram);
        *output_count = -2;
        return NULL;
    }

    jcv_delauney_begin(&diagram, &iter);
    while (jcv_delauney_next(&iter, &edge))
    {
        const int offset = edge_index * 4;
        edges[offset + 0] = edge.pos[0].x;
        edges[offset + 1] = edge.pos[0].y;
        edges[offset + 2] = edge.pos[1].x;
        edges[offset + 3] = edge.pos[1].y;
        ++edge_index;
    }
    jcv_diagram_free(&diagram);
    *output_count = edge_count;
    return edges;
}
