#define JC_TEST_USE_DEFAULT_MAIN // int main();
#include "jc_test.h"

#include <memory.h>
#include <deque>

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

#include <stdint.h>


struct test_edge_iter
{
    jcv_edge_iter iterator;
    jcv_edge current;
};

struct test_graphedge_iter
{
    jcv_edge_iter iterator;
    std::deque<jcv_edge> edges;
};

static void test_diagram_get_edges(const jcv_diagram* diagram, test_edge_iter* iter)
{
    jcv_diagram_get_edges(diagram, &iter->iterator);
}

static const jcv_edge* test_edge_next(test_edge_iter* iter)
{
    return jcv_edge_next(&iter->iterator, &iter->current) ? &iter->current : 0;
}

static void test_site_get_edges(const jcv_diagram* diagram, const jcv_site* site, test_graphedge_iter* iter)
{
    jcv_site_get_edges(diagram, site, &iter->iterator);
    iter->edges.clear();
}

static const jcv_edge* test_graphedge_next(test_graphedge_iter* iter)
{
    iter->edges.push_back(jcv_edge());
    jcv_edge* edge = &iter->edges.back();
    if( !jcv_edge_next(&iter->iterator, edge) )
    {
        iter->edges.pop_back();
        return 0;
    }
    return edge;
}

static const jcv_point* test_graphedge_get_position(const jcv_diagram*, const jcv_edge* edge, int endpoint)
{
    return &edge->pos[endpoint];
}

static int test_graphedge_get_vertex(const jcv_diagram*, const jcv_edge* edge, int endpoint)
{
    return edge->vertices[endpoint];
}

static const jcv_site* test_graphedge_get_neighbor(const jcv_diagram*, const jcv_edge* edge)
{
    return edge->sites[1];
}

static const jcv_edge* test_graphedge_get_edge(const jcv_diagram*, const jcv_edge* edge)
{
    return edge;
}

#define IMAGE_SIZE 512

typedef struct Context_
{
    int             width;
    jcv_diagram     diagram;
} Context;

struct VoronoiTest : public jc_test_base_class {
    Context* ctx;
    void SetUp()
    {
        ctx = new Context;
        ctx->width = IMAGE_SIZE;
        memset(&ctx->diagram, 0, sizeof(ctx->diagram));
    }
    void TearDown()
    {
        if (ctx->diagram.internal) {
            jcv_diagram_free(&ctx->diagram);
        }
        delete ctx;
    }
};

static int g_counting_fill_calls;

static void counting_box_fillgaps(const jcv_clipper* clipper, jcv_context_internal* internal, jcv_site* site)
{
    ++g_counting_fill_calls;
    jcv_boxshape_fillgaps(clipper, internal, site);
}

static bool check_point_eq(const jcv_point* a, const jcv_point* b)
{
    return a->x == b->x && a->y == b->y;
}

static bool check_graphedge_eq(const jcv_diagram* diagram, const jcv_edge* e, const jcv_point* p1, const jcv_point* p2)
{
    return check_point_eq(test_graphedge_get_position(diagram, e, 0), p1) &&
           check_point_eq(test_graphedge_get_position(diagram, e, 1), p2);
}

#define ASSERT_POINT_EQ( _P1, _P2 ) \
    ASSERT_EQ( (_P1).x, (_P2).x ); \
    ASSERT_EQ( (_P1).y, (_P2).y )

#define ASSERT_POINT_NE( _P1, _P2 ) \
    ASSERT_TRUE( !check_point_eq( &_P1, &_P2) )

static int validate_vertex_indices(const jcv_diagram* diagram)
{
    if( diagram->numvertices == 0 )
        return 0;

    bool* seen = (bool*)calloc((size_t)diagram->numvertices, sizeof(bool));
    jcv_point* vertices = (jcv_point*)malloc((size_t)diagram->numvertices * sizeof(jcv_point));
    jcv_diagram_get_vertices(diagram, vertices);

    int errors = 0;
    const jcv_site* sites = jcv_diagram_get_sites(diagram);
    for( int i = 0; i < diagram->numsites; ++i )
    {
        test_graphedge_iter iter;
        test_site_get_edges(diagram, &sites[i], &iter);
        for( const jcv_edge* edge = test_graphedge_next(&iter); edge; edge = test_graphedge_next(&iter) )
        {
            for( int endpoint = 0; endpoint < 2; ++endpoint )
            {
                int vertex = test_graphedge_get_vertex(diagram, edge, endpoint);
                if( vertex < 0 || vertex >= diagram->numvertices )
                {
                    ++errors;
                    continue;
                }
                seen[vertex] = true;
                errors += !check_point_eq(&vertices[vertex], test_graphedge_get_position(diagram, edge, endpoint));
            }
        }
    }
    for( int i = 0; i < diagram->numvertices; ++i )
        errors += !seen[i];

    free(vertices);
    free(seen);
    return errors;
}

static void check_edges(const jcv_diagram* diagram, const jcv_site* site, int num_expected,
                        const jcv_point* expected_points, const jcv_site** expected_neighbors)
{
    int num_matched = 0;
    for( int i = 0; i < num_expected; ++i )
    {
        test_graphedge_iter iter;
        test_site_get_edges(diagram, site, &iter);
        const jcv_edge* e;
        while( (e = test_graphedge_next(&iter)) != 0 )
        {
            if( check_graphedge_eq(diagram, e, &expected_points[i], &expected_points[(i+1)%num_expected]) )
            {
                ASSERT_EQ( expected_neighbors[i], test_graphedge_get_neighbor(diagram, e) );
                num_matched++;
                break;
            }
        }
    }

    ASSERT_EQ( num_expected, num_matched );
}

TEST_F(VoronoiTest, ceil_floor)
{
    // CEIL
    ASSERT_NEAR((jcv_real)0.0f, jcv_ceil((jcv_real)-0.8f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real)0.0f, jcv_ceil((jcv_real)-0.2f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real)1.0f, jcv_ceil((jcv_real) 0.2f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real)1.0f, jcv_ceil((jcv_real) 0.8f), JCV_REAL_TYPE_EPSILON);

    // FLOOR
    ASSERT_NEAR((jcv_real)-1.0f, jcv_floor((jcv_real)-0.8f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real)-1.0f, jcv_floor((jcv_real)-0.2f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real) 0.0f, jcv_floor((jcv_real) 0.2f), JCV_REAL_TYPE_EPSILON);
    ASSERT_NEAR((jcv_real) 0.0f, jcv_floor((jcv_real) 0.8f), JCV_REAL_TYPE_EPSILON);

    // Check large values
    printf("sizeof(jcv_real) == %zu\n", sizeof(jcv_real));
    if (sizeof(jcv_real) == 8)
    {
        ASSERT_NEAR((jcv_real)-1000000000000000.0, jcv_floor((jcv_real)-999999999999999.5), JCV_REAL_TYPE_EPSILON);
        ASSERT_NEAR((jcv_real) -999999999999999.0, jcv_ceil((jcv_real)-999999999999999.5), JCV_REAL_TYPE_EPSILON);

        ASSERT_NEAR((jcv_real) 1000000000000000.0, jcv_ceil((jcv_real) 999999999999999.5), JCV_REAL_TYPE_EPSILON);
        ASSERT_NEAR((jcv_real)  999999999999999.0, jcv_floor((jcv_real)999999999999999.5), JCV_REAL_TYPE_EPSILON);
    } else{
        ASSERT_NEAR((jcv_real)-1000000.0f, jcv_floor((jcv_real)-999999.5f), JCV_REAL_TYPE_EPSILON);
        ASSERT_NEAR((jcv_real) -999999.0f, jcv_ceil((jcv_real)-999999.5f), JCV_REAL_TYPE_EPSILON);

        ASSERT_NEAR((jcv_real) 1000000.0f, jcv_ceil((jcv_real)999999.5f), JCV_REAL_TYPE_EPSILON);
        ASSERT_NEAR((jcv_real)  999999.0f, jcv_floor((jcv_real) 999999.5f), JCV_REAL_TYPE_EPSILON);
    }
}

TEST_F(VoronoiTest, pseudo_angle_preserves_polar_order)
{
    const jcv_point directions[] = {
        { 1,  0}, { 2,  1}, { 1,  1}, { 1,  2},
        { 0,  1}, {-1,  2}, {-1,  1}, {-2,  1},
        {-1,  0}, {-2, -1}, {-1, -1}, {-1, -2},
        { 0, -1}, { 1, -2}, { 1, -1}, { 2, -1}
    };

    jcv_real previous = jcv_pseudo_angle(directions[0].x, directions[0].y);
    ASSERT_EQ((jcv_real)0, previous);
    for( size_t i = 1; i < sizeof(directions) / sizeof(directions[0]); ++i )
    {
        jcv_real current = jcv_pseudo_angle(directions[i].x, directions[i].y);
        ASSERT_GT(current, previous);
        previous = current;
    }
    ASSERT_LT(previous, (jcv_real)4);

    ASSERT_EQ(jcv_pseudo_angle((jcv_real)1, (jcv_real)1),
              jcv_pseudo_angle((jcv_real)100, (jcv_real)100));
    ASSERT_EQ((jcv_real)0, jcv_pseudo_angle((jcv_real)0, (jcv_real)0));

    const jcv_real large = (jcv_real)JCV_FLT_MAX;
    ASSERT_EQ((jcv_real)0.5, jcv_pseudo_angle(large, large));
    ASSERT_EQ((jcv_real)1.5, jcv_pseudo_angle(-large, large));
    ASSERT_EQ((jcv_real)2.5, jcv_pseudo_angle(-large, -large));
    ASSERT_EQ((jcv_real)3.5, jcv_pseudo_angle(large, -large));
}

TEST_F(VoronoiTest, site_index_and_boundary_share_storage)
{
    struct previous_site_layout
    {
        jcv_point p;
        int index;
    };

    ASSERT_EQ(sizeof(previous_site_layout), sizeof(jcv_site));
    jcv_site site = {};
    site.index = UINT32_C(0x7fffffff);
    site.boundary = 1;
    ASSERT_EQ(UINT32_C(0x7fffffff), site.index);
    ASSERT_EQ(1u, site.boundary);
}

TEST_F(VoronoiTest, parallel_horiz_2)
{
    jcv_point points[] = { {IMAGE_SIZE/4, IMAGE_SIZE/2}, {(IMAGE_SIZE*3)/4, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( 2, ctx->diagram.numsites );

    const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    ASSERT_POINT_EQ( points[0], sites[0].p );
    ASSERT_POINT_EQ( points[1], sites[1].p );

    jcv_point expected_edges_0[4];
    expected_edges_0[0].x = IMAGE_SIZE/2;
    expected_edges_0[0].y = ctx->diagram.min.y;
    expected_edges_0[1].x = IMAGE_SIZE/2;
    expected_edges_0[1].y = ctx->diagram.max.y;
    expected_edges_0[2].x = ctx->diagram.min.x;
    expected_edges_0[2].y = ctx->diagram.max.y;
    expected_edges_0[3].x = ctx->diagram.min.x;
    expected_edges_0[3].y = ctx->diagram.min.y;
    const jcv_site* expected_neighbors_0[4];
    expected_neighbors_0[0] = &sites[1];
    expected_neighbors_0[1] = 0;
    expected_neighbors_0[2] = 0;
    expected_neighbors_0[3] = 0;

    jcv_point expected_edges_1[4];
    expected_edges_1[0].x = IMAGE_SIZE/2;
    expected_edges_1[0].y = ctx->diagram.max.y;
    expected_edges_1[1].x = IMAGE_SIZE/2;
    expected_edges_1[1].y = ctx->diagram.min.y;
    expected_edges_1[2].x = ctx->diagram.max.x;
    expected_edges_1[2].y = ctx->diagram.min.y;
    expected_edges_1[3].x = ctx->diagram.max.x;
    expected_edges_1[3].y = ctx->diagram.max.y;
    const jcv_site* expected_neighbors_1[4];
    expected_neighbors_1[0] = &sites[0];
    expected_neighbors_1[1] = 0;
    expected_neighbors_1[2] = 0;
    expected_neighbors_1[3] = 0;

    check_edges( &ctx->diagram, &sites[0], 4, expected_edges_0, expected_neighbors_0 );
    check_edges( &ctx->diagram, &sites[1], 4, expected_edges_1, expected_neighbors_1 );
}

TEST_F(VoronoiTest, parallel_vert_2)
{
    jcv_point points[] = { {IMAGE_SIZE/2, (IMAGE_SIZE*1)/4}, {IMAGE_SIZE/2, (IMAGE_SIZE*3)/4} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( 2, ctx->diagram.numsites );

    const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    ASSERT_POINT_EQ( points[0], sites[0].p );
    ASSERT_POINT_EQ( points[1], sites[1].p );
}

TEST_F(VoronoiTest, one_site)
{
    jcv_point points[] = { {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( 1, ctx->diagram.numsites );


    jcv_point expected_edges_0[4];
    expected_edges_0[0].x = ctx->diagram.min.x;
    expected_edges_0[0].y = ctx->diagram.min.y;
    expected_edges_0[1].x = ctx->diagram.max.x;
    expected_edges_0[1].y = ctx->diagram.min.y;
    expected_edges_0[2].x = ctx->diagram.max.x;
    expected_edges_0[2].y = ctx->diagram.max.y;
    expected_edges_0[3].x = ctx->diagram.min.x;
    expected_edges_0[3].y = ctx->diagram.max.y;
    const jcv_site* expected_neighbors_0[4];
    expected_neighbors_0[0] = 0;
    expected_neighbors_0[1] = 0;
    expected_neighbors_0[2] = 0;
    expected_neighbors_0[3] = 0;

    const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    check_edges( &ctx->diagram, &sites[0], 4, expected_edges_0, expected_neighbors_0 );
}

TEST_F(VoronoiTest, culling)
{
    jcv_point points[] = { {IMAGE_SIZE/2, -IMAGE_SIZE/2}, {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_rect rect = { {0, 0}, {IMAGE_SIZE, IMAGE_SIZE} };
    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);

    ASSERT_EQ( 1, ctx->diagram.numsites );

    jcv_point expected_edges_0[4];
    expected_edges_0[0].x = 0;
    expected_edges_0[0].y = 0;
    expected_edges_0[1].x = IMAGE_SIZE;
    expected_edges_0[1].y = 0;
    expected_edges_0[2].x = IMAGE_SIZE;
    expected_edges_0[2].y = IMAGE_SIZE;
    expected_edges_0[3].x = 0;
    expected_edges_0[3].y = IMAGE_SIZE;
    const jcv_site* expected_neighbors_0[4];
    expected_neighbors_0[0] = 0;
    expected_neighbors_0[1] = 0;
    expected_neighbors_0[2] = 0;
    expected_neighbors_0[3] = 0;

    const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    check_edges( &ctx->diagram, &sites[0], 4, expected_edges_0, expected_neighbors_0 );
}


static jcv_context_internal* setup_test_context_internal(int num_points, jcv_point* points, void* ctx)
{
    jcv_context_internal* internal = jcv_alloc_internal(num_points, ctx, jcv_alloc_fn, jcv_free_fn);
    internal->numsites = num_points;
    jcv_site* sites = internal->sites;

    for( int i = 0; i < num_points; ++i )
    {
        sites[i].p        = points[i];
        sites[i].index    = (uint32_t)i;
    }
    qsort(sites, (size_t)num_points, sizeof(jcv_site), jcv_point_cmp);

    return internal;
}

static void setup_clip_shape_box(jcv_context_internal* internal, jcv_rect rect)
{
    jcv_clipper box_clipper;
    box_clipper.test_fn = jcv_boxshape_test;
    box_clipper.clip_fn = jcv_boxshape_clip;
    box_clipper.fill_fn = jcv_boxshape_fillgaps;
    internal->clipper = box_clipper;

    internal->clipper.min = rect.min;
    internal->clipper.max = rect.max;
}

static void teardown_test_context_internal(jcv_context_internal* internal)
{
    jcv_free_fn(0, internal->mem);
}

TEST_F(VoronoiTest, prune_duplicates)
{
    jcv_point duplicate = {1,2};
    jcv_point points[] = { {1,2}, {2,2}, {1,2}, {3,3}};
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_context_internal* internal = setup_test_context_internal(num_points, points, 0);
    ASSERT_EQ( 4, internal->numsites );

    jcv_rect rect;
    int num_removed = jcv_prune_duplicates(internal, &rect);

    ASSERT_EQ( 1, num_removed );
    ASSERT_EQ( 3, internal->numsites );

    int count = 0;
    for (int i = 0; i < internal->numsites; ++i)
    {
        if (internal->sites[i].p.x == duplicate.x &&
            internal->sites[i].p.y == duplicate.y) {
            count++;
        }
    }
    ASSERT_EQ( 1, count );

    ASSERT_EQ( 1, rect.min.x );
    ASSERT_EQ( 2, rect.min.y );
    ASSERT_EQ( 3, rect.max.x );
    ASSERT_EQ( 3, rect.max.y );

    teardown_test_context_internal(internal);
}

TEST_F(VoronoiTest, custom_clipper_fill_visits_all_sites)
{
    jcv_point points[] = {
        {20,20}, {50,20}, {80,20},
        {20,50}, {50,50}, {80,50},
        {20,80}, {50,80}, {80,80}
    };
    jcv_rect rect = {{0,0}, {100,100}};
    jcv_clipper clipper = {};
    clipper.test_fn = jcv_boxshape_test;
    clipper.clip_fn = jcv_boxshape_clip;
    clipper.fill_fn = counting_box_fillgaps;

    g_counting_fill_calls = 0;
    jcv_diagram_generate((int)(sizeof(points) / sizeof(points[0])), points, &rect, &clipper, &ctx->diagram);
    ASSERT_EQ(ctx->diagram.numsites, g_counting_fill_calls);
}

TEST_F(VoronoiTest, box_clipper_marks_only_boundary_sites)
{
    jcv_point points[] = {
        {20,20}, {50,20}, {80,20},
        {20,50}, {50,50}, {80,50},
        {20,80}, {50,80}, {80,80}
    };
    jcv_rect rect = {{0,0}, {100,100}};
    jcv_diagram_generate((int)(sizeof(points) / sizeof(points[0])), points, &rect, 0, &ctx->diagram);

    int boundary_sites = 0;
    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        boundary_sites += sites[i].boundary;
        if( sites[i].p.x == 50 && sites[i].p.y == 50 )
            ASSERT_EQ(0u, sites[i].boundary);
    }
    ASSERT_EQ(8, boundary_sites);
}

TEST_F(VoronoiTest, prune_not_in_shape)
{
    jcv_point points[] = { {0,0}, {1,9}, {2,8}, {5,5}, {8,2}, {9,1}, {10,10}};
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_context_internal* internal = setup_test_context_internal(num_points, points, 0);
    ASSERT_EQ( num_points, internal->numsites );

    jcv_rect smaller_rect;
    smaller_rect.min.x = 2;
    smaller_rect.min.y = 2;
    smaller_rect.max.x = 8;
    smaller_rect.max.y = 8;
    setup_clip_shape_box(internal, smaller_rect);

    jcv_rect rect;
    int num_removed = jcv_prune_not_in_shape(internal, &rect);

    ASSERT_EQ( 4, num_removed );
    ASSERT_EQ( 3, internal->numsites );

    ASSERT_EQ( 2, rect.min.x );
    ASSERT_EQ( 2, rect.min.y );
    ASSERT_EQ( 8, rect.max.x );
    ASSERT_EQ( 8, rect.max.y );

    teardown_test_context_internal(internal);
}

// for debugging
// static void write_points(const char* name, int num_points, const jcv_point* points)
// {
//     FILE* file = fopen(name, "wb");
//     if( file )
//     {
//         fwrite(points, 1, sizeof(jcv_point)*(unsigned int)num_points, file);
//         fclose(file);
//         printf("Wrote: %s\n", name);
//     }
// }

TEST_F(VoronoiTest, same_site)
{
    jcv_point points[] = { {IMAGE_SIZE/2, IMAGE_SIZE/2}, {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( 1, ctx->diagram.numsites );
}

TEST_F(VoronoiTest, many)
{
    srand(0);

    int maxcount = 10;
    for( int count = 0; count < maxcount; ++count )
    {
        const int num_points = 10000;
        jcv_point* points = (jcv_point*)malloc( sizeof(jcv_point) * num_points );

        int pointoffset = 0;

        for( int i = 0; i < num_points; ++i )
        {
            points[i].x = (float) (pointoffset + rand() % (IMAGE_SIZE-2*pointoffset));
            points[i].y = (float) (pointoffset + rand() % (IMAGE_SIZE-2*pointoffset));
        }

        jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

        //ASSERT_EQ( num_points, ctx->diagram.numsites );

        int edges_without_vertices = 0;
        test_edge_iter edge_iter;
        test_diagram_get_edges(&ctx->diagram, &edge_iter);
        for( const jcv_edge* edge = test_edge_next(&edge_iter); edge; edge = test_edge_next(&edge_iter) )
            edges_without_vertices += edge->vertices[0] < 0 || edge->vertices[1] < 0;
        ASSERT_EQ(0, edges_without_vertices);
        ASSERT_EQ(0, validate_vertex_indices(&ctx->diagram));

        if( count < maxcount-1 )
        {
            jcv_diagram_free( &ctx->diagram );
            memset(&ctx->diagram, 0, sizeof(jcv_diagram));
        }

        free(points);
    }
}

TEST_F(VoronoiTest, many_diagonal)
{
    const int num_points = 1000;
    jcv_point* points = (jcv_point*)malloc( sizeof(jcv_point) * num_points );

    for( int i = 0; i < num_points; ++i )
    {
        points[i].x = i / (float)num_points;
        points[i].y = points[i].x;
    }

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( num_points, ctx->diagram.numsites );

    free(points);
}

// Testing a large event queue (https://github.com/JCash/voronoi/issues/3)
TEST_F(VoronoiTest, many_circle)
{
    const int num_points = 100;
    jcv_point* points = (jcv_point*)malloc( sizeof(jcv_point) * num_points );

    float half_size = IMAGE_SIZE/2;
    for( int i = 0; i < num_points; ++i )
    {
        float a = (2 * JCV_PI * i ) / (float)num_points;
        points[i].x = half_size + half_size * 0.75f * cosf(a);
        points[i].y = half_size + half_size * 0.75f * sinf(a);
    }

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    ASSERT_EQ( num_points, ctx->diagram.numsites );

    free(points);
}

TEST_F(VoronoiTest, site_with_more_than_128_edges)
{
    const int outer_count = 129;
    const int num_points = outer_count + 1;
    jcv_point* points = (jcv_point*)malloc(sizeof(jcv_point) * num_points);

    points[0].x = 0;
    points[0].y = 0;
    for( int i = 0; i < outer_count; ++i )
    {
        jcv_real angle = (jcv_real)(2 * JCV_PI * i) / (jcv_real)outer_count;
        points[i+1].x = (jcv_real)cos((double)angle);
        points[i+1].y = (jcv_real)sin((double)angle);
    }

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    const jcv_site* center = 0;
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        if( sites[i].index == 0 )
        {
            center = &sites[i];
            break;
        }
    }
    ASSERT_NE((const jcv_site*)0, center);

    test_graphedge_iter iter;
    test_site_get_edges(&ctx->diagram, center, &iter);
    int edge_count = 0;
    while( test_graphedge_next(&iter) )
        ++edge_count;

    ASSERT_EQ(outer_count, edge_count);
    free(points);
}

TEST_F(VoronoiTest, crash1)
{
    jcv_point points[] = { {-0.148119405f, 0.307878017f}, {-0.0949054062f, -0.37929377f}, {0.170877606f, 0.477409601f}, {-0.0634334087f, 0.0787638053f}, {-0.244908407f, 0.402904421f}, {-0.0830767974f, 0.442425013f} };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );
}

// Issue: https://github.com/JCash/voronoi/issues/10
TEST_F(VoronoiTest, issue10_zero_edge_length)
{
    jcv_point points[] = {
        { -5.544f, -3.492f },
        { -5.010f, -4.586f },
        { 3.030f, -3.045f },
        { -5.279f, -5.474f },
    };
    jcv_rect rect = { {-6.418f, -5.500f}, {3.140f, 0.009f} };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );

    test_edge_iter edge_iter;
    test_diagram_get_edges(&ctx->diagram, &edge_iter);
    const jcv_edge* edge = test_edge_next(&edge_iter);
    while( edge )
    {
        ASSERT_POINT_NE(edge->pos[0], edge->pos[1]);
        edge = test_edge_next(&edge_iter);
    }
}


// Issue: https://github.com/JCash/voronoi/issues/22
TEST_F(VoronoiTest, issue22_wrong_edge_count)
{
    jcv_point points[] = {
        { 0, 0 },
        { 2, 0 },
        { -2, 0 },
        { 0, -2 },
    };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );

    const jcv_site *sites = jcv_diagram_get_sites(&ctx->diagram);
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        int count = 0;
        test_graphedge_iter iter;
        test_site_get_edges(&ctx->diagram, site, &iter);
        while( test_graphedge_next(&iter) )
            ++count;
        ASSERT_EQ( 4, count );
    }
}


// Issue: https://github.com/JCash/voronoi/issues/28
TEST_F(VoronoiTest, issue28_not_all_edges_returned)
{
    jcv_point points[] = {
        { 0, 0 },
        { 2, 0 },
        { -2, 0 },
    };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );

    // 1. count all graph edges
    int count = 0;

    const jcv_site *sites = jcv_diagram_get_sites(&ctx->diagram);

    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        test_graphedge_iter graph_iter;
        test_site_get_edges(&ctx->diagram, site, &graph_iter);
        const jcv_edge* e;
        while( (e = test_graphedge_next(&graph_iter)) != 0 ) {
            const jcv_site* neighbor = test_graphedge_get_neighbor(&ctx->diagram, e);
            // If it's a border edge
            if (neighbor == 0)
                ++count;
            // or if the neighbor has a higher index (i.e. only count the edge once)
            else if(neighbor->index > site->index)
                ++count;

            // 2. Make sure the graph edge points are the same as the edge points
            const jcv_edge* source = test_graphedge_get_edge(&ctx->diagram, e);
            bool eq =   check_graphedge_eq(&ctx->diagram, e, &source->pos[0], &source->pos[1]) ||
                        check_graphedge_eq(&ctx->diagram, e, &source->pos[1], &source->pos[0]);
            ASSERT_TRUE(eq);
        }
    }
    ASSERT_EQ( 10, count );

    // 3. count the edges
    int count_edges = 0;
    test_edge_iter edge_iter;
    test_diagram_get_edges(&ctx->diagram, &edge_iter);
    const jcv_edge* edge = test_edge_next(&edge_iter);
    while (edge) {
        ++count_edges;
        edge = test_edge_next(&edge_iter);
    }
    ASSERT_EQ( 10, count_edges );
}

static inline int is_closed_loop(const jcv_diagram* diagram, const jcv_site* site);

// When using these points, the test asserts
TEST_F(VoronoiTest, issue38_numsites_equals_one_assert)
{
    jcv_point points[4];
    points[0].x = 191.969146728515625000; points[0].y = -15.99730110168457031250;
    points[1].x = -49.232059478759765625; points[1].y = -15.99410915374755859375;
    points[2].x = 206.767944335937500000; points[2].y = -15.99410915374755859375;
    points[3].x = 127.188446044921875000; points[3].y = -15.99205684661865234375;

    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );
}

#if defined(TEST_USE_DOUBLE)
// Regression test for #49. Reduced from the reporter's 78033-point input.
TEST_F(VoronoiTest, issue49_fillgaps_assert)
{
    jcv_point points[] = {
        { -5715.7060000000001, -4806.0259999999998 },
        { -5715.2060011216226, -4806.024940933673 },
        { -5714.7060022432452, -4806.0238818673461 },
        { -4404.5410240739639, -4808.1697195474244 },
        { -4404.0410267488487, -4808.1713550526938 },
        { -3763.3760882712004, -4810.2779716738905 },
        { -3762.8760909460852, -4810.2796071791599 },
        { -3736.9160709735625, -4810.3649570418402 },
    };
    int num_points = (int)(sizeof(points) / sizeof(points[0]));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ(num_points, ctx->diagram.numsites);

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    for( int i = 0; i < ctx->diagram.numsites; ++i )
        ASSERT_TRUE(is_closed_loop(&ctx->diagram, &sites[i]));
}
#endif

// Checks if the points of all edges are connected
static inline int is_closed_loop(const jcv_diagram* diagram, const jcv_site* site)
{
    test_graphedge_iter iter;
    test_site_get_edges(diagram, site, &iter);
    const jcv_edge* first = test_graphedge_next(&iter);
    if( !first )
        return 0;

    const jcv_edge* edge = first;
    const jcv_edge* next;
    while( (next = test_graphedge_next(&iter)) != 0 )
    {
        if( !jcv_point_eq(test_graphedge_get_position(diagram, edge, 1),
                          test_graphedge_get_position(diagram, next, 0)) )
            return 0;
        edge = next;
    }
    return jcv_point_eq(test_graphedge_get_position(diagram, edge, 1),
                        test_graphedge_get_position(diagram, first, 0));
}

#if 0
// TODO: Re-enable when degenerate circle events have a shared robustness
// policy independent of the beach-line tree implementation. See
// ../plan_degenerate_inputs.md.
static inline int is_counter_clockwise(const jcv_diagram* diagram, const jcv_site* site)
{
    double twice_area = 0.0;
    test_graphedge_iter iter;
    test_site_get_edges(diagram, site, &iter);
    for( const jcv_edge* edge = test_graphedge_next(&iter); edge; edge = test_graphedge_next(&iter) )
    {
        const jcv_point* p0 = test_graphedge_get_position(diagram, edge, 0);
        const jcv_point* p1 = test_graphedge_get_position(diagram, edge, 1);
        double x0 = (double)p0->x - (double)site->p.x;
        double y0 = (double)p0->y - (double)site->p.y;
        double x1 = (double)p1->x - (double)site->p.x;
        double y1 = (double)p1->y - (double)site->p.y;
        twice_area += x0 * y1 - y0 * x1;
    }
    return twice_area > 0.0;
}

static uint32_t near_cocircular_random_next(uint32_t* state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

TEST_F(VoronoiTest, near_cocircular_cells_are_closed_and_ccw)
{
    const int num_points = 1000;
    jcv_point* points = new jcv_point[num_points];
    uint32_t state = 45;
    for( int i = 0; i < num_points; ++i )
    {
        float angle = 6.2831853071795864769f * (float)i / (float)num_points;
        float radius = 3000.0f + (float)(near_cocircular_random_next(&state) % 7) * 0.0001f;
        points[i].x = (jcv_real)(5000.0f + radius * cosf(angle));
        points[i].y = (jcv_real)(5000.0f + radius * sinf(angle));
    }
    jcv_rect rect = {{-100, -100}, {10100, 10100}};
    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);

    ASSERT_EQ(num_points, ctx->diagram.numsites);
    ASSERT_EQ(0, validate_vertex_indices(&ctx->diagram));
    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        ASSERT_TRUE(is_closed_loop(&ctx->diagram, &sites[i]));
        ASSERT_TRUE(is_counter_clockwise(&ctx->diagram, &sites[i]));
    }
    delete[] points;
}
#endif

static int collect_tree_inorder(jcv_halfedge* node, jcv_halfedge* nil, jcv_halfedge** nodes, int count)
{
    if (node == nil)
        return count;
    count = collect_tree_inorder(node->tree_left, nil, nodes, count);
    nodes[count++] = node;
    return collect_tree_inorder(node->tree_right, nil, nodes, count);
}

static int validate_ravl_node(jcv_halfedge* node, jcv_halfedge* nil, jcv_halfedge* parent)
{
    if (node == nil)
        return -1;

    EXPECT_EQ(parent, node->tree_parent);
    int left_height = validate_ravl_node(node->tree_left, nil, node);
    int right_height = validate_ravl_node(node->tree_right, nil, node);
    int left_rank = node->tree_left == nil ? -1 : (int)node->tree_left->tree_rank;
    int right_rank = node->tree_right == nil ? -1 : (int)node->tree_right->tree_rank;
    EXPECT_LT(left_rank, (int)node->tree_rank);
    EXPECT_LT(right_rank, (int)node->tree_rank);
    int height = 1 + (left_height > right_height ? left_height : right_height);
    EXPECT_LE(height, (int)node->tree_rank);
    return height;
}

static void validate_beachline(jcv_context_internal* internal, int expected_count)
{
    jcv_halfedge* nil = &internal->beachline_nil;

    if (internal->beachline_root != nil)
    {
        ASSERT_EQ(nil, internal->beachline_root->tree_parent);
        validate_ravl_node(internal->beachline_root, nil, nil);
    }

    jcv_halfedge* tree_nodes[32];
    int tree_count = collect_tree_inorder(internal->beachline_root, nil, tree_nodes, 0);
    ASSERT_EQ(expected_count, tree_count);

    int list_count = 0;
    jcv_halfedge* previous = internal->beachline_start;
    for (jcv_halfedge* node = previous->right; node != internal->beachline_end; node = node->right)
    {
        ASSERT_EQ(previous, node->left);
        ASSERT_EQ(tree_nodes[list_count], node);
        previous = node;
        ++list_count;
    }
    ASSERT_EQ(previous, internal->beachline_end->left);
    ASSERT_EQ(expected_count, list_count);
}

TEST_F(VoronoiTest, beachline_ravl_insert_remove)
{
    jcv_context_internal internal;
    jcv_halfedge start;
    jcv_halfedge end;
    jcv_halfedge nodes[16];
    memset(&internal, 0, sizeof(internal));
    memset(&start, 0, sizeof(start));
    memset(&end, 0, sizeof(end));
    memset(nodes, 0, sizeof(nodes));

    jcv_beachline_init(&internal);
    internal.beachline_start = &start;
    internal.beachline_end = &end;
    start.right = &end;
    end.left = &start;

    for (int i = 0; i < 16; ++i)
    {
        jcv_halfedge* after = (i & 1) ? &start : end.left;
        jcv_beachline_insert_after(&internal, after, &nodes[i]);
        validate_beachline(&internal, i + 1);
    }

    const int removal_order[] = {7, 0, 15, 8, 3, 12, 1, 14, 2, 13, 4, 11, 5, 10, 6, 9};
    for (int i = 0; i < 16; ++i)
    {
        jcv_beachline_remove(&internal, &nodes[removal_order[i]]);
        validate_beachline(&internal, 15 - i);
    }
    ASSERT_EQ(&internal.beachline_nil, internal.beachline_root);
    ASSERT_EQ(&end, start.right);
    ASSERT_EQ(&start, end.left);
}

TEST_F(VoronoiTest, unique_vertices)
{
    jcv_point points[] = {
        {0.25f, 0.25f},
        {0.75f, 0.25f},
        {0.25f, 0.75f},
        {0.75f, 0.75f},
    };
    jcv_rect rect = {{0, 0}, {1, 1}};
    jcv_diagram_generate(4, points, &rect, 0, &ctx->diagram);

    // Four corners, four edge midpoints and one four-way center vertex.
    ASSERT_EQ(9, ctx->diagram.numvertices);
    ASSERT_EQ(ctx->diagram.numvertices, jcv_get_num_vertices(&ctx->diagram));
    bool indices[9] = {};
    jcv_point vertices[9];
    jcv_diagram_get_vertices(&ctx->diagram, vertices);

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        test_graphedge_iter graph_iter;
        test_site_get_edges(&ctx->diagram, &sites[i], &graph_iter);
        const jcv_edge* first = test_graphedge_next(&graph_iter);
        ASSERT_TRUE(first != 0);
        const jcv_edge* edge = first;
        while( edge )
        {
            const jcv_edge* next = test_graphedge_next(&graph_iter);
            if( !next ) next = first;
            int vertex0 = test_graphedge_get_vertex(&ctx->diagram, edge, 0);
            int vertex1 = test_graphedge_get_vertex(&ctx->diagram, edge, 1);
            ASSERT_GE(vertex0, 0);
            ASSERT_LT(vertex0, ctx->diagram.numvertices);
            ASSERT_GE(vertex1, 0);
            ASSERT_LT(vertex1, ctx->diagram.numvertices);
            ASSERT_EQ(vertex1, test_graphedge_get_vertex(&ctx->diagram, next, 0));
            ASSERT_POINT_EQ(vertices[vertex0], *test_graphedge_get_position(&ctx->diagram, edge, 0));
            ASSERT_POINT_EQ(vertices[vertex1], *test_graphedge_get_position(&ctx->diagram, edge, 1));
            indices[vertex0] = true;
            indices[vertex1] = true;
            edge = next == first ? 0 : next;
        }
    }

    for( int i = 0; i < ctx->diagram.numvertices; ++i )
        ASSERT_TRUE(indices[i]);

    test_edge_iter edge_iter;
    test_diagram_get_edges(&ctx->diagram, &edge_iter);
    for( const jcv_edge* edge = test_edge_next(&edge_iter); edge; edge = test_edge_next(&edge_iter) )
    {
        ASSERT_GE(edge->vertices[0], 0);
        ASSERT_LT(edge->vertices[0], ctx->diagram.numvertices);
        ASSERT_GE(edge->vertices[1], 0);
        ASSERT_LT(edge->vertices[1], ctx->diagram.numvertices);
        ASSERT_POINT_EQ(vertices[edge->vertices[0]], edge->pos[0]);
        ASSERT_POINT_EQ(vertices[edge->vertices[1]], edge->pos[1]);
    }
}

// Issue: https://github.com/JCash/voronoi/issues/83
TEST_F(VoronoiTest, issue83_fillgaps_terminates)
{
    jcv_point points[] = {
        {1.5484909, -0.21024238},
        {1.5413352, -0.21024236},
        {1.5378445, -0.21024236},
        {1.5343539, -0.21024235},
        {1.5308806, -0.19333011},
    };
    int num_points = (int)(sizeof(points) / sizeof(points[0]));
    jcv_rect rect = {{-4, -4}, {4, 4}};

    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);

    ASSERT_EQ(num_points, ctx->diagram.numsites);
}

TEST_F(VoronoiTest, issue91_cells_are_closed)
{
    jcv_point points[] = {
        {580, 20.9086037f},
        {670, 20.9085979f},
        {700, 20.9085999f},
        {730, 20.9085999f},
        {760, 20.9086056f},
        {710.857178f, 46.506916f},
    };
    int num_points = (int)(sizeof(points) / sizeof(points[0]));
    jcv_rect rect = {{0, 0}, {1655, 1462}};

    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);
    ASSERT_EQ(num_points, ctx->diagram.numsites);

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    int num_open_cells = 0;
    int num_invalid_edges = 0;
    int num_invalid_neighbors = 0;
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        ASSERT_POINT_EQ(points[sites[i].index], sites[i].p);
        int count = 0;
        test_graphedge_iter count_iter;
        test_site_get_edges(&ctx->diagram, &sites[i], &count_iter);
        while( test_graphedge_next(&count_iter) )
            ++count;
        ASSERT_TRUE(count < 64);
        if( !is_closed_loop(&ctx->diagram, &sites[i]) )
        {
            ++num_open_cells;
        }

        test_graphedge_iter graph_iter;
        test_site_get_edges(&ctx->diagram, &sites[i], &graph_iter);
        for( const jcv_edge* edge = test_graphedge_next(&graph_iter); edge; edge = test_graphedge_next(&graph_iter) )
        {
            const jcv_point* pos0 = test_graphedge_get_position(&ctx->diagram, edge, 0);
            const jcv_point* pos1 = test_graphedge_get_position(&ctx->diagram, edge, 1);
            const jcv_site* neighbor = test_graphedge_get_neighbor(&ctx->diagram, edge);
            const jcv_edge* source = test_graphedge_get_edge(&ctx->diagram, edge);
            double xmid = ((double)pos0->x + (double)pos1->x) * 0.5;
            double ymid = ((double)pos0->y + (double)pos1->y) * 0.5;
            double dx = xmid - (double)sites[i].p.x;
            double dy = ymid - (double)sites[i].p.y;
            double site_distance_sq = dx * dx + dy * dy;
            double tolerance = site_distance_sq * 1.0e-4 + 1.0e-4;
            if( neighbor )
            {
                dx = xmid - (double)neighbor->p.x;
                dy = ymid - (double)neighbor->p.y;
                double neighbor_distance_sq = dx * dx + dy * dy;
                if( fabs(site_distance_sq - neighbor_distance_sq) > tolerance )
                    ++num_invalid_neighbors;
                if( !((source->sites[0] == &sites[i] && source->sites[1] == neighbor) ||
                      (source->sites[1] == &sites[i] && source->sites[0] == neighbor)) )
                    ++num_invalid_neighbors;
            }
            for( int j = 0; j < ctx->diagram.numsites; ++j )
            {
                dx = xmid - (double)sites[j].p.x;
                dy = ymid - (double)sites[j].p.y;
                double other_distance_sq = dx * dx + dy * dy;
                if( other_distance_sq + tolerance < site_distance_sq )
                {
                    ++num_invalid_edges;
                    break;
                }
            }
        }
    }
    ASSERT_EQ(0, num_open_cells);
    ASSERT_EQ(0, num_invalid_edges);
    ASSERT_EQ(0, num_invalid_neighbors);
}

TEST_F(VoronoiTest, issue_missing_border_edges)
{
    jcv_point points[] = {
        {1.5, 1.5},
        {0.5, 1.0},
        {1.5, 0.5},
    };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );
    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    const jcv_site* site = &sites[1];
    ASSERT_EQ( site->index, 1); // Make sure we test the correct one
    ASSERT_EQ(1, is_closed_loop(&ctx->diagram, site));
    int edge_count = 0;
    test_graphedge_iter graph_iter;
    test_site_get_edges(&ctx->diagram, site, &graph_iter);
    while( test_graphedge_next(&graph_iter) )
        ++edge_count;
    ASSERT_EQ(5, edge_count);
}

TEST_F(VoronoiTest, issue68_two_sites_have_all_clipping_edges)
{
#if defined(TEST_USE_DOUBLE)
    const jcv_point points[] = {
        {888.19238281250000, 377.82843017578125},
        {914.00000000000000, 341.00000000000000},
    };
    const int expected_edge_counts[] = {5, 3};
#else
    const jcv_point points[] = {
        {883.382263f, 340.749908f},
        {850.622253f, 378.323486f},
    };
    const int expected_edge_counts[] = {3, 5};
#endif
    const int num_points = (int)(sizeof(points) / sizeof(points[0]));
    const jcv_rect rect = {{600, 250}, {1000, 650}};

    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);
    ASSERT_EQ(num_points, ctx->diagram.numsites);

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    bool seen[] = {false, false};
    for( int i = 0; i < ctx->diagram.numsites; ++i )
    {
        int point_index = -1;
        for( int j = 0; j < num_points; ++j )
        {
            if( check_point_eq(&points[j], &sites[i].p) )
            {
                point_index = j;
                break;
            }
        }
        ASSERT_NE(-1, point_index);
        ASSERT_FALSE(seen[point_index]);
        seen[point_index] = true;
        ASSERT_EQ(1, is_closed_loop(&ctx->diagram, &sites[i]));

        int edge_count = 0;
        test_graphedge_iter graph_iter;
        test_site_get_edges(&ctx->diagram, &sites[i], &graph_iter);
        while( test_graphedge_next(&graph_iter) )
            ++edge_count;
        ASSERT_EQ(expected_edge_counts[point_index], edge_count);
    }
}

TEST_F(VoronoiTest, issue47_no_invalid_edges_span_clipping_rect)
{
    const jcv_point points[] = {
        {1205.64417f, 0.224472046f},
        {190.131897f, 0.285560131f},
        {1955.71814f, 0.0870857239f},
        {128.955994f, 0.344024658f},
    };
    const int num_points = (int)(sizeof(points) / sizeof(points[0]));
    jcv_rect rect = {{0, 0}, {2048, 2048}};

    jcv_diagram_generate(num_points, points, &rect, 0, &ctx->diagram);
    ASSERT_EQ(num_points, ctx->diagram.numsites);

    // Issue #47 produced geometrically invalid internal edges running from
    // y=0 to y=2048 across the entire clipping rectangle.
    int num_spanning_edges = 0;
    int num_invalid_spanning_edges = 0;
    test_edge_iter edge_iter;
    test_diagram_get_edges(&ctx->diagram, &edge_iter);
    for( const jcv_edge* edge = test_edge_next(&edge_iter); edge; edge = test_edge_next(&edge_iter) )
    {
        const int spans_height =
            (edge->pos[0].y == rect.min.y && edge->pos[1].y == rect.max.y) ||
            (edge->pos[1].y == rect.min.y && edge->pos[0].y == rect.max.y);
        if( !spans_height || !edge->sites[0] || !edge->sites[1] )
            continue;

        ++num_spanning_edges;
        const double xmid = ((double)edge->pos[0].x + (double)edge->pos[1].x) * 0.5;
        const double ymid = ((double)edge->pos[0].y + (double)edge->pos[1].y) * 0.5;
        const double dx = xmid - (double)edge->sites[0]->p.x;
        const double dy = ymid - (double)edge->sites[0]->p.y;
        const double site_distance_sq = dx * dx + dy * dy;
        const double tolerance = site_distance_sq * 1.0e-4 + 1.0e-4;
        for( int i = 0; i < num_points; ++i )
        {
            const double other_dx = xmid - (double)points[i].x;
            const double other_dy = ymid - (double)points[i].y;
            const double other_distance_sq = other_dx * other_dx + other_dy * other_dy;
            if( other_distance_sq + tolerance < site_distance_sq )
            {
                ++num_invalid_spanning_edges;
                break;
            }
        }
    }

    ASSERT_EQ(3, num_spanning_edges);
    ASSERT_EQ(0, num_invalid_spanning_edges);
}

TEST_F(VoronoiTest, issue48_frontier_performance_pattern)
{
    const int pair_count = 49999;
    const int num_points = pair_count * 2;
    jcv_point* points = new jcv_point[num_points];
    for (int i = 0; i < pair_count; ++i)
    {
        jcv_real value = (jcv_real)(i + 1);
        points[i * 2].x = value;
        points[i * 2].y = -value;
        points[i * 2 + 1].x = -value;
        points[i * 2 + 1].y = -value;
    }

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ(num_points, ctx->diagram.numsites);
    ASSERT_EQ(0, validate_vertex_indices(&ctx->diagram));

    const jcv_site* sites = jcv_diagram_get_sites(&ctx->diagram);
    jcv_real tolerance = (ctx->diagram.max.x - ctx->diagram.min.x) * (jcv_real)0.00001;
    int valid_cells = 0;
    int closed_cells = 0;
    for (int i = 0; i < ctx->diagram.numsites; ++i)
    {
        test_graphedge_iter graph_iter;
        test_site_get_edges(&ctx->diagram, &sites[i], &graph_iter);
        const jcv_edge* edge = test_graphedge_next(&graph_iter);
        int valid = edge != 0;
        closed_cells += edge != 0 && is_closed_loop(&ctx->diagram, &sites[i]);
        while (valid && edge)
        {
            for (int point_index = 0; point_index < 2; ++point_index)
            {
                const jcv_point* point = test_graphedge_get_position(&ctx->diagram, edge, point_index);
                valid = valid && isfinite((double)point->x) && isfinite((double)point->y);
                valid = valid && point->x >= ctx->diagram.min.x - tolerance;
                valid = valid && point->x <= ctx->diagram.max.x + tolerance;
                valid = valid && point->y >= ctx->diagram.min.y - tolerance;
                valid = valid && point->y <= ctx->diagram.max.y + tolerance;
            }
            edge = test_graphedge_next(&graph_iter);
        }
        valid_cells += valid;
    }
    ASSERT_EQ(num_points, valid_cells);
    // The original algorithm leaves a handful of numerically degenerate cells
    // open for this very large coordinate range (and thousands on master).
    ASSERT_GE(closed_cells, num_points - 16);

    delete[] points;
}


TEST_F(VoronoiTest, Delauney)
{
    jcv_point points[] = {
        {1.5, 1.5},
        {0.5, 1.0},
        {1.5, 0.5},
    };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );

    jcv_delauney_iter iter;
    jcv_delauney_begin( &ctx->diagram, &iter );
    jcv_delauney_edge delauney_edge;

    bool seen[3][3] = {};
    int count = 0;
    while (jcv_delauney_next( &iter, &delauney_edge ))
    {
        int sitea = delauney_edge.sites[0]->index;
        int siteb = delauney_edge.sites[1]->index;

        ASSERT_NE(sitea, siteb);
        ASSERT_FALSE(seen[sitea][siteb] || seen[siteb][sitea]);
        seen[sitea][siteb] = true;

        ASSERT_EQ(points[sitea].x, delauney_edge.pos[0].x);
        ASSERT_EQ(points[sitea].y, delauney_edge.pos[0].y);

        ASSERT_EQ(points[siteb].x, delauney_edge.pos[1].x);
        ASSERT_EQ(points[siteb].y, delauney_edge.pos[1].y);

        count++;
    }
    ASSERT_EQ(3, count);
    ASSERT_TRUE(seen[0][1] || seen[1][0]);
    ASSERT_TRUE(seen[0][2] || seen[2][0]);
    ASSERT_TRUE(seen[1][2] || seen[2][1]);
}

TEST_F(VoronoiTest, Delauney_edge_remains_valid_after_next)
{
    jcv_point points[] = {
        {1.5, 1.5},
        {0.5, 1.0},
        {1.5, 0.5},
    };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, 0, &ctx->diagram);

    jcv_delauney_iter iter;
    jcv_delauney_begin(&ctx->diagram, &iter);

    jcv_delauney_edge first;
    ASSERT_TRUE(jcv_delauney_next(&iter, &first));
    const jcv_edge* retained_edge = &first.edge;
    const jcv_site* retained_sites[2] = {
        retained_edge->sites[0],
        retained_edge->sites[1],
    };

    jcv_delauney_edge second;
    ASSERT_TRUE(jcv_delauney_next(&iter, &second));

    ASSERT_TRUE(retained_edge->sites[0] == retained_sites[0] &&
                retained_edge->sites[1] == retained_sites[1]);
}
