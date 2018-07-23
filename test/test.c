#define JC_TEST_IMPLEMENTATION
#include "jc_test.h"

#include <memory.h>

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

#include <stdint.h>

#define IMAGE_SIZE 512

typedef struct Context
{
    int             width;
    jcv_diagram     diagram;
} Context;

static Context* voronoi_main_setup()
{
    return (Context*) malloc( sizeof(Context) );
}

static void voronoi_main_teardown(Context* ctx)
{
    (void)ctx;
}

static void test_setup(Context* ctx)
{
    ctx->width  = IMAGE_SIZE;
    memset(&ctx->diagram, 0, sizeof(ctx->diagram));
}

static void test_teardown(Context* ctx)
{
    jcv_diagram_free(&ctx->diagram);
} 

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
        printf("  E: %f, %f -> %f, %f   neigh: %p\n", (double)e->pos[0].x, (double)e->pos[0].y, (double)e->pos[1].x, (double)e->pos[1].y, (void*)e->neighbor);
        e = e->next;
    }
}

static bool check_point_eq(const jcv_point* p1, const jcv_point* p2)
{
    return p1->x == p2->x && p1->y == p2->y;
}

static bool check_edge_eq(const jcv_graphedge* e, const jcv_point* p1, const jcv_point* p2)
{
    return check_point_eq(&e->pos[0], p1) && check_point_eq(&e->pos[1], p2);
}

#define ASSERT_POINT_EQ( _P1, _P2 ) \
    ASSERT_EQ( (_P1).x, (_P2).x ) \
    ASSERT_EQ( (_P1).y, (_P2).y ) \

static void check_edges(const jcv_graphedge* edges, int num_expected,
                        const jcv_point* expected_points, const jcv_site** expected_neighbors)
{
    int num_matched = 0;
    for( int i = 0; i < num_expected; ++i )
    {
        const jcv_graphedge* e = edges;
        while( e )
        {
            if( check_edge_eq(e, &expected_points[i], &expected_points[(i+1)%num_expected]) )
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

static void voronoi_test_parallel_horiz_2(Context* ctx)
{
    jcv_point points[] = { {IMAGE_SIZE/4, IMAGE_SIZE/2}, {(IMAGE_SIZE*3)/4, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

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


static void voronoi_test_parallel_vert_2(Context* ctx)
{
    jcv_point points[] = { {IMAGE_SIZE/2, (IMAGE_SIZE*1)/4}, {IMAGE_SIZE/2, (IMAGE_SIZE*3)/4} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

    ASSERT_EQ( 2, ctx->diagram.numsites );

    const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    ASSERT_POINT_EQ( points[0], sites[0].p );
    ASSERT_POINT_EQ( points[1], sites[1].p );
    printf("in vert_2\n");
}


static void voronoi_test_one_site(Context* ctx)
{
    printf("In one_site1\n");
    jcv_point points[] = { {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    printf("Before num_points\n");
    int num_points = sizeof(points) / sizeof(points[0]);
    printf("After num_points\n");

    //jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

    //ASSERT_EQ( 1, ctx->diagram.numsites );


    //jcv_point expected_edges_0[4];
    //expected_edges_0[0].x = ctx->diagram.min.x;
    //expected_edges_0[0].y = ctx->diagram.min.y;
    //expected_edges_0[1].x = ctx->diagram.max.x;
    //expected_edges_0[1].y = ctx->diagram.min.y;
    //expected_edges_0[2].x = ctx->diagram.max.x;
    //expected_edges_0[2].y = ctx->diagram.max.y;
    //expected_edges_0[3].x = ctx->diagram.min.x;
    //expected_edges_0[3].y = ctx->diagram.max.y;
    //const jcv_site* expected_neighbors_0[4];
    //expected_neighbors_0[0] = 0;
    //expected_neighbors_0[1] = 0;
    //expected_neighbors_0[2] = 0;
    //expected_neighbors_0[3] = 0;

    //const jcv_site* sites = jcv_diagram_get_sites( &ctx->diagram );
    //check_edges( sites[0].edges, 4, expected_edges_0, expected_neighbors_0 );
}

static void voronoi_test_culling(Context* ctx)
{
    jcv_point points[] = { {IMAGE_SIZE/2, -IMAGE_SIZE/2}, {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_rect rect = { {0, 0}, {IMAGE_SIZE, IMAGE_SIZE} };
    jcv_diagram_generate(num_points, points, &rect, &ctx->diagram);

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

static void voronoi_test_same_site(Context* ctx)
{
    jcv_point points[] = { {IMAGE_SIZE/2, IMAGE_SIZE/2}, {IMAGE_SIZE/2, IMAGE_SIZE/2} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

    ASSERT_EQ( 1, ctx->diagram.numsites );
}

static void voronoi_test_many(Context* ctx)
{
    srand(0);
    
    printf("In test_many\n");

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

        jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

        //ASSERT_EQ( num_points, ctx->diagram.numsites );

        if( count < maxcount-1 )
        {
            jcv_diagram_free( &ctx->diagram );
            memset(&ctx->diagram, 0, sizeof(jcv_diagram));
        }
    }
}

static void voronoi_test_many_diagonal(Context* ctx)
{
    const int num_points = 1000;
    jcv_point* points = (jcv_point*)malloc( sizeof(jcv_point) * num_points );

    for( int i = 0; i < num_points; ++i )
    {
        points[i].x = i / (float)num_points;
        points[i].y = points[i].x;
    }

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

    ASSERT_EQ( num_points, ctx->diagram.numsites );
}

// Testing a large event queue (https://github.com/JCash/voronoi/issues/3)
static void voronoi_test_many_circle(Context* ctx)
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

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);

    ASSERT_EQ( num_points, ctx->diagram.numsites );
}

static void voronoi_test_crash1(Context* ctx)
{
    jcv_point points[] = { {-0.148119405f, 0.307878017f}, {-0.0949054062f, -0.37929377f}, {0.170877606f, 0.477409601f}, {-0.0634334087f, 0.0787638053f}, {-0.244908407f, 0.402904421f}, {-0.0830767974f, 0.442425013f} };
    int num_points = (int)(sizeof(points) / sizeof(jcv_point));

    jcv_diagram_generate(num_points, points, 0, &ctx->diagram);
    ASSERT_EQ( num_points, ctx->diagram.numsites );
}

TEST_BEGIN(voronoi_test, voronoi_main_setup, voronoi_main_teardown, test_setup, test_teardown)
    TEST(voronoi_test_parallel_horiz_2)
    TEST(voronoi_test_parallel_vert_2)
    TEST(voronoi_test_one_site)
    //TEST(voronoi_test_same_site)
    TEST(voronoi_test_many)
    //TEST(voronoi_test_many_diagonal)
    //TEST(voronoi_test_many_circle)
    //TEST(voronoi_test_culling)
    //TEST(voronoi_test_crash1)
TEST_END(voronoi_test)


int main(int argc, const char** argv)
{
    (void)argc;
    (void)argv;
    (void)debug_edges;
    (void)debug_points;
    
    RUN_ALL();
    return 0;
}
