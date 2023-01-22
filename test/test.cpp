#define JC_TEST_USE_DEFAULT_MAIN // int main();
#include "jc_test.h"

#include <memory.h>

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

#include <stdint.h>

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

static void debug_points(int num, const jcv_point* points)
{
    printf("\nNUM POINTS: %d\n", num);
    for( int i = 0; i < num; ++i)
    {
        printf("  %d: %f, %f\n", i, (double)points[i].x, (double)points[i].y);
    }
    printf("\n");
}

static void debug_edges(const jcv_graphedge* e)
{
    while( e )
    {
        printf("  E: %f, %f -> %f, %f   neigh: %d\n", (double)e->pos[0].x, (double)e->pos[0].y, (double)e->pos[1].x, (double)e->pos[1].y, e->neighbor?e->neighbor->index:-1);
        e = e->next;
    }
}

static void debug_sites(int num, const jcv_site* sites)
{
    printf("\nNUM sites: %d\n", num);
    for( int i = 0; i < num; ++i)
    {
        const jcv_site* site = &sites[i];
        printf("%d: idx: %d %f, %f\n", i, site->index, (double)site->p.x, (double)site->p.y);
        debug_edges(site->edges);
    }
    printf("\n");
}

static bool check_point_eq(const jcv_point* a, const jcv_point* b)
{
    return a->x == b->x && a->y == b->y;
}

static bool check_graphedge_eq(const jcv_graphedge* e, const jcv_point* p1, const jcv_point* p2)
{
    return check_point_eq(&e->pos[0], p1) && check_point_eq(&e->pos[1], p2);
}

#define ASSERT_POINT_EQ( _P1, _P2 ) \
    ASSERT_EQ( (_P1).x, (_P2).x ); \
    ASSERT_EQ( (_P1).y, (_P2).y )

#define ASSERT_POINT_NE( _P1, _P2 ) \
    ASSERT_TRUE( !check_point_eq( &_P1, &_P2) )

static void check_edges(const jcv_graphedge* edges, int num_expected,
                        const jcv_point* expected_points, const jcv_site** expected_neighbors)
{
    int num_matched = 0;
    for( int i = 0; i < num_expected; ++i )
    {
        const jcv_graphedge* e = edges;
        while( e )
        {
            if( check_graphedge_eq(e, &expected_points[i], &expected_points[(i+1)%num_expected]) )
            {
                ASSERT_EQ( expected_neighbors[i], e->neighbor );
                num_matched++;
                break;
            }

            e = e->next;
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

    check_edges( sites[0].edges, 4, expected_edges_0, expected_neighbors_0 );
    check_edges( sites[1].edges, 4, expected_edges_1, expected_neighbors_1 );
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
    check_edges( sites[0].edges, 4, expected_edges_0, expected_neighbors_0 );
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
    check_edges( sites[0].edges, 4, expected_edges_0, expected_neighbors_0 );
}


static jcv_context_internal* setup_test_context_internal(int num_points, jcv_point* points, void* ctx)
{
    jcv_context_internal* internal = jcv_alloc_internal(num_points, ctx, jcv_alloc_fn, jcv_free_fn);
    internal->numsites = num_points;
    jcv_site* sites = internal->sites;

    for( int i = 0; i < num_points; ++i )
    {
        sites[i].p        = points[i];
        sites[i].edges    = 0;
        sites[i].index    = i;
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

    const jcv_edge* edge = jcv_diagram_get_edges( &ctx->diagram );
    while( edge )
    {
        ASSERT_POINT_NE(edge->pos[0], edge->pos[1]);
        edge = jcv_diagram_get_next_edge(edge);
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
        const jcv_graphedge* e = site->edges;
        int count = 0;
        while (e) {
            ++count;
            e = e->next;
        }
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
        const jcv_graphedge* e = site->edges;
        while (e) {
            // If it's a border edge
            if (e->neighbor == 0)
                ++count;
            // or if the neighbor has a higher index (i.e. only count the edge once)
            else if(e->neighbor->index > site->index)
                ++count;

            // 2. Make sure the graph edge points are the same as the edge points
            ASSERT_TRUE(e->edge != 0);
            bool eq =   check_graphedge_eq(e, &e->edge->pos[0], &e->edge->pos[1]) ||
                        check_graphedge_eq(e, &e->edge->pos[1], &e->edge->pos[0]);
            ASSERT_TRUE(eq);

            e = e->next;
        }
    }
    ASSERT_EQ( 10, count );

    // 3. count the edges
    int count_edges = 0;
    const jcv_edge* edge = jcv_diagram_get_edges(&ctx->diagram);
    while (edge) {
        ++count_edges;
        edge = jcv_diagram_get_next_edge(edge);
    }
    ASSERT_EQ( 10, count_edges );
}

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

// Checks if the points of all edges are connected
static inline int is_closed_loop(jcv_graphedge* edge)
{
    jcv_graphedge* first = edge;
    while (edge)
    {
        jcv_graphedge* next = edge->next ? edge->next : first;
        if (!jcv_point_eq(&edge->pos[1], &next->pos[0]))
            return 0;
        edge = edge->next;
    }
    return 1;
}

static void voronoi_testfn_closed_loop(Context* ctx)
{
    (void)ctx;
    jcv_point points[3] = {
        {1, 1},
        {3, 1},
        {2, 3},
    };

    jcv_graphedge edges[3];
    edges[0].pos[0] = points[0];
    edges[0].pos[1] = points[1];
    edges[1].pos[0] = points[1];
    edges[1].pos[1] = points[2];
    edges[2].pos[0] = points[2];
    edges[2].pos[1] = points[0];

    edges[0].next = &edges[1];
    edges[1].next = &edges[2];
    edges[2].next = 0;
    ASSERT_EQ(1, is_closed_loop(edges));

    edges[2].pos[1] = points[1];
    ASSERT_EQ(0, is_closed_loop(edges));
}

static inline int count_edges(jcv_graphedge* edge)
{
    int count = 0;
    jcv_graphedge* first = edge;
    while (edge)
    {
        ++count;
        edge = edge->next;
        if (edge == first)
            break;
    }
    return count;
}

TEST_F(VoronoiTest, fn_count_edges)
{
    jcv_graphedge edges[4];
    edges[0].next = &edges[1];
    edges[1].next = &edges[2];
    edges[2].next = 0;
    ASSERT_EQ(3, count_edges(edges));

    edges[2].next = &edges[0];
    ASSERT_EQ(3, count_edges(edges));

    edges[2].next = &edges[3];
    edges[3].next = &edges[0];
    ASSERT_EQ(4, count_edges(edges));
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
    ASSERT_EQ(1, is_closed_loop(site->edges));
    ASSERT_EQ(5, count_edges(site->edges));
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

    int expected_sites[] = {
        1, 0,
        2, 0,
        2, 1,
    };

    int count = 0;
    while (jcv_delauney_next( &iter, &delauney_edge ))
    {
        int sitea = delauney_edge.sites[0]->index;
        int siteb = delauney_edge.sites[1]->index;

        ASSERT_EQ(expected_sites[count*2+0], sitea);
        ASSERT_EQ(expected_sites[count*2+1], siteb);

        ASSERT_EQ(points[sitea].x, delauney_edge.pos[0].x);
        ASSERT_EQ(points[sitea].y, delauney_edge.pos[0].y);

        ASSERT_EQ(points[siteb].x, delauney_edge.pos[1].x);
        ASSERT_EQ(points[siteb].y, delauney_edge.pos[1].y);

        count++;
    }
    ASSERT_EQ(3, count);
}
