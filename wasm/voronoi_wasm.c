#include <stddef.h>
#include <stdlib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

/* Compact immutable output for the ergonomic JavaScript API. The construction
 * diagram is released before this snapshot is returned. */
typedef struct jcv_wasm_edge_
{
    jcv_site* sites[2];
    jcv_point pos[2];
    int vertices[2];
} jcv_wasm_edge;

typedef struct jcv_wasm_diagram_
{
    int input_count;
    int site_count;
    int vertex_count;
    jcv_site* sites;
    jcv_site** sites_by_input;
    int edge_count;
    jcv_wasm_edge* edges;
    int* cell_edge_offsets;
    uint32_t* cell_edge_refs;
} jcv_wasm_diagram;

enum
{
    JCV_WASM_CELL_SITE_FLIP = 1,
    JCV_WASM_CELL_POSITION_FLIP = 2,
    JCV_WASM_CELL_EDGE_SHIFT = 2
};

static void jcv_wasm_diagram_release(jcv_wasm_diagram* handle)
{
    if (handle == NULL)
        return;
    free(handle->cell_edge_refs);
    free(handle->cell_edge_offsets);
    free(handle->edges);
    free(handle->sites_by_input);
    free(handle->sites);
    free(handle);
}

static uint32_t jcv_wasm_edge_hash(int vertex0, int vertex1)
{
    uint32_t a = (uint32_t)(vertex0 < vertex1 ? vertex0 : vertex1);
    uint32_t b = (uint32_t)(vertex0 < vertex1 ? vertex1 : vertex0);
    uint32_t hash = a * UINT32_C(0x9e3779b1) ^ b * UINT32_C(0x85ebca6b);
    hash ^= hash >> 16;
    return hash;
}

static size_t jcv_wasm_hash_capacity(int count)
{
    size_t capacity = 1;
    size_t required = count > 0 ? (size_t)count * 2 : 1;
    while (capacity < required)
        capacity <<= 1;
    return capacity;
}

static int jcv_wasm_find_edge(const jcv_wasm_diagram* handle,
                              const uint32_t* edge_hash,
                              size_t hash_capacity,
                              int vertex0,
                              int vertex1)
{
    size_t slot = (size_t)jcv_wasm_edge_hash(vertex0, vertex1) & (hash_capacity - 1);
    int min_vertex = vertex0 < vertex1 ? vertex0 : vertex1;
    int max_vertex = vertex0 < vertex1 ? vertex1 : vertex0;
    while (edge_hash[slot] != 0)
    {
        int edge_index = (int)edge_hash[slot] - 1;
        const jcv_wasm_edge* edge = &handle->edges[edge_index];
        int edge_min = edge->vertices[0] < edge->vertices[1] ? edge->vertices[0] : edge->vertices[1];
        int edge_max = edge->vertices[0] < edge->vertices[1] ? edge->vertices[1] : edge->vertices[0];
        if (edge_min == min_vertex && edge_max == max_vertex)
            return edge_index;
        slot = (slot + 1) & (hash_capacity - 1);
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
jcv_wasm_diagram* jcv_wasm_diagram_create(const float* xy,
                                          int num_points,
                                          float min_x,
                                          float min_y,
                                          float max_x,
                                          float max_y)
{
    jcv_wasm_diagram* handle;
    jcv_diagram diagram = {0};
    const jcv_site* sites;
    jcv_rect rect;
    jcv_edge_iter iter;
    jcv_edge edge;
    uint32_t* edge_hash = NULL;
    size_t hash_capacity = 0;
    int cell_edge_count = 0;
    int i;

    if (num_points < 0 || max_x <= min_x || max_y <= min_y ||
        (num_points > 0 && xy == NULL))
        return NULL;

    handle = (jcv_wasm_diagram*)calloc(1, sizeof(jcv_wasm_diagram));
    if (handle == NULL)
        return NULL;
    handle->input_count = num_points;

    rect.min.x = min_x;
    rect.min.y = min_y;
    rect.max.x = max_x;
    rect.max.y = max_y;
    jcv_diagram_generate(num_points, (const jcv_point*)xy, &rect, NULL, &diagram);

    handle->site_count = diagram.numsites;
    handle->vertex_count = diagram.numvertices;
    if (handle->site_count > 0)
    {
        handle->sites = (jcv_site*)malloc(sizeof(jcv_site) * (size_t)handle->site_count);
        if (handle->sites == NULL)
            goto failure;
        memcpy(handle->sites, jcv_diagram_get_sites(&diagram),
               sizeof(jcv_site) * (size_t)handle->site_count);
    }

    if (num_points > 0)
    {
        handle->sites_by_input = (jcv_site**)calloc((size_t)num_points, sizeof(jcv_site*));
        if (handle->sites_by_input == NULL)
            goto failure;
    }
    sites = jcv_diagram_get_sites(&diagram);
    for (i = 0; i < handle->site_count; ++i)
    {
        if (sites[i].index < (uint32_t)num_points)
            handle->sites_by_input[sites[i].index] = &handle->sites[i];
    }

    handle->edge_count = jcv_diagram_get_edge_count(&diagram);
    if (handle->edge_count > 0)
    {
        int edge_index = 0;
        handle->edges = (jcv_wasm_edge*)malloc(sizeof(jcv_wasm_edge) * (size_t)handle->edge_count);
        if (handle->edges == NULL)
            goto failure;
        jcv_diagram_get_edges(&diagram, &iter);
        while (edge_index < handle->edge_count && jcv_edge_next(&iter, &edge))
        {
            jcv_wasm_edge* output = &handle->edges[edge_index++];
            output->sites[0] = edge.sites[0] != NULL ? handle->sites_by_input[edge.sites[0]->index] : NULL;
            output->sites[1] = edge.sites[1] != NULL ? handle->sites_by_input[edge.sites[1]->index] : NULL;
            output->pos[0] = edge.pos[0];
            output->pos[1] = edge.pos[1];
            output->vertices[0] = edge.vertices[0];
            output->vertices[1] = edge.vertices[1];
        }
        handle->edge_count = edge_index;
    }

    hash_capacity = jcv_wasm_hash_capacity(handle->edge_count);
    edge_hash = (uint32_t*)calloc(hash_capacity, sizeof(uint32_t));
    if (edge_hash == NULL)
        goto failure;
    for (i = 0; i < handle->edge_count; ++i)
    {
        size_t slot = (size_t)jcv_wasm_edge_hash(handle->edges[i].vertices[0],
                                                 handle->edges[i].vertices[1]) & (hash_capacity - 1);
        while (edge_hash[slot] != 0)
            slot = (slot + 1) & (hash_capacity - 1);
        edge_hash[slot] = (uint32_t)i + 1;
    }

    handle->cell_edge_offsets = (int*)calloc((size_t)num_points + 1, sizeof(int));
    if (handle->cell_edge_offsets == NULL)
        goto failure;
    for (i = 0; i < num_points; ++i)
    {
        const jcv_site* site = NULL;
        if (handle->sites_by_input[i] != NULL)
            site = &sites[handle->sites_by_input[i] - handle->sites];
        if (site != NULL)
        {
            jcv_site_get_edges(&diagram, site, &iter);
            while (jcv_edge_next(&iter, &edge))
                ++cell_edge_count;
        }
        handle->cell_edge_offsets[i + 1] = cell_edge_count;
    }

    if (cell_edge_count > 0)
    {
        int cell_edge_index = 0;
        handle->cell_edge_refs = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)cell_edge_count);
        if (handle->cell_edge_refs == NULL)
            goto failure;
        for (i = 0; i < num_points; ++i)
        {
            const jcv_site* site = NULL;
            if (handle->sites_by_input[i] != NULL)
                site = &sites[handle->sites_by_input[i] - handle->sites];
            if (site == NULL)
                continue;
            jcv_site_get_edges(&diagram, site, &iter);
            while (jcv_edge_next(&iter, &edge))
            {
                int edge_index = jcv_wasm_find_edge(handle, edge_hash, hash_capacity,
                                                    edge.vertices[0], edge.vertices[1]);
                uint32_t flags = 0;
                const jcv_wasm_edge* stored;
                if (edge_index < 0 || (uint32_t)edge_index > (UINT32_MAX >> JCV_WASM_CELL_EDGE_SHIFT))
                    goto failure;
                stored = &handle->edges[edge_index];
                if (edge.sites[0] != NULL && stored->sites[0] != NULL &&
                    edge.sites[0]->index != stored->sites[0]->index)
                    flags |= JCV_WASM_CELL_SITE_FLIP;
                if (edge.vertices[0] != stored->vertices[0])
                    flags |= JCV_WASM_CELL_POSITION_FLIP;
                handle->cell_edge_refs[cell_edge_index++] =
                    ((uint32_t)edge_index << JCV_WASM_CELL_EDGE_SHIFT) | flags;
            }
        }
    }
    free(edge_hash);
    jcv_diagram_free(&diagram);
    return handle;

failure:
    free(edge_hash);
    if (diagram.internal != NULL)
        jcv_diagram_free(&diagram);
    jcv_wasm_diagram_release(handle);
    return NULL;
}

EMSCRIPTEN_KEEPALIVE
void jcv_wasm_diagram_destroy(jcv_wasm_diagram* handle)
{
    jcv_wasm_diagram_release(handle);
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_diagram_input_count(const jcv_wasm_diagram* handle)
{
    return handle != NULL ? handle->input_count : 0;
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_diagram_site_count(const jcv_wasm_diagram* handle)
{
    return handle != NULL ? handle->site_count : 0;
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_diagram_vertex_count(const jcv_wasm_diagram* handle)
{
    return handle != NULL ? handle->vertex_count : 0;
}

EMSCRIPTEN_KEEPALIVE
const jcv_site* jcv_wasm_diagram_site(const jcv_wasm_diagram* handle, int input_index)
{
    if (handle == NULL || input_index < 0 || input_index >= handle->input_count)
        return NULL;
    return handle->sites_by_input[input_index];
}

EMSCRIPTEN_KEEPALIVE
const jcv_site* jcv_wasm_diagram_site_at(const jcv_wasm_diagram* handle, int site_index)
{
    if (handle == NULL || site_index < 0 || site_index >= handle->site_count)
        return NULL;
    return &handle->sites[site_index];
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_diagram_edge_count(const jcv_wasm_diagram* handle)
{
    return handle != NULL ? handle->edge_count : 0;
}

EMSCRIPTEN_KEEPALIVE
const jcv_wasm_edge* jcv_wasm_diagram_edge(const jcv_wasm_diagram* handle, int edge_index)
{
    if (handle == NULL || edge_index < 0 || edge_index >= handle->edge_count)
        return NULL;
    return &handle->edges[edge_index];
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_cell_edge_count(const jcv_wasm_diagram* handle, int input_index)
{
    if (handle == NULL || input_index < 0 || input_index >= handle->input_count ||
        handle->sites_by_input[input_index] == NULL)
        return 0;
    return handle->cell_edge_offsets[input_index + 1] - handle->cell_edge_offsets[input_index];
}

EMSCRIPTEN_KEEPALIVE
const jcv_wasm_edge* jcv_wasm_cell_edge(const jcv_wasm_diagram* handle, int input_index, int edge_index)
{
    int begin;
    int count;
    uint32_t reference;
    if (handle == NULL || input_index < 0 || input_index >= handle->input_count ||
        handle->sites_by_input[input_index] == NULL)
        return NULL;
    begin = handle->cell_edge_offsets[input_index];
    count = handle->cell_edge_offsets[input_index + 1] - begin;
    if (edge_index < 0 || edge_index >= count)
        return NULL;
    reference = handle->cell_edge_refs[begin + edge_index];
    return &handle->edges[reference >> JCV_WASM_CELL_EDGE_SHIFT];
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_cell_edge_flags(const jcv_wasm_diagram* handle, int input_index, int edge_index)
{
    int begin;
    int count;
    if (handle == NULL || input_index < 0 || input_index >= handle->input_count ||
        handle->sites_by_input[input_index] == NULL)
        return 0;
    begin = handle->cell_edge_offsets[input_index];
    count = handle->cell_edge_offsets[input_index + 1] - begin;
    if (edge_index < 0 || edge_index >= count)
        return 0;
    return (int)(handle->cell_edge_refs[begin + edge_index] &
                 ((UINT32_C(1) << JCV_WASM_CELL_EDGE_SHIFT) - 1));
}

EMSCRIPTEN_KEEPALIVE
const jcv_point* jcv_wasm_site_point(const jcv_site* site)
{
    return site != NULL ? &site->p : NULL;
}

EMSCRIPTEN_KEEPALIVE
uint32_t jcv_wasm_site_index(const jcv_site* site)
{
    return site != NULL ? site->index : 0;
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_site_boundary(const jcv_site* site)
{
    return site != NULL ? (int)site->boundary : 0;
}

EMSCRIPTEN_KEEPALIVE
float jcv_wasm_point_x(const jcv_point* point)
{
    return point != NULL ? point->x : 0.0f;
}

EMSCRIPTEN_KEEPALIVE
float jcv_wasm_point_y(const jcv_point* point)
{
    return point != NULL ? point->y : 0.0f;
}

EMSCRIPTEN_KEEPALIVE
const jcv_site* jcv_wasm_edge_site(const jcv_wasm_edge* edge, int index)
{
    return edge != NULL && index >= 0 && index < 2 ? edge->sites[index] : NULL;
}

EMSCRIPTEN_KEEPALIVE
const jcv_point* jcv_wasm_edge_position(const jcv_wasm_edge* edge, int index)
{
    return edge != NULL && index >= 0 && index < 2 ? &edge->pos[index] : NULL;
}

EMSCRIPTEN_KEEPALIVE
int jcv_wasm_edge_vertex(const jcv_wasm_edge* edge, int index)
{
    return edge != NULL && index >= 0 && index < 2 ? edge->vertices[index] : -1;
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
