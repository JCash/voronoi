#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

/* Relocatable packed output copied into a JavaScript-owned ArrayBuffer. All
 * references are indices or byte offsets, so JavaScript never needs to call
 * back into Wasm while traversing the result. */
enum
{
    JCV_PACK_MAGIC = 0x4a435631,
    JCV_PACK_VERSION = 1,
    JCV_PACK_HEADER_WORDS = 14,
    JCV_PACK_SITE_WORDS = 4,
    JCV_PACK_EDGE_WORDS = 4,
    JCV_PACK_CELL_SITE_FLIP = 1,
    JCV_PACK_CELL_POSITION_FLIP = 2,
    JCV_PACK_CELL_EDGE_SHIFT = 2
};

typedef struct jcv_pack_edge_
{
    int sites[2];
    int vertices[2];
} jcv_pack_edge;

static uint32_t jcv_pack_edge_hash(int vertex0, int vertex1)
{
    uint32_t a = (uint32_t)(vertex0 < vertex1 ? vertex0 : vertex1);
    uint32_t b = (uint32_t)(vertex0 < vertex1 ? vertex1 : vertex0);
    uint32_t hash = a * UINT32_C(0x9e3779b1) ^ b * UINT32_C(0x85ebca6b);
    return hash ^ (hash >> 16);
}

static size_t jcv_pack_hash_capacity(int count)
{
    size_t capacity = 1;
    size_t required = count > 0 ? (size_t)count * 2 : 1;
    while (capacity < required)
        capacity <<= 1;
    return capacity;
}

static int jcv_pack_find_edge(const jcv_pack_edge* edges,
                              const uint32_t* edge_hash,
                              size_t hash_capacity,
                              int vertex0,
                              int vertex1)
{
    size_t slot = (size_t)jcv_pack_edge_hash(vertex0, vertex1) & (hash_capacity - 1);
    int min_vertex = vertex0 < vertex1 ? vertex0 : vertex1;
    int max_vertex = vertex0 < vertex1 ? vertex1 : vertex0;
    while (edge_hash[slot] != 0)
    {
        int edge_index = (int)edge_hash[slot] - 1;
        const jcv_pack_edge* edge = &edges[edge_index];
        int edge_min = edge->vertices[0] < edge->vertices[1] ? edge->vertices[0] : edge->vertices[1];
        int edge_max = edge->vertices[0] < edge->vertices[1] ? edge->vertices[1] : edge->vertices[0];
        if (edge_min == min_vertex && edge_max == max_vertex)
            return edge_index;
        slot = (slot + 1) & (hash_capacity - 1);
    }
    return -1;
}

static int jcv_pack_add_size(size_t* total, size_t count, size_t stride)
{
    if (count > (SIZE_MAX - *total) / stride)
        return 0;
    *total += count * stride;
    return *total <= (size_t)INT_MAX;
}

EMSCRIPTEN_KEEPALIVE
void* jcv_wasm_generate_packed(const float* xy,
                               int num_points,
                               float min_x,
                               float min_y,
                               float max_x,
                               float max_y)
{
    jcv_diagram diagram = {0};
    const jcv_site* sites;
    jcv_rect rect;
    jcv_edge_iter iter;
    jcv_edge edge;
    unsigned char* block = NULL;
    uint32_t* header;
    int32_t* input_to_site;
    uint32_t* site_words;
    float* site_floats;
    float* vertices;
    jcv_pack_edge* edges;
    uint32_t* cell_offsets;
    uint32_t* cell_refs;
    uint32_t* edge_hash = NULL;
    size_t hash_capacity;
    size_t total = JCV_PACK_HEADER_WORDS * sizeof(uint32_t);
    size_t input_offset;
    size_t sites_offset;
    size_t vertices_offset;
    size_t edges_offset;
    size_t cell_offsets_offset;
    size_t cell_refs_offset;
    int site_count;
    int vertex_count;
    int edge_count;
    int cell_edge_count = 0;
    int i;

    if (num_points < 0 || max_x <= min_x || max_y <= min_y ||
        (num_points > 0 && xy == NULL))
        return NULL;

    rect.min.x = min_x;
    rect.min.y = min_y;
    rect.max.x = max_x;
    rect.max.y = max_y;
    jcv_diagram_generate(num_points, (const jcv_point*)xy, &rect, NULL, &diagram);
    sites = jcv_diagram_get_sites(&diagram);
    site_count = diagram.numsites;
    vertex_count = diagram.numvertices;
    edge_count = jcv_diagram_get_edge_count(&diagram);

    for (i = 0; i < site_count; ++i)
    {
        jcv_site_get_edges(&diagram, &sites[i], &iter);
        while (jcv_edge_next(&iter, &edge))
            ++cell_edge_count;
    }

    input_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)num_points, sizeof(int32_t)))
        goto failure;
    sites_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)site_count, JCV_PACK_SITE_WORDS * sizeof(uint32_t)))
        goto failure;
    vertices_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)vertex_count, 2 * sizeof(float)))
        goto failure;
    edges_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)edge_count, sizeof(jcv_pack_edge)))
        goto failure;
    cell_offsets_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)num_points + 1, sizeof(uint32_t)))
        goto failure;
    cell_refs_offset = total;
    if (!jcv_pack_add_size(&total, (size_t)cell_edge_count, sizeof(uint32_t)))
        goto failure;

    block = (unsigned char*)malloc(total);
    if (block == NULL)
        goto failure;
    memset(block, 0, total);
    header = (uint32_t*)block;
    input_to_site = (int32_t*)(block + input_offset);
    site_words = (uint32_t*)(block + sites_offset);
    site_floats = (float*)(block + sites_offset);
    vertices = (float*)(block + vertices_offset);
    edges = (jcv_pack_edge*)(block + edges_offset);
    cell_offsets = (uint32_t*)(block + cell_offsets_offset);
    cell_refs = (uint32_t*)(block + cell_refs_offset);

    header[0] = JCV_PACK_MAGIC;
    header[1] = JCV_PACK_VERSION;
    header[2] = (uint32_t)total;
    header[3] = (uint32_t)num_points;
    header[4] = (uint32_t)site_count;
    header[5] = (uint32_t)vertex_count;
    header[6] = (uint32_t)edge_count;
    header[7] = (uint32_t)cell_edge_count;
    header[8] = (uint32_t)input_offset;
    header[9] = (uint32_t)sites_offset;
    header[10] = (uint32_t)vertices_offset;
    header[11] = (uint32_t)edges_offset;
    header[12] = (uint32_t)cell_offsets_offset;
    header[13] = (uint32_t)cell_refs_offset;

    for (i = 0; i < num_points; ++i)
        input_to_site[i] = -1;
    for (i = 0; i < site_count; ++i)
    {
        int offset = i * JCV_PACK_SITE_WORDS;
        site_floats[offset + 0] = sites[i].p.x;
        site_floats[offset + 1] = sites[i].p.y;
        site_words[offset + 2] = sites[i].index;
        site_words[offset + 3] = sites[i].boundary;
        if (sites[i].index < (uint32_t)num_points)
            input_to_site[sites[i].index] = i;
    }

    i = 0;
    jcv_diagram_get_edges(&diagram, &iter);
    while (i < edge_count && jcv_edge_next(&iter, &edge))
    {
        edges[i].sites[0] = edge.sites[0] != NULL ? (int)edge.sites[0]->index : -1;
        edges[i].sites[1] = edge.sites[1] != NULL ? (int)edge.sites[1]->index : -1;
        edges[i].vertices[0] = edge.vertices[0];
        edges[i].vertices[1] = edge.vertices[1];
        vertices[edge.vertices[0] * 2 + 0] = edge.pos[0].x;
        vertices[edge.vertices[0] * 2 + 1] = edge.pos[0].y;
        vertices[edge.vertices[1] * 2 + 0] = edge.pos[1].x;
        vertices[edge.vertices[1] * 2 + 1] = edge.pos[1].y;
        ++i;
    }
    edge_count = i;
    header[6] = (uint32_t)edge_count;

    hash_capacity = jcv_pack_hash_capacity(edge_count);
    edge_hash = (uint32_t*)calloc(hash_capacity, sizeof(uint32_t));
    if (edge_hash == NULL)
        goto failure;
    for (i = 0; i < edge_count; ++i)
    {
        size_t slot = (size_t)jcv_pack_edge_hash(edges[i].vertices[0], edges[i].vertices[1]) & (hash_capacity - 1);
        while (edge_hash[slot] != 0)
            slot = (slot + 1) & (hash_capacity - 1);
        edge_hash[slot] = (uint32_t)i + 1;
    }

    {
        uint32_t cell_edge_index = 0;
        for (i = 0; i < num_points; ++i)
        {
            int site_index = input_to_site[i];
            cell_offsets[i] = cell_edge_index;
            if (site_index < 0)
                continue;
            jcv_site_get_edges(&diagram, &sites[site_index], &iter);
            while (jcv_edge_next(&iter, &edge))
            {
                int edge_index = jcv_pack_find_edge(edges, edge_hash, hash_capacity,
                                                    edge.vertices[0], edge.vertices[1]);
                uint32_t flags = 0;
                if (edge_index < 0 || (uint32_t)edge_index > (UINT32_MAX >> JCV_PACK_CELL_EDGE_SHIFT))
                    goto failure;
                if (edge.sites[0] != NULL && edges[edge_index].sites[0] >= 0 &&
                    edge.sites[0]->index != (uint32_t)edges[edge_index].sites[0])
                    flags |= JCV_PACK_CELL_SITE_FLIP;
                if (edge.vertices[0] != edges[edge_index].vertices[0])
                    flags |= JCV_PACK_CELL_POSITION_FLIP;
                cell_refs[cell_edge_index++] =
                    ((uint32_t)edge_index << JCV_PACK_CELL_EDGE_SHIFT) | flags;
            }
        }
        cell_offsets[num_points] = cell_edge_index;
        header[7] = cell_edge_index;
    }

    free(edge_hash);
    jcv_diagram_free(&diagram);
    return block;

failure:
    free(edge_hash);
    free(block);
    if (diagram.internal != NULL)
        jcv_diagram_free(&diagram);
    return NULL;
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
                          float min_x,
                          float min_y,
                          float max_x,
                          float max_y,
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
    if (num_points < 0 || max_x <= min_x || max_y <= min_y ||
        (num_points > 0 && xy == NULL))
        return NULL;
    if (num_points == 0)
    {
        *output_count = 0;
        return NULL;
    }

    rect.min.x = min_x;
    rect.min.y = min_y;
    rect.max.x = max_x;
    rect.max.y = max_y;
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
