#define JC_TEST_IMPLEMENTATION
#include "jc_test.h"


#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi_hest.h"

#include <stdint.h>

typedef struct Context
{
    int                 width;
    struct jcv_diagram* diagram;
} Context;

static Context* voronoi_main_setup()
{
    return reinterpret_cast<Context*>( malloc( sizeof(Context) ) );
}

static void voronoi_main_teardown(Context* ctx)
{
}

static void test_setup(Context* ctx)
{
    ctx->diagram    = 0;
    ctx->width      = 512;
}

static void test_teardown(Context* ctx)
{
    jcv_diagram_free(ctx->diagram);
}

static void voronoi_create(Context* ctx)
{
    jcv_point points[] = { {} };
    int num_points = sizeof(points) / sizeof(points[0]);

    jcv_diagram_generate( num_points, points, ctx->width, ctx->width, &ctx->diagram);

    ASSERT_TRUE( ctx->diagram != 0 );
}


TEST_BEGIN(voronoi_test, voronoi_main_setup, voronoi_main_teardown, test_setup, test_teardown)
    TEST(voronoi_create)
TEST_END(voronoi_test)


int main(int argc, const char** argv)
{
    (void)argc;
    (void)argv;
    
    RUN_ALL();
    return 0;
}