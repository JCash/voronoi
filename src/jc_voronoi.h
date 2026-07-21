// Copyright (c) 2015-2026 Mathias Westerdahl
// For LICENSE (MIT), USAGE or HISTORY, see bottom of file

#ifndef JC_VORONOI_H
#define JC_VORONOI_H

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <string.h> // memset

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JCV_REAL_TYPE
    #define JCV_REAL_TYPE float
#endif

#ifndef JCV_REAL_TYPE_EPSILON
    #define JCV_REAL_TYPE_EPSILON FLT_EPSILON
#endif

#ifndef JCV_ATAN2
    #define JCV_ATAN2(_Y_, _X_) atan2f(_Y_, _X_)
#endif

#ifndef JCV_SQRT
    #define JCV_SQRT(_X_)       sqrtf(_X_)
#endif

#ifndef JCV_PI
    #define JCV_PI 3.14159265358979323846264338327950288f
#endif

#ifndef JCV_FLT_MAX
    #define JCV_FLT_MAX 3.402823466e+38F
#endif

#ifndef JCV_EDGE_INTERSECT_THRESHOLD
    // Fix for Issue #40
    #define JCV_EDGE_INTERSECT_THRESHOLD 1.0e-10F
#endif

typedef JCV_REAL_TYPE jcv_real;

typedef struct jcv_point_           jcv_point;
typedef struct jcv_rect_            jcv_rect;
typedef struct jcv_site_            jcv_site;
typedef struct jcv_edge_            jcv_edge;
typedef struct jcv_edge_iter_       jcv_edge_iter;
typedef struct jcv_delauney_edge_   jcv_delauney_edge;
typedef struct jcv_delauney_iter_   jcv_delauney_iter;
typedef struct jcv_diagram_         jcv_diagram;
typedef struct jcv_clipper_         jcv_clipper;
typedef struct jcv_context_internal_ jcv_context_internal;

/// Tests if a point is inside the final shape
typedef int (*jcv_clip_test_point_fn)(const jcv_clipper* clipper, const jcv_point p);
/** Given an edge, and the clipper, calculates the e->pos[0] and e->pos[1]
 * Returns 0 if not successful
 */
typedef int (*jcv_clip_edge_fn)(const jcv_clipper* clipper, jcv_edge* e);
/** Given the clipper, the site and the last edge,
 * closes any gaps in the polygon by adding new edges that follow the bounding shape
 * The internal context is use when allocating new edges.
 */
typedef void (*jcv_clip_fillgap_fn)(const jcv_clipper* clipper, jcv_context_internal* allocator, jcv_site* s);



/**
 * Uses malloc
 * If a clipper is not supplied, a default box clipper will be used
 * If rect is null, an automatic bounding box is calculated, with an extra padding of 10 units
 * All points will be culled against the bounding rect, and all edges will be clipped against it.
 */
extern void jcv_diagram_generate( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* diagram );

typedef void* (*FJCVAllocFn)(void* userctx, size_t size);
typedef void (*FJCVFreeFn)(void* userctx, void* p);

// Same as above, but allows the client to use a custom allocator
extern void jcv_diagram_generate_useralloc( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram );

// Uses free (or the registered custom free function)
extern void jcv_diagram_free( jcv_diagram* diagram );

// Returns an array of sites, where each index is the same as the original input point array.
extern const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram );

// Returns the number of unique vertices in the diagram.
extern int jcv_get_num_vertices( const jcv_diagram* diagram );

// Writes all unique vertices to a client-owned array of diagram->numvertices points.
extern void jcv_diagram_get_vertices( const jcv_diagram* diagram, jcv_point* vertices );

// Creates an iterator over every edge in the diagram.
extern void jcv_diagram_get_edges( const jcv_diagram* diagram, jcv_edge_iter* iter );

// Creates an iterator over one site's edges, in counter-clockwise order.
// Returned edges are oriented for the site: sites[0] is always site and the
// edge runs counter-clockwise around it.
extern void jcv_site_get_edges( const jcv_diagram* diagram, const jcv_site* site, jcv_edge_iter* iter );

// Writes the next edge to client-owned storage. Returns 0 at the end.
extern int jcv_edge_next( jcv_edge_iter* iter, jcv_edge* edge );

// Creates an iterator over the delauney edges of a voronoi diagram
void jcv_delauney_begin( const jcv_diagram* diagram, jcv_delauney_iter* iter );

// Steps the iterator and returns the next edge
// Returns 0 when there are no more edges
int jcv_delauney_next( jcv_delauney_iter* iter, jcv_delauney_edge* next );

// For the default clipper
extern int jcv_boxshape_test(const jcv_clipper* clipper, const jcv_point p);
extern int jcv_boxshape_clip(const jcv_clipper* clipper, jcv_edge* e);
extern void jcv_boxshape_fillgaps(const jcv_clipper* clipper, jcv_context_internal* allocator, jcv_site* s);


struct jcv_point_
{
    jcv_real x;
    jcv_real y;
};

struct jcv_site_
{
    jcv_point       p;
    int             index;  // Index into the original list of points
};

// The coefficients a, b and c are from the general line equation: ax * by + c = 0
struct jcv_edge_
{
    jcv_site*           sites[2];
    jcv_point           pos[2];
    int                 vertices[2]; // Unique endpoint indices, indexed like pos
    jcv_real            a;
    jcv_real            b;
    jcv_real            c;
};

struct jcv_edge_iter_
{
    const void*         current;
    const void*         end;
    const jcv_site*     site;
};

struct jcv_delauney_iter_
{
    jcv_edge_iter       edges;
};

struct jcv_delauney_edge_
{
    jcv_edge        edge;       // The voronoi edge separating the two sites
    const jcv_site* sites[2];
    jcv_point       pos[2];     // the positions of the two sites
};

struct jcv_rect_
{
    jcv_point   min;
    jcv_point   max;
};

struct jcv_clipper_
{
    jcv_clip_test_point_fn  test_fn;
    jcv_clip_edge_fn        clip_fn;
    jcv_clip_fillgap_fn     fill_fn;
    jcv_point               min;        // The bounding rect min
    jcv_point               max;        // The bounding rect max
    void*                   ctx;        // User defined context
};

struct jcv_diagram_
{
    jcv_context_internal*   internal;
    int                     numsites;
    int                     numvertices;
    jcv_point               min;
    jcv_point               max;
};

#ifdef __cplusplus
}
#endif

#endif // JC_VORONOI_H

#ifdef JC_VORONOI_IMPLEMENTATION
#undef JC_VORONOI_IMPLEMENTATION

#include <memory.h>

// INTERNAL FUNCTIONS

#if defined(_MSC_VER) && !defined(__cplusplus)
    #define inline __inline
#endif

static const int JCV_DIRECTION_LEFT  = 0;
static const int JCV_DIRECTION_RIGHT = 1;
static const jcv_real JCV_INVALID_VALUE = (jcv_real)-JCV_FLT_MAX;
static const int JCV_INVALID_VERTEX = -1;

// jcv_real

static inline jcv_real jcv_abs(jcv_real v) {
    return (v < 0) ? -v : v;
}

static inline int jcv_real_eq(jcv_real a, jcv_real b)
{
    return jcv_abs(a - b) < JCV_REAL_TYPE_EPSILON;
}

static inline jcv_real jcv_real_to_int(jcv_real v) {
    return (sizeof(jcv_real) == 4) ? (jcv_real)(int)v : (jcv_real)(long long)v;
}

// Only used for calculating the initial bounding box
static inline jcv_real jcv_floor(jcv_real v) {
    jcv_real i = jcv_real_to_int(v);
    return (v < i) ? i - 1 : i;
}

// Only used for calculating the initial bounding box
static inline jcv_real jcv_ceil(jcv_real v) {
    jcv_real i = jcv_real_to_int(v);
    return (v > i) ? i + 1 : i;
}

static inline jcv_real jcv_min(jcv_real a, jcv_real b) {
    return a < b ? a : b;
}

static inline jcv_real jcv_max(jcv_real a, jcv_real b) {
    return a > b ? a : b;
}

// jcv_point

static inline int jcv_point_cmp(const void* p1, const void* p2)
{
    const jcv_point* s1 = (const jcv_point*) p1;
    const jcv_point* s2 = (const jcv_point*) p2;
    return (s1->y != s2->y) ? (s1->y < s2->y ? -1 : 1) : (s1->x < s2->x ? -1 : 1);
}

static inline int jcv_point_less( const jcv_point* pt1, const jcv_point* pt2 )
{
    return (pt1->y == pt2->y) ? (pt1->x < pt2->x) : pt1->y < pt2->y;
}

static inline int jcv_point_eq( const jcv_point* pt1, const jcv_point* pt2 )
{
    return jcv_real_eq(pt1->y, pt2->y) && jcv_real_eq(pt1->x, pt2->x);
}

static inline int jcv_point_on_box_edge( const jcv_point* pt, const jcv_point* min, const jcv_point* max )
{
    return pt->x == min->x || pt->y == min->y || pt->x == max->x || pt->y == max->y;
}

// corners

static const int JCV_EDGE_LEFT    = 1;
static const int JCV_EDGE_RIGHT   = 2;
static const int JCV_EDGE_BOTTOM  = 4;
static const int JCV_EDGE_TOP     = 8;

static const int JCV_CORNER_NONE          = 0;
static const int JCV_CORNER_TOP_LEFT      = 1;
static const int JCV_CORNER_BOTTOM_LEFT   = 2;
static const int JCV_CORNER_BOTTOM_RIGHT  = 3;
static const int JCV_CORNER_TOP_RIGHT     = 4;

static inline int jcv_get_edge_flags( const jcv_point* pt, const jcv_point* min, const jcv_point* max )
{
    int flags = 0;
    if      (pt->x == min->x)   flags |= JCV_EDGE_LEFT;
    else if (pt->x == max->x)   flags |= JCV_EDGE_RIGHT;
    if      (pt->y == min->y)   flags |= JCV_EDGE_BOTTOM;
    else if (pt->y == max->y)   flags |= JCV_EDGE_TOP;
    return flags;
}

static inline int jcv_edge_flags_to_corner(int edge_flags)
{
    #define TEST_FLAGS(_FLAGS, _RETVAL) if ( (_FLAGS) == edge_flags ) return _RETVAL
    TEST_FLAGS(JCV_EDGE_TOP|JCV_EDGE_LEFT, JCV_CORNER_TOP_LEFT);
    TEST_FLAGS(JCV_EDGE_TOP|JCV_EDGE_RIGHT, JCV_CORNER_TOP_RIGHT);
    TEST_FLAGS(JCV_EDGE_BOTTOM|JCV_EDGE_LEFT, JCV_CORNER_BOTTOM_LEFT);
    TEST_FLAGS(JCV_EDGE_BOTTOM|JCV_EDGE_RIGHT, JCV_CORNER_BOTTOM_RIGHT);
    #undef TEST_FLAGS
    return 0;
}

static inline int jcv_is_corner(int corner)
{
    return corner != 0;
}

static inline int jcv_corner_rotate_90(int corner)
{
    corner--;
    corner = (corner+1)%4;
    return corner + 1;
}
static inline jcv_point jcv_corner_to_point(int corner, const jcv_point* min, const jcv_point* max )
{
    jcv_point p;
    if      (corner == JCV_CORNER_TOP_LEFT)     { p.x = min->x; p.y = max->y; }
    else if (corner == JCV_CORNER_TOP_RIGHT)    { p.x = max->x; p.y = max->y; }
    else if (corner == JCV_CORNER_BOTTOM_LEFT)  { p.x = min->x; p.y = min->y; }
    else if (corner == JCV_CORNER_BOTTOM_RIGHT) { p.x = max->x; p.y = min->y; }
    else                                        { p.x = JCV_INVALID_VALUE; p.y = JCV_INVALID_VALUE; }
    return p;
}

static inline jcv_real jcv_point_dist_sq( const jcv_point* pt1, const jcv_point* pt2)
{
    jcv_real diffx = pt1->x - pt2->x;
    jcv_real diffy = pt1->y - pt2->y;
    return diffx * diffx + diffy * diffy;
}

static inline jcv_real jcv_point_dist( const jcv_point* pt1, const jcv_point* pt2 )
{
    return (jcv_real)(JCV_SQRT(jcv_point_dist_sq(pt1, pt2)));
}

// Structs

typedef struct jcv_edge_internal_
{
    jcv_site*                   sites[2];
    jcv_point                   pos[2];
    int                         vertices[2];
    jcv_real                    a;
    jcv_real                    b;
    jcv_real                    c;
    struct jcv_edge_internal_*  next;
} jcv_edge_internal;

// Construction-only site incidence. These are allocated from temporary
// blocks and released before diagram generation returns.
typedef struct jcv_graphedge_
{
    struct jcv_graphedge_*  next;
    jcv_edge_internal*      edge;
    jcv_real                angle;
    unsigned char           site_index;
    unsigned char           flip;
} jcv_graphedge;

typedef struct jcv_halfedge_
{
    jcv_edge_internal*      edge;
    struct jcv_halfedge_*   left;
    struct jcv_halfedge_*   right;
    struct jcv_halfedge_*   rb_parent;
    struct jcv_halfedge_*   rb_left;
    struct jcv_halfedge_*   rb_right;
    jcv_point               vertex;
    jcv_real                y;
    int                     direction; // 0=left, 1=right
    int                     pqpos;
    uint8_t                 rb_red;
} jcv_halfedge;

typedef struct jcv_memoryblock_
{
    size_t sizefree;
    struct jcv_memoryblock_* next;
    char*  memory;
} jcv_memoryblock;


typedef int  (*FJCVPriorityQueuePrint)(const void* node, int pos);
typedef int  (*FJCVPriorityQueueCompare)(const void* a, const void* b);
typedef void (*FJCVPriorityQueueSetPos)(void* node, int pos);
typedef int  (*FJCVPriorityQueueGetPos)(const void* node);

typedef struct jcv_priorityqueue_
{
    // Implements a binary heap
    int                         maxnumitems;
    int                         numitems;
    void**                      items;
    FJCVPriorityQueueCompare    compare_fn;
    FJCVPriorityQueueSetPos     set_pos;
    FJCVPriorityQueueGetPos     get_pos;
} jcv_priorityqueue;

struct jcv_context_internal_
{
    void*               mem;
    jcv_edge_internal*  edges;
    jcv_halfedge*       beachline_start;
    jcv_halfedge*       beachline_end;
    jcv_halfedge*       beachline_root;
    jcv_halfedge        beachline_nil;
    jcv_priorityqueue*  eventqueue;

    jcv_site*           sites;
    jcv_edge_internal** site_edge_refs;
    int*                site_edge_offsets;
    jcv_graphedge**     build_site_edges;
    int*                build_site_counts;
    jcv_site*           bottomsite;
    int                 numsites;
    int                 currentsite;
    int                 numvertices;

    jcv_memoryblock*    memblocks;
    jcv_memoryblock*    tempmemblocks;
    jcv_halfedge*       halfedgepool;
    void**              eventmem;
    jcv_clipper         clipper;

    void*               memctx; // Given by the user
    FJCVAllocFn         alloc;
    FJCVFreeFn          free;

    jcv_rect            rect;
};

void jcv_diagram_free( jcv_diagram* d )
{
    jcv_context_internal* internal = d->internal;
    void* memctx = internal->memctx;
    FJCVFreeFn freefn = internal->free;
    while(internal->memblocks)
    {
        jcv_memoryblock* p = internal->memblocks;
        internal->memblocks = internal->memblocks->next;
        freefn( memctx, p );
    }

    freefn( memctx, internal->mem );
}

const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram )
{
    return diagram->internal->sites;
}

int jcv_get_num_vertices( const jcv_diagram* diagram )
{
    return diagram->numvertices;
}

void jcv_diagram_get_vertices( const jcv_diagram* diagram, jcv_point* vertices )
{
    jcv_edge_iter iter;
    jcv_edge edge;
    jcv_diagram_get_edges(diagram, &iter);
    while( jcv_edge_next(&iter, &edge) )
    {
        vertices[edge.vertices[0]] = edge.pos[0];
        vertices[edge.vertices[1]] = edge.pos[1];
    }
}

static void jcv_edge_copy(const jcv_edge_internal* source, jcv_edge* target)
{
    target->sites[0] = source->sites[0];
    target->sites[1] = source->sites[1];
    target->pos[0] = source->pos[0];
    target->pos[1] = source->pos[1];
    target->vertices[0] = source->vertices[0];
    target->vertices[1] = source->vertices[1];
    target->a = source->a;
    target->b = source->b;
    target->c = source->c;
}

void jcv_diagram_get_edges( const jcv_diagram* diagram, jcv_edge_iter* iter )
{
    iter->current = diagram->internal->edges;
    iter->end = 0;
    iter->site = 0;
}

void jcv_site_get_edges( const jcv_diagram* diagram, const jcv_site* site, jcv_edge_iter* iter )
{
    int index = (int)(site - diagram->internal->sites);
    if( index >= 0 && index < diagram->numsites )
    {
        iter->current = diagram->internal->site_edge_refs + diagram->internal->site_edge_offsets[index];
        iter->end = diagram->internal->site_edge_refs + diagram->internal->site_edge_offsets[index+1];
    }
    else
    {
        iter->current = 0;
        iter->end = 0;
    }
    iter->site = site;
}

int jcv_edge_next( jcv_edge_iter* iter, jcv_edge* edge )
{
    if( !iter->site )
    {
        const jcv_edge_internal* source = (const jcv_edge_internal*)iter->current;
        while( source && jcv_point_eq(&source->pos[0], &source->pos[1]) )
            source = source->next;
        if( !source )
            return 0;
        iter->current = source->next;
        jcv_edge_copy(source, edge);
        return 1;
    }

    jcv_edge_internal* const* current = (jcv_edge_internal* const*)iter->current;
    if( !current || current == (jcv_edge_internal* const*)iter->end )
        return 0;
    const jcv_edge_internal* source = *current;
    iter->current = current + 1;
    int site_index = source->sites[0] == iter->site ? 0 : 1;
    if( source->sites[1] == 0 )
    {
        jcv_edge_copy(source, edge);
        return 1;
    }
    int flip = ((source->pos[0].x - source->sites[0]->p.x) * (source->pos[1].y - source->sites[0]->p.y) -
                (source->pos[0].y - source->sites[0]->p.y) * (source->pos[1].x - source->sites[0]->p.x)) > (jcv_real)0 ? 0 : 1;
    edge->sites[0] = source->sites[site_index];
    edge->sites[1] = source->sites[1-site_index];
    edge->pos[flip] = source->pos[site_index];
    edge->pos[1-flip] = source->pos[1-site_index];
    edge->vertices[flip] = source->vertices[site_index];
    edge->vertices[1-flip] = source->vertices[1-site_index];
    edge->a = source->a;
    edge->b = source->b;
    edge->c = source->c;
    return 1;
}

void jcv_delauney_begin( const jcv_diagram* diagram, jcv_delauney_iter* iter )
{
    jcv_diagram_get_edges(diagram, &iter->edges);
}

int jcv_delauney_next( jcv_delauney_iter* iter, jcv_delauney_edge* next )
{
    do {
        if( !jcv_edge_next(&iter->edges, &next->edge) )
            return 0;
    } while( next->edge.sites[0] == 0 || next->edge.sites[1] == 0 );

    next->sites[0] = next->edge.sites[0];
    next->sites[1] = next->edge.sites[1];
    next->pos[0] = next->sites[0]->p;
    next->pos[1] = next->sites[1]->p;
    return 1;
}

static inline void* jcv_align(void* value, size_t alignment)
{
    return (void*) (((uintptr_t) value + (alignment-1)) & ~(alignment-1));
}

static void* jcv_alloc(jcv_context_internal* internal, size_t size)
{
    if( !internal->memblocks || internal->memblocks->sizefree < (size+sizeof(void*)) )
    {
        size_t blocksize = 16 * 1024;
        size_t required = sizeof(jcv_memoryblock) + size + sizeof(void*);
        if( blocksize < required )
            blocksize = required;
        jcv_memoryblock* block = (jcv_memoryblock*)internal->alloc( internal->memctx, blocksize );
        size_t offset = sizeof(jcv_memoryblock);
        block->sizefree = blocksize - offset;
        block->next = internal->memblocks;
        block->memory = ((char*)block) + offset;
        internal->memblocks = block;
    }
    void* p_raw = internal->memblocks->memory;
    void* p_aligned = jcv_align(p_raw, sizeof(void*));
    size += (uintptr_t)p_aligned - (uintptr_t)p_raw;
    internal->memblocks->memory += size;
    internal->memblocks->sizefree -= size;
    return p_aligned;
}

static jcv_edge_internal* jcv_alloc_edge(jcv_context_internal* internal)
{
    return (jcv_edge_internal*)jcv_alloc(internal, sizeof(jcv_edge_internal));
}

static jcv_halfedge* jcv_alloc_halfedge(jcv_context_internal* internal)
{
    if( internal->halfedgepool )
    {
        jcv_halfedge* edge = internal->halfedgepool;
        internal->halfedgepool = internal->halfedgepool->right;
        return edge;
    }

    return (jcv_halfedge*)jcv_alloc(internal, sizeof(jcv_halfedge));
}

static void* jcv_temp_alloc(jcv_context_internal* internal, size_t size)
{
    if( !internal->tempmemblocks || internal->tempmemblocks->sizefree < (size+sizeof(void*)) )
    {
        size_t blocksize = 16 * 1024;
        size_t required = sizeof(jcv_memoryblock) + size + sizeof(void*);
        if( blocksize < required )
            blocksize = required;
        jcv_memoryblock* block = (jcv_memoryblock*)internal->alloc(internal->memctx, blocksize);
        size_t offset = sizeof(jcv_memoryblock);
        block->sizefree = blocksize - offset;
        block->next = internal->tempmemblocks;
        block->memory = ((char*)block) + offset;
        internal->tempmemblocks = block;
    }
    void* p_raw = internal->tempmemblocks->memory;
    void* p_aligned = jcv_align(p_raw, sizeof(void*));
    size += (uintptr_t)p_aligned - (uintptr_t)p_raw;
    internal->tempmemblocks->memory += size;
    internal->tempmemblocks->sizefree -= size;
    return p_aligned;
}

static jcv_graphedge* jcv_alloc_graphedge(jcv_context_internal* internal)
{
    return (jcv_graphedge*)jcv_temp_alloc(internal, sizeof(jcv_graphedge));
}

static void jcv_temp_free_all(jcv_context_internal* internal)
{
    FJCVFreeFn freefn = internal->free;
    while( internal->tempmemblocks )
    {
        jcv_memoryblock* block = internal->tempmemblocks;
        internal->tempmemblocks = block->next;
        freefn(internal->memctx, block);
    }
}

static void* jcv_alloc_fn(void* memctx, size_t size)
{
    (void)memctx;
    return malloc(size);
}

static void jcv_free_fn(void* memctx, void* p)
{
    (void)memctx;
    free(p);
}

// jcv_edge

static inline int jcv_is_valid(const jcv_point* p)
{
    return (p->x != JCV_INVALID_VALUE || p->y != JCV_INVALID_VALUE) ? 1 : 0;
}

static void jcv_edge_create(jcv_edge_internal* e, jcv_site* s1, jcv_site* s2)
{
    e->next = 0;
    e->sites[0] = s1;
    e->sites[1] = s2;
    e->pos[0].x = JCV_INVALID_VALUE;
    e->pos[0].y = JCV_INVALID_VALUE;
    e->pos[1].x = JCV_INVALID_VALUE;
    e->pos[1].y = JCV_INVALID_VALUE;
    e->vertices[0] = JCV_INVALID_VERTEX;
    e->vertices[1] = JCV_INVALID_VERTEX;

    // Create line equation between S1 and S2:
    // jcv_real a = -1 * (s2->p.y - s1->p.y);
    // jcv_real b = s2->p.x - s1->p.x;
    // //jcv_real c = -1 * (s2->p.x - s1->p.x) * s1->p.y + (s2->p.y - s1->p.y) * s1->p.x;
    //
    // // create perpendicular line
    // jcv_real pa = b;
    // jcv_real pb = -a;
    // //jcv_real pc = pa * s1->p.x + pb * s1->p.y;
    //
    // // Move to the mid point
    // jcv_real mx = s1->p.x + dx * jcv_real(0.5);
    // jcv_real my = s1->p.y + dy * jcv_real(0.5);
    // jcv_real pc = ( pa * mx + pb * my );

    jcv_real dx = s2->p.x - s1->p.x;
    jcv_real dy = s2->p.y - s1->p.y;
    int dx_is_larger = (dx*dx) > (dy*dy); // instead of fabs

    // Simplify it, using dx and dy
    e->c = dx * (s1->p.x + dx * (jcv_real)0.5) + dy * (s1->p.y + dy * (jcv_real)0.5);

    if( dx_is_larger )
    {
        e->a = (jcv_real)1;
        e->b = dy / dx;
        e->c /= dx;
    }
    else
    {
        e->a = dx / dy;
        e->b = (jcv_real)1;
        e->c /= dy;
    }
}

// CLIPPING
int jcv_boxshape_test(const jcv_clipper* clipper, const jcv_point p)
{
    return p.x >= clipper->min.x && p.x <= clipper->max.x &&
           p.y >= clipper->min.y && p.y <= clipper->max.y;
}

// The line equation: ax + by + c = 0
// see jcv_edge_create
int jcv_boxshape_clip(const jcv_clipper* clipper, jcv_edge* e)
{
    jcv_real pxmin = clipper->min.x;
    jcv_real pxmax = clipper->max.x;
    jcv_real pymin = clipper->min.y;
    jcv_real pymax = clipper->max.y;

    jcv_real x1, y1, x2, y2;
    jcv_point* s1;
    jcv_point* s2;
    if (e->a == (jcv_real)1 && e->b >= (jcv_real)0)
    {
        s1 = jcv_is_valid(&e->pos[1]) ? &e->pos[1] : 0;
        s2 = jcv_is_valid(&e->pos[0]) ? &e->pos[0] : 0;
    }
    else
    {
        s1 = jcv_is_valid(&e->pos[0]) ? &e->pos[0] : 0;
        s2 = jcv_is_valid(&e->pos[1]) ? &e->pos[1] : 0;
    }
    int s1_inside = s1 != 0 && jcv_boxshape_test(clipper, *s1);
    int s2_inside = s2 != 0 && jcv_boxshape_test(clipper, *s2);

    if (e->a == (jcv_real)1) // delta x is larger
    {
        y1 = pymin;
        if (s1 != 0 && s1->y > pymin)
        {
            y1 = s1->y;
        }
        if( y1 > pymax )
        {
            y1 = pymax;
        }
        x1 = e->c - e->b * y1;
        y2 = pymax;
        if (s2 != 0 && s2->y < pymax)
            y2 = s2->y;

        if( y2 < pymin )
        {
            y2 = pymin;
        }
        x2 = (e->c) - (e->b) * y2;
        // Never occurs according to lcov
        // if( ((x1 > pxmax) & (x2 > pxmax)) | ((x1 < pxmin) & (x2 < pxmin)) )
        // {
        //     return 0;
        // }
        if (x1 > pxmax)
        {
            x1 = pxmax;
            y1 = (e->c - x1) / e->b;
        }
        else if (x1 < pxmin)
        {
            x1 = pxmin;
            y1 = (e->c - x1) / e->b;
        }
        if (x2 > pxmax)
        {
            x2 = pxmax;
            y2 = (e->c - x2) / e->b;
        }
        else if (x2 < pxmin)
        {
            x2 = pxmin;
            y2 = (e->c - x2) / e->b;
        }
    }
    else // delta y is larger
    {
        x1 = pxmin;
        if( s1 != 0 && s1->x > pxmin )
            x1 = s1->x;
        if( x1 > pxmax )
        {
            x1 = pxmax;
        }
        y1 = e->c - e->a * x1;
        x2 = pxmax;
        if( s2 != 0 && s2->x < pxmax )
            x2 = s2->x;
        if( x2 < pxmin )
        {
            x2 = pxmin;
        }
        y2 = e->c - e->a * x2;
        // Never occurs according to lcov
        // if( ((y1 > pymax) & (y2 > pymax)) | ((y1 < pymin) & (y2 < pymin)) )
        // {
        //     return 0;
        // }
        if( y1 > pymax )
        {
            y1 = pymax;
            x1 = (e->c - y1) / e->a;
        }
        else if( y1 < pymin )
        {
            y1 = pymin;
            x1 = (e->c - y1) / e->a;
        }
        if( y2 > pymax )
        {
            y2 = pymax;
            x2 = (e->c - y2) / e->a;
        }
        else if( y2 < pymin )
        {
            y2 = pymin;
            x2 = (e->c - y2) / e->a;
        }
    }

    // Circle events assign the same vertex to all incident edges. Preserve
    // that exact point when it is already inside the clipping box instead of
    // projecting it independently onto each edge's rounded line equation.
    if( s1_inside )
    {
        x1 = s1->x;
        y1 = s1->y;
    }
    if( s2_inside )
    {
        x2 = s2->x;
        y2 = s2->y;
    }

    e->pos[0].x = x1;
    e->pos[0].y = y1;
    e->pos[1].x = x2;
    e->pos[1].y = y2;

    // If the two points are equal, the result is invalid
    return (x1 == x2 && y1 == y2) ? 0 : 1;
}

// The line equation: ax + by + c = 0
// see jcv_edge_create
static int jcv_edge_clipline(jcv_context_internal* internal, jcv_edge_internal* e)
{
    jcv_point previous_pos[2] = {e->pos[0], e->pos[1]};
    int previous_vertices[2] = {e->vertices[0], e->vertices[1]};
    jcv_edge clipped;
    jcv_edge_copy(e, &clipped);
    if( !internal->clipper.clip_fn(&internal->clipper, &clipped) )
        return 0;
    e->pos[0] = clipped.pos[0];
    e->pos[1] = clipped.pos[1];

    for( int i = 0; i < 2; ++i )
    {
        e->vertices[i] = JCV_INVALID_VERTEX;
        for( int j = 0; j < 2; ++j )
        {
            // Clipping may retain or reorder an existing endpoint. Only carry
            // its identity across when the point itself was copied exactly.
            if( previous_vertices[j] >= 0 &&
                e->pos[i].x == previous_pos[j].x && e->pos[i].y == previous_pos[j].y )
            {
                e->vertices[i] = previous_vertices[j];
                break;
            }
        }
        if( e->vertices[i] < 0 )
            e->vertices[i] = internal->numvertices++;
    }
    return 1;
}

static jcv_edge_internal* jcv_edge_new(jcv_context_internal* internal, jcv_site* s1, jcv_site* s2)
{
    jcv_edge_internal* e = jcv_alloc_edge(internal);
    jcv_edge_create(e, s1, s2);
    return e;
}


// jcv_halfedge

static void jcv_halfedge_link(jcv_halfedge* edge, jcv_halfedge* newedge)
{
    newedge->left = edge;
    newedge->right = edge->right;
    edge->right->left = newedge;
    edge->right = newedge;
}

static inline void jcv_halfedge_unlink(jcv_halfedge* he)
{
    he->left->right = he->right;
    he->right->left = he->left;
    he->left  = 0;
    he->right = 0;
}

static inline jcv_halfedge* jcv_halfedge_new(jcv_context_internal* internal, jcv_edge_internal* e, int direction)
{
    jcv_halfedge* he = jcv_alloc_halfedge(internal);
    he->edge        = e;
    he->left        = 0;
    he->right       = 0;
    he->rb_parent   = &internal->beachline_nil;
    he->rb_left     = &internal->beachline_nil;
    he->rb_right    = &internal->beachline_nil;
    he->direction   = direction;
    he->pqpos       = 0;
    he->rb_red      = 0;
    // These are set outside
    //he->y
    //he->vertex
    return he;
}

static void jcv_halfedge_delete(jcv_context_internal* internal, jcv_halfedge* he)
{
    he->right = internal->halfedgepool;
    internal->halfedgepool = he;
}

static inline jcv_site* jcv_halfedge_leftsite(const jcv_halfedge* he)
{
    return he->edge->sites[he->direction];
}

static inline jcv_site* jcv_halfedge_rightsite(const jcv_halfedge* he)
{
    return he->edge ? he->edge->sites[1 - he->direction] : 0;
}

static int jcv_halfedge_rightof(const jcv_halfedge* he, const jcv_point* p)
{
    const jcv_edge_internal* e = he->edge;
    const jcv_site* topsite = e->sites[1];

    int right_of_site = (p->x > topsite->p.x) ? 1 : 0;
    if (right_of_site && he->direction == JCV_DIRECTION_LEFT)
        return 1;
    if (!right_of_site && he->direction == JCV_DIRECTION_RIGHT)
        return 0;

    jcv_real dxp, dyp, dxs, t1, t2, t3, yl;

    int above;
    if (e->a == (jcv_real)1)
    {
        dyp = p->y - topsite->p.y;
        dxp = p->x - topsite->p.x;
        int fast = 0;
        if( (!right_of_site & (e->b < (jcv_real)0)) | (right_of_site & (e->b >= (jcv_real)0)) )
        {
            above = dyp >= e->b * dxp;
            fast = above;
        }
        else
        {
            above = (p->x + p->y * e->b) > e->c;
            if (e->b < (jcv_real)0)
                above = !above;
            if (!above)
                fast = 1;
        }
        if (!fast)
        {
            dxs = topsite->p.x - e->sites[0]->p.x;
            above = e->b * (dxp * dxp - dyp * dyp)
                    < dxs * dyp * ((jcv_real)1 + (jcv_real)2 * dxp / dxs + e->b * e->b);
            if (e->b < (jcv_real)0)
                above = !above;
        }
    }
    else // e->b == 1
    {
        yl = e->c - e->a * p->x;
        t1 = p->y - yl;
        t2 = p->x - topsite->p.x;
        t3 = yl - topsite->p.y;
        above = t1 * t1 > (t2 * t2 + t3 * t3);
    }
    return (he->direction == JCV_DIRECTION_LEFT ? above : !above);
}

// The linked beachline provides constant-time neighboring halfedges. The
// red-black tree indexes that same order so the predecessor at a site event can
// be found in logarithmic time.
static void jcv_beachline_init(jcv_context_internal* internal)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    memset(nil, 0, sizeof(*nil));
    nil->rb_parent = nil;
    nil->rb_left = nil;
    nil->rb_right = nil;
    nil->rb_red = 0;
    internal->beachline_root = nil;
}

static void jcv_rb_rotate_left(jcv_context_internal* internal, jcv_halfedge* node)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    jcv_halfedge* child = node->rb_right;
    node->rb_right = child->rb_left;
    if (child->rb_left != nil)
        child->rb_left->rb_parent = node;
    child->rb_parent = node->rb_parent;
    if (node->rb_parent == nil)
        internal->beachline_root = child;
    else if (node == node->rb_parent->rb_left)
        node->rb_parent->rb_left = child;
    else
        node->rb_parent->rb_right = child;
    child->rb_left = node;
    node->rb_parent = child;
}

static void jcv_rb_rotate_right(jcv_context_internal* internal, jcv_halfedge* node)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    jcv_halfedge* child = node->rb_left;
    node->rb_left = child->rb_right;
    if (child->rb_right != nil)
        child->rb_right->rb_parent = node;
    child->rb_parent = node->rb_parent;
    if (node->rb_parent == nil)
        internal->beachline_root = child;
    else if (node == node->rb_parent->rb_right)
        node->rb_parent->rb_right = child;
    else
        node->rb_parent->rb_left = child;
    child->rb_right = node;
    node->rb_parent = child;
}

static void jcv_rb_insert_fixup(jcv_context_internal* internal, jcv_halfedge* node)
{
    while (node->rb_parent->rb_red)
    {
        if (node->rb_parent == node->rb_parent->rb_parent->rb_left)
        {
            jcv_halfedge* uncle = node->rb_parent->rb_parent->rb_right;
            if (uncle->rb_red)
            {
                node->rb_parent->rb_red = 0;
                uncle->rb_red = 0;
                node->rb_parent->rb_parent->rb_red = 1;
                node = node->rb_parent->rb_parent;
            }
            else
            {
                if (node == node->rb_parent->rb_right)
                {
                    node = node->rb_parent;
                    jcv_rb_rotate_left(internal, node);
                }
                node->rb_parent->rb_red = 0;
                node->rb_parent->rb_parent->rb_red = 1;
                jcv_rb_rotate_right(internal, node->rb_parent->rb_parent);
            }
        }
        else
        {
            jcv_halfedge* uncle = node->rb_parent->rb_parent->rb_left;
            if (uncle->rb_red)
            {
                node->rb_parent->rb_red = 0;
                uncle->rb_red = 0;
                node->rb_parent->rb_parent->rb_red = 1;
                node = node->rb_parent->rb_parent;
            }
            else
            {
                if (node == node->rb_parent->rb_left)
                {
                    node = node->rb_parent;
                    jcv_rb_rotate_right(internal, node);
                }
                node->rb_parent->rb_red = 0;
                node->rb_parent->rb_parent->rb_red = 1;
                jcv_rb_rotate_left(internal, node->rb_parent->rb_parent);
            }
        }
    }
    internal->beachline_root->rb_red = 0;
}

static jcv_halfedge* jcv_rb_minimum(jcv_context_internal* internal, jcv_halfedge* node)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    while (node->rb_left != nil)
        node = node->rb_left;
    return node;
}

static void jcv_rb_transplant(jcv_context_internal* internal, jcv_halfedge* oldnode, jcv_halfedge* newnode)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    if (oldnode->rb_parent == nil)
        internal->beachline_root = newnode;
    else if (oldnode == oldnode->rb_parent->rb_left)
        oldnode->rb_parent->rb_left = newnode;
    else
        oldnode->rb_parent->rb_right = newnode;
    newnode->rb_parent = oldnode->rb_parent;
}

static void jcv_rb_remove_fixup(jcv_context_internal* internal, jcv_halfedge* node)
{
    while (node != internal->beachline_root && !node->rb_red)
    {
        if (node == node->rb_parent->rb_left)
        {
            jcv_halfedge* sibling = node->rb_parent->rb_right;
            if (sibling->rb_red)
            {
                sibling->rb_red = 0;
                node->rb_parent->rb_red = 1;
                jcv_rb_rotate_left(internal, node->rb_parent);
                sibling = node->rb_parent->rb_right;
            }
            if (!sibling->rb_left->rb_red && !sibling->rb_right->rb_red)
            {
                sibling->rb_red = 1;
                node = node->rb_parent;
            }
            else
            {
                if (!sibling->rb_right->rb_red)
                {
                    sibling->rb_left->rb_red = 0;
                    sibling->rb_red = 1;
                    jcv_rb_rotate_right(internal, sibling);
                    sibling = node->rb_parent->rb_right;
                }
                sibling->rb_red = node->rb_parent->rb_red;
                node->rb_parent->rb_red = 0;
                sibling->rb_right->rb_red = 0;
                jcv_rb_rotate_left(internal, node->rb_parent);
                node = internal->beachline_root;
            }
        }
        else
        {
            jcv_halfedge* sibling = node->rb_parent->rb_left;
            if (sibling->rb_red)
            {
                sibling->rb_red = 0;
                node->rb_parent->rb_red = 1;
                jcv_rb_rotate_right(internal, node->rb_parent);
                sibling = node->rb_parent->rb_left;
            }
            if (!sibling->rb_right->rb_red && !sibling->rb_left->rb_red)
            {
                sibling->rb_red = 1;
                node = node->rb_parent;
            }
            else
            {
                if (!sibling->rb_left->rb_red)
                {
                    sibling->rb_right->rb_red = 0;
                    sibling->rb_red = 1;
                    jcv_rb_rotate_left(internal, sibling);
                    sibling = node->rb_parent->rb_left;
                }
                sibling->rb_red = node->rb_parent->rb_red;
                node->rb_parent->rb_red = 0;
                sibling->rb_left->rb_red = 0;
                jcv_rb_rotate_right(internal, node->rb_parent);
                node = internal->beachline_root;
            }
        }
    }
    node->rb_red = 0;
}

static void jcv_rb_remove(jcv_context_internal* internal, jcv_halfedge* node)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    jcv_halfedge* replacement = node;
    jcv_halfedge* child;
    uint8_t replacement_was_red = replacement->rb_red;

    if (node->rb_left == nil)
    {
        child = node->rb_right;
        jcv_rb_transplant(internal, node, node->rb_right);
    }
    else if (node->rb_right == nil)
    {
        child = node->rb_left;
        jcv_rb_transplant(internal, node, node->rb_left);
    }
    else
    {
        replacement = jcv_rb_minimum(internal, node->rb_right);
        replacement_was_red = replacement->rb_red;
        child = replacement->rb_right;
        if (replacement->rb_parent == node)
        {
            child->rb_parent = replacement;
        }
        else
        {
            jcv_rb_transplant(internal, replacement, replacement->rb_right);
            replacement->rb_right = node->rb_right;
            replacement->rb_right->rb_parent = replacement;
        }
        jcv_rb_transplant(internal, node, replacement);
        replacement->rb_left = node->rb_left;
        replacement->rb_left->rb_parent = replacement;
        replacement->rb_red = node->rb_red;
    }

    if (!replacement_was_red)
        jcv_rb_remove_fixup(internal, child);

    node->rb_parent = nil;
    node->rb_left = nil;
    node->rb_right = nil;
    node->rb_red = 0;
    nil->rb_parent = nil;
    nil->rb_red = 0;
}

static void jcv_beachline_insert_after(jcv_context_internal* internal, jcv_halfedge* after, jcv_halfedge* node)
{
    jcv_halfedge* nil = &internal->beachline_nil;
    jcv_halfedge* parent = nil;

    node->rb_left = nil;
    node->rb_right = nil;
    node->rb_red = 1;

    if (internal->beachline_root == nil)
    {
        internal->beachline_root = node;
    }
    else if (after == internal->beachline_start)
    {
        parent = jcv_rb_minimum(internal, internal->beachline_root);
        assert(parent->rb_left == nil);
        parent->rb_left = node;
    }
    else if (after->rb_right == nil)
    {
        parent = after;
        parent->rb_right = node;
    }
    else
    {
        parent = jcv_rb_minimum(internal, after->rb_right);
        assert(parent->rb_left == nil);
        parent->rb_left = node;
    }

    node->rb_parent = parent;
    jcv_halfedge_link(after, node);
    jcv_rb_insert_fixup(internal, node);
}

static void jcv_beachline_remove(jcv_context_internal* internal, jcv_halfedge* node)
{
    jcv_rb_remove(internal, node);
    jcv_halfedge_unlink(node);
}

// Keeps the priority queue sorted with events sorted in ascending order
// Return 1 if the edges needs to be swapped
static inline int jcv_halfedge_compare( const jcv_halfedge* he1, const jcv_halfedge* he2 )
{
	return  (he1->y == he2->y) ? he1->vertex.x > he2->vertex.x : he1->y > he2->y;
}

static int jcv_halfedge_compare_void(const void* a, const void* b)
{
    return jcv_halfedge_compare((const jcv_halfedge*)a, (const jcv_halfedge*)b);
}

static void jcv_halfedge_set_pos_void(void* node, int pos)
{
    ((jcv_halfedge*)node)->pqpos = pos;
}

static int jcv_halfedge_get_pos_void(const void* node)
{
    return ((const jcv_halfedge*)node)->pqpos;
}

static int jcv_halfedge_intersect(const jcv_halfedge* he1, const jcv_halfedge* he2, jcv_point* out)
{
    const jcv_edge_internal* e1 = he1->edge;
    const jcv_edge_internal* e2 = he2->edge;

    jcv_real d = e1->a * e2->b - e1->b * e2->a;
    if( ((jcv_real)-JCV_EDGE_INTERSECT_THRESHOLD < d && d < (jcv_real)JCV_EDGE_INTERSECT_THRESHOLD) )
    {
        return 0;
    }
    out->x = (e1->c * e2->b - e1->b * e2->c) / d;
    out->y = (e1->a * e2->c - e1->c * e2->a) / d;

    const jcv_edge_internal* e;
    const jcv_halfedge* he;
    if( jcv_point_less( &e1->sites[1]->p, &e2->sites[1]->p) )
    {
        he = he1;
        e = e1;
    }
    else
    {
        he = he2;
        e = e2;
    }

    int right_of_site = out->x >= e->sites[1]->p.x;
    if ((right_of_site && he->direction == JCV_DIRECTION_LEFT) || (!right_of_site && he->direction == JCV_DIRECTION_RIGHT))
    {
        return 0;
    }

    return 1;
}


// Priority queue

static int jcv_pq_moveup(jcv_priorityqueue* pq, int pos)
{
    void** items = pq->items;
    void* node = items[pos];

    for( int parent = (pos >> 1);
         pos > 1 && pq->compare_fn(items[parent], node);
         pos = parent, parent = parent >> 1)
    {
        items[pos] = items[parent];
        pq->set_pos(items[pos], pos);
    }

    pq->set_pos(node, pos);
    items[pos] = node;
    return pos;
}

static int jcv_pq_maxchild(jcv_priorityqueue* pq, int pos)
{
    int child = pos << 1;
    if( child >= pq->numitems )
        return 0;
    void** items = pq->items;
    if( (child + 1) < pq->numitems && pq->compare_fn(items[child], items[child+1]) )
        return child+1;
    return child;
}

static int jcv_pq_movedown(jcv_priorityqueue* pq, int pos)
{
    void** items = pq->items;
    void* node = items[pos];

    int child = jcv_pq_maxchild(pq, pos);
    while( child && pq->compare_fn(node, items[child]) )
    {
        items[pos] = items[child];
        pq->set_pos(items[pos], pos);
        pos = child;
        child = jcv_pq_maxchild(pq, pos);
    }

    items[pos] = node;
    pq->set_pos(items[pos], pos);
    return pos;
}

static void jcv_pq_create(jcv_priorityqueue* pq, int capacity, void** buffer,
                          FJCVPriorityQueueCompare compare,
                          FJCVPriorityQueueSetPos set_pos,
                          FJCVPriorityQueueGetPos get_pos)
{
    pq->maxnumitems = capacity;
    pq->numitems    = 1;
    pq->items       = buffer;
    pq->compare_fn  = compare;
    pq->set_pos     = set_pos;
    pq->get_pos     = get_pos;
}

static int jcv_pq_empty(jcv_priorityqueue* pq)
{
    return pq->numitems == 1 ? 1 : 0;
}

static int jcv_pq_push(jcv_priorityqueue* pq, void* node)
{
    assert(pq->numitems < pq->maxnumitems);
    int n = pq->numitems++;
    pq->items[n] = node;
    return jcv_pq_moveup(pq, n);
}

static void* jcv_pq_pop(jcv_priorityqueue* pq)
{
    void* node = pq->items[1];
    --pq->numitems;
    pq->set_pos(node, 0);
    if (pq->numitems > 1)
    {
        pq->items[1] = pq->items[pq->numitems];
        jcv_pq_movedown(pq, 1);
    }
    return node;
}

static void* jcv_pq_top(jcv_priorityqueue* pq)
{
    return pq->items[1];
}

static void jcv_pq_remove(jcv_priorityqueue* pq, void* node)
{
    if( pq->numitems == 1 )
        return;
    int pos = pq->get_pos(node);
    if( pos == 0 )
        return;

    void** items = pq->items;
    int last = --pq->numitems;
    pq->set_pos(node, 0);
    if (pos == last)
        return;

    items[pos] = items[last];
    if( pos > 1 && pq->compare_fn(items[pos >> 1], items[pos]) )
        jcv_pq_moveup(pq, pos);
    else
        jcv_pq_movedown(pq, pos);
}

// internal functions

static inline jcv_site* jcv_nextsite(jcv_context_internal* internal)
{
    return (internal->currentsite < internal->numsites) ? &internal->sites[internal->currentsite++] : 0;
}

static jcv_halfedge* jcv_get_edge_above_x(jcv_context_internal* internal, const jcv_point* p)
{
    // Gets the arc on the beach line at the x coordinate (i.e. right above the new site event)
    jcv_halfedge* nil = &internal->beachline_nil;
    jcv_halfedge* node = internal->beachline_root;
    jcv_halfedge* predecessor = internal->beachline_start;
    while (node != nil)
    {
        if (jcv_halfedge_rightof(node, p))
        {
            predecessor = node;
            node = node->rb_right;
        }
        else
        {
            node = node->rb_left;
        }
    }
    return predecessor;
}

static int jcv_check_circle_event(const jcv_halfedge* he1, const jcv_halfedge* he2, jcv_point* vertex)
{
    jcv_edge_internal* e1 = he1->edge;
    jcv_edge_internal* e2 = he2->edge;
    if( e1 == 0 || e2 == 0 || e1->sites[1] == e2->sites[1] )
    {
        return 0;
    }

    return jcv_halfedge_intersect(he1, he2, vertex);
}

// Computes center.y + radius without catastrophic cancellation when the
// circumcenter is far below the site. This is algebraically equivalent to
// radius - dy = dx^2 / (radius + dy), where dy = site.y - center.y.
static jcv_real jcv_calc_circle_event_y(const jcv_site* site, const jcv_point* center)
{
    jcv_real dx = site->p.x - center->x;
    jcv_real dy = site->p.y - center->y;
    jcv_real radius = JCV_SQRT(dx * dx + dy * dy);
    if( dy > (jcv_real)0 )
        return site->p.y + (dx * dx) / (radius + dy);
    return center->y + radius;
}

static void jcv_site_event(jcv_context_internal* internal, jcv_site* site)
{
    jcv_halfedge* left   = jcv_get_edge_above_x(internal, &site->p);
    jcv_halfedge* right  = left->right;
    jcv_site*     bottom = jcv_halfedge_rightsite(left);
    if( !bottom )
        bottom = internal->bottomsite;

    jcv_edge_internal* edge = jcv_edge_new(internal, bottom, site);
    edge->next = internal->edges;
    internal->edges = edge;

    jcv_halfedge* edge1 = jcv_halfedge_new(internal, edge, JCV_DIRECTION_LEFT);
    jcv_halfedge* edge2 = jcv_halfedge_new(internal, edge, JCV_DIRECTION_RIGHT);

    jcv_beachline_insert_after(internal, left, edge1);
    jcv_beachline_insert_after(internal, edge1, edge2);

    jcv_point p;
    if( jcv_check_circle_event( left, edge1, &p ) )
    {
        jcv_pq_remove(internal->eventqueue, left);
        left->vertex    = p;
        left->y         = jcv_calc_circle_event_y(site, &p);
        jcv_pq_push(internal->eventqueue, left);
    }
    if( jcv_check_circle_event( edge2, right, &p ) )
    {
        edge2->vertex   = p;
        edge2->y        = jcv_calc_circle_event_y(site, &p);
        jcv_pq_push(internal->eventqueue, edge2);
    }
}

// https://cp-algorithms.com/geometry/oriented-triangle-area.html
static inline jcv_real jcv_determinant(const jcv_point* a, const jcv_point* b, const jcv_point* c)
{
    return (b->x - a->x)*(c->y - a->y) - (b->y - a->y)*(c->x - a->x);
}

static void jcv_graphedge_copy(const jcv_graphedge* source, jcv_edge* target)
{
    const jcv_edge_internal* edge = source->edge;
    int site_index = source->site_index;
    if( edge->sites[1] == 0 )
    {
        jcv_edge_copy(edge, target);
        return;
    }
    int flip = source->flip;
    target->sites[0] = edge->sites[site_index];
    target->sites[1] = edge->sites[1-site_index];
    target->pos[flip] = edge->pos[site_index];
    target->pos[1-flip] = edge->pos[1-site_index];
    target->vertices[flip] = edge->vertices[site_index];
    target->vertices[1-flip] = edge->vertices[1-site_index];
    target->a = edge->a;
    target->b = edge->b;
    target->c = edge->c;
}

static inline const jcv_point* jcv_graphedge_pos(const jcv_graphedge* edge, int endpoint)
{
    int source_index = endpoint == edge->flip ? edge->site_index : 1 - edge->site_index;
    return &edge->edge->pos[source_index];
}

// A monotonic mapping of [0, 2*pi) to [0, 4). It preserves polar ordering
// without the cost of atan2. The scaled fallback avoids overflowing the
// denominator for very large, finite coordinates.
static inline jcv_real jcv_pseudo_angle(jcv_real x, jcv_real y)
{
    jcv_real absx = jcv_abs(x);
    jcv_real absy = jcv_abs(y);
    jcv_real denominator = absx + absy;
    if( denominator == (jcv_real)0 )
        return (jcv_real)0;

    jcv_real angle;
    if( denominator <= (jcv_real)JCV_FLT_MAX )
    {
        angle = y / denominator;
    }
    else
    {
        jcv_real scale = jcv_max(absx, absy);
        angle = (y / scale) / (absx / scale + absy / scale);
    }

    if( x < (jcv_real)0 )
        angle = (jcv_real)2 - angle;
    else if( y < (jcv_real)0 )
        angle = (jcv_real)4 + angle;
    return angle;
}

static inline jcv_real jcv_calc_sort_metric(const jcv_site* site, const jcv_graphedge* edge)
{
    const jcv_point* pos0 = jcv_graphedge_pos(edge, 0);
    const jcv_point* pos1 = jcv_graphedge_pos(edge, 1);
    jcv_real half = 1/(jcv_real)2;
    jcv_real x = (pos0->x + pos1->x) * half;
    jcv_real y = (pos0->y + pos1->y) * half;
    return jcv_pseudo_angle(x - site->p.x, y - site->p.y);
}

static inline int jcv_graphedge_eq(jcv_graphedge* a, jcv_graphedge* b)
{
    return jcv_real_eq(a->angle, b->angle) &&
        jcv_point_eq(jcv_graphedge_pos(a, 0), jcv_graphedge_pos(b, 0)) &&
        jcv_point_eq(jcv_graphedge_pos(a, 1), jcv_graphedge_pos(b, 1));
}

static void jcv_sortedges_insert(jcv_context_internal* internal, jcv_site* site, jcv_graphedge* edge)
{
    int site_index = (int)(site - internal->sites);
    ++internal->build_site_counts[site_index];
    jcv_graphedge** head = &internal->build_site_edges[site_index];
    jcv_graphedge* prev = 0;
    jcv_graphedge* first = *head;
    if( first == 0 || first->angle >= edge->angle )
    {
        edge->next = first;
        *head = edge;
    }
    else
    {
        jcv_graphedge* current = first;
        while( current->next != 0 && current->next->angle < edge->angle )
            current = current->next;
        prev = current;
        edge->next = current->next;
        current->next = edge;
    }
    if( prev && jcv_graphedge_eq(prev, edge) )
    {
        prev->next = edge->next;
        --internal->build_site_counts[site_index];
    }
    else if( edge->next && jcv_graphedge_eq(edge, edge->next) )
    {
        edge->next = edge->next->next;
        --internal->build_site_counts[site_index];
    }
}

static void jcv_create_graphedge(jcv_context_internal* internal, jcv_edge_internal* e, int site_index, jcv_graphedge* ge)
{
    ge->edge = e;
    ge->next = 0;
    ge->site_index = (unsigned char)site_index;
    ge->flip = (unsigned char)(jcv_determinant(&e->sites[0]->p, &e->pos[0], &e->pos[1]) > (jcv_real)0 ? 0 : 1);
    ge->angle = jcv_calc_sort_metric(e->sites[site_index], ge);
    jcv_sortedges_insert(internal, e->sites[site_index], ge);
}

static void jcv_build_graph_edges(jcv_context_internal* internal)
{
    memset(internal->build_site_counts, 0, sizeof(int) * (size_t)internal->numsites);
    int numgraphedges = 0;
    for( jcv_edge_internal* e = internal->edges; e; e = e->next )
    {
        if( !jcv_edge_clipline(internal, e) )
        {
            e->pos[1] = e->pos[0];
            e->a = JCV_INVALID_VALUE;
            continue;
        }
        numgraphedges += 2;
    }
    if( numgraphedges == 0 )
        return;

    jcv_graphedge* graphedges = (jcv_graphedge*)jcv_temp_alloc(internal, sizeof(jcv_graphedge) * (size_t)numgraphedges);
    int cursor = 0;
    for( jcv_edge_internal* e = internal->edges; e; e = e->next )
    {
        if( e->a == JCV_INVALID_VALUE )
            continue;
        jcv_create_graphedge(internal, e, 0, &graphedges[cursor++]);
        jcv_create_graphedge(internal, e, 1, &graphedges[cursor++]);
    }
}

static void jcv_endpos(jcv_edge_internal* e, const jcv_point* p, int direction, int vertex)
{
    e->pos[direction] = *p;
    e->vertices[direction] = vertex;
}

static jcv_edge_internal* jcv_create_gap_edge(jcv_context_internal* internal, jcv_site* site,
    const jcv_point* pos0, const jcv_point* pos1, int vertex0, int vertex1)
{
    jcv_edge_internal* edge  = jcv_alloc_edge(internal);
    edge->pos[0] = *pos0;
    edge->pos[1] = *pos1;
    edge->vertices[0] = vertex0;
    edge->vertices[1] = vertex1;
    edge->sites[0]  = site;
    edge->sites[1]  = 0;
    edge->a = edge->b = edge->c = 0;
    edge->next      = internal->edges;
    internal->edges = edge;
    return edge;
}

static jcv_graphedge* jcv_insert_gap_after(jcv_context_internal* internal, jcv_site* site,
    jcv_graphedge* current, const jcv_point* pos0, const jcv_point* pos1, int vertex0, int vertex1)
{
    int site_index = (int)(site - internal->sites);
    jcv_edge_internal* edge = jcv_create_gap_edge(internal, site, pos0, pos1, vertex0, vertex1);
    jcv_graphedge* gap = jcv_alloc_graphedge(internal);
    gap->edge = edge;
    gap->site_index = 0;
    gap->flip = 0;
    gap->angle = jcv_calc_sort_metric(site, gap);
    if( current )
    {
        gap->next = current->next;
        current->next = gap;
    }
    else
    {
        gap->next = internal->build_site_edges[site_index];
        internal->build_site_edges[site_index] = gap;
    }
    ++internal->build_site_counts[site_index];
    return gap;
}

// Construction helpers used by optional clippers. The handles are deliberately
// opaque so a clipper never depends on the internal edge representation.
static void* jcv_clip_site_edge_head(jcv_context_internal* internal, const jcv_site* site)
{
    return internal->build_site_edges[site - internal->sites];
}

static void* jcv_clip_site_edge_next(void* edge)
{
    return edge ? ((jcv_graphedge*)edge)->next : 0;
}

static void jcv_clip_site_edge_copy(const void* edge, jcv_edge* output)
{
    jcv_graphedge_copy((const jcv_graphedge*)edge, output);
}

static void* jcv_clip_site_insert_gap(jcv_context_internal* internal, jcv_site* site, void* current,
    const jcv_point* pos0, const jcv_point* pos1, int vertex0, int vertex1)
{
    return jcv_insert_gap_after(internal, site, (jcv_graphedge*)current, pos0, pos1, vertex0, vertex1);
}

void jcv_boxshape_fillgaps(const jcv_clipper* clipper, jcv_context_internal* allocator, jcv_site* site)
{
    // They're sorted CCW, so if the current->pos[1] != next->pos[0], then we have a gap
    int site_index = (int)(site - allocator->sites);
    jcv_graphedge* current = allocator->build_site_edges[site_index];
    if( !current )
    {
        assert( allocator->numsites == 1 );
        jcv_point end = {clipper->max.x, clipper->min.y};
        int vertex0 = allocator->numvertices++;
        int vertex1 = allocator->numvertices++;
        current = jcv_insert_gap_after(allocator, site, 0, &clipper->min, &end, vertex0, vertex1);
    }

    jcv_graphedge* next = current->next;
    if( !next )
    {
        jcv_edge current_edge;
        jcv_graphedge_copy(current, &current_edge);
        jcv_point corner;
        if( current_edge.pos[1].x < allocator->rect.max.x && current_edge.pos[1].y == allocator->rect.min.y )
        {
            corner.x = allocator->rect.max.x;
            corner.y = allocator->rect.min.y;
        }
        else if( current_edge.pos[1].x > allocator->rect.min.x && current_edge.pos[1].y == allocator->rect.max.y )
        {
            corner.x = allocator->rect.min.x;
            corner.y = allocator->rect.max.y;
        }
        else if( current_edge.pos[1].y > allocator->rect.min.y && current_edge.pos[1].x == allocator->rect.min.x )
        {
            corner.x = allocator->rect.min.x;
            corner.y = allocator->rect.min.y;
        }
        else
        {
            corner.x = allocator->rect.max.x;
            corner.y = allocator->rect.max.y;
        }
        current = jcv_insert_gap_after(allocator, site, current, &current_edge.pos[1], &corner,
            current_edge.vertices[1], allocator->numvertices++);
        next = allocator->build_site_edges[site_index];
    }

    while( current && next )
    {
        jcv_edge current_edge;
        jcv_edge next_edge;
        jcv_graphedge_copy(current, &current_edge);
        jcv_graphedge_copy(next, &next_edge);
        int current_edge_flags = jcv_get_edge_flags(&current_edge.pos[1], &clipper->min, &clipper->max);
        if( current_edge_flags && !jcv_point_eq(&current_edge.pos[1], &next_edge.pos[0]))
        {
            int next_edge_flags = jcv_get_edge_flags(&next_edge.pos[0], &clipper->min, &clipper->max);
            if( !next_edge_flags )
                return;
            if( current_edge_flags & next_edge_flags )
            {
                jcv_insert_gap_after(allocator, site, current, &current_edge.pos[1], &next_edge.pos[0],
                    current_edge.vertices[1], next_edge.vertices[0]);
            }
            else
            {
                int corner_flag = jcv_edge_flags_to_corner(current_edge_flags);
                if (corner_flag)
                    corner_flag = jcv_corner_rotate_90(corner_flag);
                else
                {
                    if      (current_edge_flags == JCV_EDGE_TOP)    { corner_flag = JCV_CORNER_TOP_LEFT; }
                    else if (current_edge_flags == JCV_EDGE_LEFT)   { corner_flag = JCV_CORNER_BOTTOM_LEFT; }
                    else if (current_edge_flags == JCV_EDGE_BOTTOM) { corner_flag = JCV_CORNER_BOTTOM_RIGHT; }
                    else if (current_edge_flags == JCV_EDGE_RIGHT)  { corner_flag = JCV_CORNER_TOP_RIGHT; }
                }
                jcv_point corner = jcv_corner_to_point(corner_flag, &clipper->min, &clipper->max);
                jcv_insert_gap_after(allocator, site, current, &current_edge.pos[1], &corner,
                    current_edge.vertices[1], allocator->numvertices++);
            }
        }
        current = current->next;
        if( current )
        {
            next = current->next;
            if( !next )
                next = allocator->build_site_edges[site_index];
        }
    }
}


// Since the algorithm leaves gaps at the borders/corner, we want to fill them
static void jcv_fillgaps(jcv_diagram* diagram)
{
    jcv_context_internal* internal = diagram->internal;
    if (!internal->clipper.fill_fn)
        return;

    for( int i = 0; i < internal->numsites; ++i )
    {
        jcv_site* site = &internal->sites[i];
        internal->clipper.fill_fn(&internal->clipper, internal, site);
    }
}

static void jcv_finalize_site_edges(jcv_context_internal* internal)
{
    int total = 0;
    internal->site_edge_offsets = (int*)jcv_alloc(internal, sizeof(int) * (size_t)(internal->numsites + 1));
    for( int i = 0; i < internal->numsites; ++i )
    {
        internal->site_edge_offsets[i] = total;
        total += internal->build_site_counts[i];
    }
    internal->site_edge_offsets[internal->numsites] = total;
    internal->site_edge_refs = (jcv_edge_internal**)jcv_alloc(internal, sizeof(jcv_edge_internal*) * (size_t)total);
    int cursor = 0;
    for( int i = 0; i < internal->numsites; ++i )
    {
        for( jcv_graphedge* graph = internal->build_site_edges[i]; graph; graph = graph->next )
            internal->site_edge_refs[cursor++] = graph->edge;
    }
}

static void jcv_circle_event(jcv_context_internal* internal)
{
    jcv_halfedge* left      = (jcv_halfedge*)jcv_pq_pop(internal->eventqueue);

    jcv_halfedge* leftleft  = left->left;
    jcv_halfedge* right     = left->right;
    jcv_halfedge* rightright= right->right;
    jcv_site* bottom = jcv_halfedge_leftsite(left);
    jcv_site* top    = jcv_halfedge_rightsite(right);

    jcv_point vertex = left->vertex;
    int vertex_index = JCV_INVALID_VERTEX;
    jcv_edge_internal* incident_edges[2] = {left->edge, right->edge};
    // Four or more cocircular sites can report the same vertex in consecutive
    // circle events. In that case one incident edge already carries its index.
    for( int i = 0; i < 2 && vertex_index == JCV_INVALID_VERTEX; ++i )
    {
        for( int endpoint = 0; endpoint < 2; ++endpoint )
        {
            if( incident_edges[i]->vertices[endpoint] >= 0 &&
                incident_edges[i]->pos[endpoint].x == vertex.x &&
                incident_edges[i]->pos[endpoint].y == vertex.y )
            {
                vertex_index = incident_edges[i]->vertices[endpoint];
                break;
            }
        }
    }
    if( vertex_index == JCV_INVALID_VERTEX &&
        (!internal->clipper.test_fn || internal->clipper.test_fn(&internal->clipper, vertex)) )
    {
        vertex_index = internal->numvertices++;
    }
    jcv_endpos(left->edge, &vertex, left->direction, vertex_index);
    jcv_endpos(right->edge, &vertex, right->direction, vertex_index);

    jcv_pq_remove(internal->eventqueue, right);
    jcv_beachline_remove(internal, left);
    jcv_beachline_remove(internal, right);
    jcv_halfedge_delete(internal, left);
    jcv_halfedge_delete(internal, right);

    int direction = JCV_DIRECTION_LEFT;
    if( bottom->p.y > top->p.y )
    {
        jcv_site* temp = bottom;
        bottom = top;
        top = temp;
        direction = JCV_DIRECTION_RIGHT;
    }

    jcv_edge_internal* edge = jcv_edge_new(internal, bottom, top);
    edge->next = internal->edges;
    internal->edges = edge;

    jcv_halfedge* he = jcv_halfedge_new(internal, edge, direction);
    jcv_beachline_insert_after(internal, leftleft, he);
    jcv_endpos(edge, &vertex, JCV_DIRECTION_RIGHT - direction, vertex_index);

    jcv_point p;
    if( jcv_check_circle_event( leftleft, he, &p ) )
    {
        jcv_pq_remove(internal->eventqueue, leftleft);
        leftleft->vertex    = p;
        leftleft->y         = jcv_calc_circle_event_y(bottom, &p);
        jcv_pq_push(internal->eventqueue, leftleft);
    }
    if( jcv_check_circle_event( he, rightright, &p ) )
    {
        he->vertex      = p;
        he->y           = jcv_calc_circle_event_y(bottom, &p);
        jcv_pq_push(internal->eventqueue, he);
    }
}

void jcv_diagram_generate( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* d )
{
    jcv_diagram_generate_useralloc(num_points, points, rect, clipper, 0, jcv_alloc_fn, jcv_free_fn, d);
}

typedef union jcv_cast_align_struct_
{
    char*                   charp;
    void**                  voidpp;
    jcv_context_internal*   internalp;
    jcv_site*               sitep;
    jcv_priorityqueue*      priorityqueuep;
} jcv_cast_align_struct;

static inline void jcv_rect_union(jcv_rect* rect, const jcv_point* p)
{
    rect->min.x = jcv_min(rect->min.x, p->x);
    rect->min.y = jcv_min(rect->min.y, p->y);
    rect->max.x = jcv_max(rect->max.x, p->x);
    rect->max.y = jcv_max(rect->max.y, p->y);
}

static inline void jcv_rect_round(jcv_rect* rect)
{
    rect->min.x = jcv_floor(rect->min.x);
    rect->min.y = jcv_floor(rect->min.y);
    rect->max.x = jcv_ceil(rect->max.x);
    rect->max.y = jcv_ceil(rect->max.y);
}

static inline void jcv_rect_inflate(jcv_rect* rect, jcv_real amount)
{
    rect->min.x -= amount;
    rect->min.y -= amount;
    rect->max.x += amount;
    rect->max.y += amount;
}

static int jcv_prune_duplicates(jcv_context_internal* internal, jcv_rect* rect)
{
    int num_sites = internal->numsites;
    jcv_site* sites = internal->sites;

    jcv_rect r;
    r.min.x = r.min.y = JCV_FLT_MAX;
    r.max.x = r.max.y = -JCV_FLT_MAX;

    int offset = 0;
    // Prune duplicates first
    for (int i = 0; i < num_sites; i++)
    {
        const jcv_site* s = &sites[i];
        // Remove duplicates, to avoid anomalies
        if( i > 0 && jcv_point_eq(&s->p, &sites[i - 1].p) )
        {
            offset++;
            continue;
        }

        sites[i - offset] = sites[i];

        jcv_rect_union(&r, &s->p);
    }
    internal->numsites -= offset;
    if (rect) {
        *rect = r;
    }
    return offset;
}

static int jcv_prune_not_in_shape(jcv_context_internal* internal, jcv_rect* rect)
{
    int num_sites = internal->numsites;
    jcv_site* sites = internal->sites;

    jcv_rect r;
    r.min.x = r.min.y = JCV_FLT_MAX;
    r.max.x = r.max.y = -JCV_FLT_MAX;

    int offset = 0;
    for (int i = 0; i < num_sites; i++)
    {
        const jcv_site* s = &sites[i];

        if (!internal->clipper.test_fn(&internal->clipper, s->p))
        {
            offset++;
            continue;
        }

        sites[i - offset] = sites[i];

        jcv_rect_union(&r, &s->p);
    }
    internal->numsites -= offset;
    if (rect) {
        *rect = r;
    }
    return offset;
}

static jcv_context_internal* jcv_alloc_internal(int num_points, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn)
{
    // Interesting limits from Euler's equation
    // Slide 81: https://courses.cs.washington.edu/courses/csep521/01au/lectures/lecture10slides.pdf
    // Page 3: https://sites.cs.ucsb.edu/~suri/cs235/Voronoi.pdf
    size_t eventssize = (size_t)(num_points*2) * sizeof(void*); // beachline can have max 2*n-5 parabolas
    size_t sitessize = (size_t)num_points * sizeof(jcv_site);
    size_t memsize = sizeof(jcv_priorityqueue) + eventssize + sitessize + sizeof(jcv_context_internal) + 16u; // 16 bytes padding for alignment

    char* originalmem = (char*)allocfn(userallocctx, memsize);
    memset(originalmem, 0, memsize);

    // align memory
    char* mem = (char*)jcv_align(originalmem, sizeof(void*));

    jcv_cast_align_struct aligned;
    aligned.charp = mem;
    jcv_context_internal* internal = aligned.internalp;
    mem += sizeof(jcv_context_internal);
    internal->mem    = originalmem;
    internal->memctx = userallocctx;
    internal->alloc  = allocfn;
    internal->free   = freefn;

    mem = (char*)jcv_align(mem, sizeof(void*));
    aligned.charp = mem;
    internal->sites = aligned.sitep;
    mem += sitessize;

    mem = (char*)jcv_align(mem, sizeof(void*));
    aligned.charp = mem;
    internal->eventqueue = aligned.priorityqueuep;
    mem += sizeof(jcv_priorityqueue);
    assert( ((uintptr_t)mem & (sizeof(void*)-1)) == 0 );

    jcv_cast_align_struct tmp;
    tmp.charp = mem;
    internal->eventmem = tmp.voidpp;

    assert((mem+eventssize) <= (originalmem+memsize));

    return internal;
}

void jcv_diagram_generate_useralloc(int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* d)
{
    if( d->internal )
        jcv_diagram_free( d );

    jcv_context_internal* internal = jcv_alloc_internal(num_points, userallocctx, allocfn, freefn);

    jcv_beachline_init(internal);
    internal->beachline_start = jcv_halfedge_new(internal, 0, 0);
    internal->beachline_end = jcv_halfedge_new(internal, 0, 0);

    internal->beachline_start->left     = 0;
    internal->beachline_start->right    = internal->beachline_end;
    internal->beachline_end->left       = internal->beachline_start;
    internal->beachline_end->right      = 0;

    int max_num_events = num_points*2; // beachline can have max 2*n-5 parabolas
    jcv_pq_create(internal->eventqueue, max_num_events, (void**)internal->eventmem,
                  jcv_halfedge_compare_void, jcv_halfedge_set_pos_void, jcv_halfedge_get_pos_void);

    internal->numsites = num_points;
    jcv_site* sites = internal->sites;

    for( int i = 0; i < num_points; ++i )
    {
        sites[i].p        = points[i];
        sites[i].index    = i;
    }

    qsort(sites, (size_t)num_points, sizeof(jcv_site), jcv_point_cmp);

    jcv_clipper box_clipper;
    if (clipper == 0) {
        box_clipper.test_fn = jcv_boxshape_test;
        box_clipper.clip_fn = jcv_boxshape_clip;
        box_clipper.fill_fn = jcv_boxshape_fillgaps;
        clipper = &box_clipper;
    }
    internal->clipper = *clipper;

    jcv_rect tmp_rect;
    tmp_rect.min.x = tmp_rect.min.y = JCV_FLT_MAX;
    tmp_rect.max.x = tmp_rect.max.y = -JCV_FLT_MAX;
    jcv_prune_duplicates(internal, &tmp_rect);

    // Prune using the test function
    if (internal->clipper.test_fn)
    {
        // e.g. used by the box clipper in the test_fn
        internal->clipper.min = rect ? rect->min : tmp_rect.min;
        internal->clipper.max = rect ? rect->max : tmp_rect.max;

        jcv_prune_not_in_shape(internal, &tmp_rect);

        // The pruning might have made the bounding box smaller
        if (!rect) {
            // In the case of all sites being all on a horizontal or vertical line, the
            // rect area will be zero, and the diagram generation will most likely fail
            jcv_rect_round(&tmp_rect);
            jcv_rect_inflate(&tmp_rect, 10);

            internal->clipper.min = tmp_rect.min;
            internal->clipper.max = tmp_rect.max;
        }
    }

    internal->rect = rect ? *rect : tmp_rect;

    d->min      = internal->rect.min;
    d->max      = internal->rect.max;
    d->numsites = internal->numsites;
    d->internal = internal;

    internal->bottomsite = jcv_nextsite(internal);

    internal->build_site_edges = (jcv_graphedge**)jcv_temp_alloc(internal, sizeof(jcv_graphedge*) * (size_t)internal->numsites);
    memset(internal->build_site_edges, 0, sizeof(jcv_graphedge*) * (size_t)internal->numsites);
    internal->build_site_counts = (int*)internal->eventmem;

    jcv_priorityqueue* pq = internal->eventqueue;
    jcv_site* site = jcv_nextsite(internal);

    int finished = 0;
    while( !finished )
    {
        jcv_point lowest_pq_point;
        if( !jcv_pq_empty(pq) )
        {
            jcv_halfedge* he = (jcv_halfedge*)jcv_pq_top(pq);
            lowest_pq_point.x = he->vertex.x;
            lowest_pq_point.y = he->y;
        }

        if( site != 0 && (jcv_pq_empty(pq) || jcv_point_less(&site->p, &lowest_pq_point) ) )
        {
            jcv_site_event(internal, site);
            site = jcv_nextsite(internal);
        }
        else if( !jcv_pq_empty(pq) )
        {
            jcv_circle_event(internal);
        }
        else
        {
            finished = 1;
        }
    }

    jcv_build_graph_edges(internal);
    jcv_fillgaps(d);
    jcv_finalize_site_edges(internal);
    jcv_temp_free_all(internal);
    internal->build_site_edges = 0;
    d->numvertices = internal->numvertices;
}

#endif // JC_VORONOI_IMPLEMENTATION

/*

ABOUT:

    A fast single file 2D voronoi diagram generator

HISTORY:
    0.10    2026-07-21  - Replaced persistent graph-edge copies with shared edges and iterators
            2026-07-20  - Build per-site edge topology after the sweep
            2026-07-20  - Added unique vertex indices and vertex extraction
            2026-07-20  - Fix invalid topology handling for near-collinear sites
            2026-07-19  - Use a BST to manipulate the beachline
    0.9     2023-01-22  - Modified the Delauney iterator creation api
    0.8     2022-12-20  - Added fix for missing border edges
                          More robust removal of duplicate graph edges
                          Added iterator for Delauney edges
    0.7     2019-10-25  - Added support for clipping against convex polygons
                        - Added JCV_EDGE_INTERSECT_THRESHOLD for edge intersections
                        - Fixed issue where the bounds calculation wasn’t considering all points
    0.6     2018-10-21  - Removed JCV_CEIL/JCV_FLOOR/JCV_FABS
                        - Optimizations: Fewer indirections, better beach head approximation
    0.5     2018-10-14  - Fixed issue where the graph edge had the wrong edge assigned (issue #28)
                        - Fixed issue where a point was falsely passing the jcv_is_valid() test (issue #22)
                        - Fixed jcv_diagram_get_edges() so it now returns _all_ edges (issue #28)
                        - Added jcv_diagram_get_next_edge() to skip zero length edges (issue #10)
                        - Added defines JCV_CEIL/JCV_FLOOR/JCV_FLT_MAX for easier configuration
    0.4     2017-06-03  - Increased the max number of events that are preallocated
    0.3     2017-04-16  - Added clipping box as input argument (Automatically calculated if needed)
                        - Input points are pruned based on bounding box
    0.2     2016-12-30  - Fixed issue of edges not being closed properly
                        - Fixed issue when having many events
                        - Fixed edge sorting
                        - Code cleanup
    0.1                 Initial version

LICENSE:

    The MIT License (MIT)

    Copyright (c) 2015-2019 Mathias Westerdahl

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.


DISCLAIMER:

    This software is supplied "AS IS" without any warranties and support

USAGE:

    The input points are pruned if

        * There are duplicates points
        * The input points are outside of the bounding box (i.e. fail the clipping test function)
        * The input points are rejected by the clipper's test function

    The input bounding box is optional (calculated automatically)

    The input domain is (-FLT_MAX, FLT_MAX] (for floats)

    The api consists of these functions:

    void jcv_diagram_generate( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* diagram );
    void jcv_diagram_generate_useralloc( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram );
    void jcv_diagram_free( jcv_diagram* diagram );

    const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram );
    void jcv_diagram_get_edges( const jcv_diagram* diagram, jcv_edge_iter* iter );
    void jcv_site_get_edges( const jcv_diagram* diagram, const jcv_site* site, jcv_edge_iter* iter );
    int jcv_edge_next( jcv_edge_iter* iter, jcv_edge* edge );

    An example usage:

    #define JC_VORONOI_IMPLEMENTATION
    // If you wish to use doubles
    //#define JCV_REAL_TYPE double
    //#define JCV_ATAN2 atan2
    //#define JCV_FLT_MAX 1.7976931348623157E+308
    #include "jc_voronoi.h"

    void draw_edges(const jcv_diagram* diagram);
    void draw_cells(const jcv_diagram* diagram);

    void generate_and_draw(int numpoints, const jcv_point* points)
    {
        jcv_diagram diagram;
        memset(&diagram, 0, sizeof(jcv_diagram));
        jcv_diagram_generate(count, points, 0, 0, &diagram);

        draw_edges(diagram);
        draw_cells(diagram);

        jcv_diagram_free( &diagram );
    }

    void draw_edges(const jcv_diagram* diagram)
    {
        // If all you need are the edges
        jcv_edge_iter iter;
        jcv_edge edge;
        jcv_diagram_get_edges( diagram, &iter );
        while( jcv_edge_next( &iter, &edge ) )
        {
            draw_line(edge.pos[0], edge.pos[1]);
        }
    }

    void draw_cells(const jcv_diagram* diagram)
    {
        // If you want to draw triangles, or relax the diagram,
        // you can iterate over the sites and get all edges easily
        const jcv_site* sites = jcv_diagram_get_sites( diagram );
        for( int i = 0; i < diagram->numsites; ++i )
        {
            const jcv_site* site = &sites[i];

            jcv_edge_iter iter;
            jcv_edge edge;
            jcv_site_get_edges( diagram, site, &iter );
            while( jcv_edge_next( &iter, &edge ) )
            {
                draw_triangle( site->p, edge.pos[0], edge.pos[1]);
            }
        }
    }

    // Here is a simple example of how to do the relaxations of the cells
    void relax_points(const jcv_diagram* diagram, jcv_point* points)
    {
        const jcv_site* sites = jcv_diagram_get_sites(diagram);
        for( int i = 0; i < diagram->numsites; ++i )
        {
            const jcv_site* site = &sites[i];
            jcv_point sum = site->p;
            int count = 1;

            jcv_edge_iter iter;
            jcv_edge edge;
            jcv_site_get_edges(diagram, site, &iter);
            while( jcv_edge_next(&iter, &edge) )
            {
                sum.x += edge.pos[0].x;
                sum.y += edge.pos[0].y;
                ++count;
            }

            points[site->index].x = sum.x / count;
            points[site->index].y = sum.y / count;
        }
    }

 */
