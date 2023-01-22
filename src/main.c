/*
 * A simple test program to display the output of the voronoi generator

VERSION
    0.2     2017-04-16  - Added support for reading .csv files
    0.1                 - Initial version

 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h> // printf
#include <ctype.h> // isascii

// I wrapped it in a library because it spams too many warnings
extern int wrap_stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);


#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

#define JC_VORONOI_CLIP_IMPLEMENTATION
#include "jc_voronoi_clip.h"

#ifdef HAS_MODE_FASTJET
#include <vector>
#include "../test/fastjet/voronoi.h"
#endif

static void plot(int x, int y, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
{
    if( x < 0 || y < 0 || x > (width-1) || y > (height-1) )
        return;
    int index = y * width * nchannels + x * nchannels;
    for( int i = 0; i < nchannels; ++i )
    {
        image[index+i] = color[i];
    }
}

// http://members.chello.at/~easyfilter/bresenham.html
static void draw_line(int x0, int y0, int x1, int y1, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
{
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2; // error value e_xy

    for(;;)
    {  // loop
        plot(x0,y0, image, width, height, nchannels, color);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
        if (e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
    }
}

// http://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
static inline int orient2d(const jcv_point* a, const jcv_point* b, const jcv_point* c)
{
    return ((int)b->x - (int)a->x)*((int)c->y - (int)a->y) - ((int)b->y - (int)a->y)*((int)c->x - (int)a->x);
}

static inline int min2(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int max2(int a, int b)
{
    return (a > b) ? a : b;
}

static inline int min3(int a, int b, int c)
{
    return min2(a, min2(b, c));
}
static inline int max3(int a, int b, int c)
{
    return max2(a, max2(b, c));
}

static void draw_triangle(const jcv_point* v0, const jcv_point* v1, const jcv_point* v2, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
{
    int area = orient2d(v0, v1, v2);
    if( area == 0 )
        return;

    // Compute triangle bounding box
    int minX = min3((int)v0->x, (int)v1->x, (int)v2->x);
    int minY = min3((int)v0->y, (int)v1->y, (int)v2->y);
    int maxX = max3((int)v0->x, (int)v1->x, (int)v2->x);
    int maxY = max3((int)v0->y, (int)v1->y, (int)v2->y);

    // Clip against screen bounds
    minX = max2(minX, 0);
    minY = max2(minY, 0);
    maxX = min2(maxX, width - 1);
    maxY = min2(maxY, height - 1);

    // Rasterize
    jcv_point p;
    for (p.y = (jcv_real)minY; p.y <= (jcv_real)maxY; p.y++) {
        for (p.x = (jcv_real)minX; p.x <= (jcv_real)maxX; p.x++) {
            // Determine barycentric coordinates
            int w0 = orient2d(v1, v2, &p);
            int w1 = orient2d(v2, v0, &p);
            int w2 = orient2d(v0, v1, &p);

            // If p is on or inside all edges, render pixel.
            if (w0 >= 0 && w1 >= 0 && w2 >= 0)
            {
                plot((int)p.x, (int)p.y, image, width, height, nchannels, color);
            }
        }
    }
}

static void relax_points(const jcv_diagram* diagram, jcv_point* points)
{
    const jcv_site* sites = jcv_diagram_get_sites(diagram);
    for( int i = 0; i < diagram->numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_point sum = site->p;
        int count = 1;

        const jcv_graphedge* edge = site->edges;

        while( edge )
        {
            sum.x += edge->pos[0].x;
            sum.y += edge->pos[0].y;
            ++count;
            edge = edge->next;
        }

        points[site->index].x = sum.x / (jcv_real)count;
        points[site->index].y = sum.y / (jcv_real)count;
    }
}

static void Usage()
{
    printf("Usage: main [options]\n");
    printf("\t-n <num points>\n");
    printf("\t-r <num relaxations>\n");
    printf("\t-i <inputfile>\t\tA list of 2-tuples (float, float) representing 2-d coordinates\n");
    printf("\t-o <outputfile.png>\n");
    printf("\t-w <width>\n");
    printf("\t-h <height>\n");
}

static int debug_skip_point(const jcv_point* pt)
{
    (void)pt;
    // int edge = 20;
    // if (pt->x > edge && pt->x < 2048-edge && pt->y > edge && pt->y < 2048-edge)
    //     return 1;

    // if (pt->y > edge && pt->y < 2048-edge)
    //     return 1;

    return 0;
}

static inline int is_ascii(const char* chars, size_t len)
{
    for( size_t i = 0; i < len; ++i )
    {
        if (!isascii((int)chars[i]))
            return 0;
    }
    return 1;
}

static inline int is_text(FILE* file, int len)
{
    char* buffer = (char*)malloc((size_t)len);
    size_t nread = fread(buffer, 1, (size_t)len, file);
    fseek(file, 0, SEEK_SET);
    int result = is_ascii(buffer, nread);
    free(buffer);
    return result;
}

static int read_input_csv(FILE* file, jcv_point** points, uint32_t* length, jcv_rect** rect)
{
    jcv_point* pts = 0;
    uint32_t capacity = 0;
    uint32_t len = 0;
    char buffer[64];

    while( !feof(file) )
    {
        fgets(buffer, sizeof(buffer), file);

        jcv_point pt1;
        jcv_point pt2;
        int numscanned = sscanf(buffer, "%f %f %f %f\n", &pt1.x, &pt1.y, &pt2.x, &pt2.y);

        if( numscanned == 4 )
        {
            if (rect)
            {
                *rect = malloc(sizeof(jcv_rect));
                (*rect)->min = pt1;
                (*rect)->max = pt2;
            }
        }
        else if( numscanned == 2 )
        {
            if( debug_skip_point(&pt1) )
            {
                continue;
            }
            if( capacity < (len + 1))
            {
                capacity += 1024;
                pts = (jcv_point*)realloc(pts, sizeof(jcv_point) * capacity);
            }

            pts[len].x = pt1.x;
            pts[len].y = pt1.y;
            ++len;
        }
        else
        {
            fprintf(stderr, "Failed to read point on line %u: %d '%s'\n", len, numscanned, buffer);
            return 1;
        }
    }

    *points = pts;
    *length = len;
    return 0;
}

static int read_input(const char* path, jcv_point** points, uint32_t* length, jcv_rect** rect)
{
    if( !path )
    {
        return 1;
    }

    FILE* file = 0;
    if( strcmp(path, "-") == 0 )
        file = stdin;
    else
        file = fopen(path, "rb");

    if( !file )
    {
        fprintf(stderr, "Failed to open %s for reading\n", path);
        *length = 0;
        return 1;
    }

    int result = 0;
    if (is_text(file, 64))
    {
        result = read_input_csv(file, points, length, rect);
        goto end;
    }

    uint32_t capacity = 0;
    uint32_t len = 0;
    jcv_point* pts = 0;

    char buffer[64];
    uint32_t bufferoffset = 0;

    while( !feof(file) )
    {
        size_t num_read = fread((void*)&buffer[bufferoffset], 1, sizeof(buffer) - bufferoffset, file);
        num_read += bufferoffset;

        uint32_t num_points = (uint32_t) num_read / sizeof(jcv_point);
        if( capacity < (len + num_points))
        {
            capacity += 1024;
            pts = (jcv_point*)realloc(pts, sizeof(jcv_point) * capacity);
        }
        for( uint32_t i = 0; i < num_points; ++i )
        {
            jcv_point* pt = &((jcv_point*)buffer)[i];
            if( debug_skip_point(pt) )
            {
                continue;
            }
            pts[len].x = pt->x;
            pts[len].y = pt->y;
            ++len;
        }
        bufferoffset = (uint32_t) num_read - num_points * sizeof(jcv_point);
        memmove(buffer, &buffer[num_points * sizeof(jcv_point)], bufferoffset);

        *points = pts;
        *length = len;
    }

end:
    printf("Read %d points from %s\n", *length, path);

    if( strcmp(path, "-") != 0 )
        fclose(file);
    return result;
}

// Remaps the point from the input space to image space
static inline jcv_point remap(const jcv_point* pt, const jcv_point* min, const jcv_point* max, const jcv_point* scale)
{
    jcv_point p;
    p.x = (pt->x - min->x)/(max->x - min->x) * scale->x;
    p.y = (pt->y - min->y)/(max->y - min->y) * scale->y;
    return p;
}

int main(int argc, const char** argv)
{
    // Number of sites to generate
    int count = 200;
    // Image dimension
    int width = 512;
    int height = 512;
    int numrelaxations = 0;
    const char* inputfile = 0;
    const char* clipfile = 0; // a file with clipping points
    const char* outputfile = "example.png";

    if( argc == 1 )
    {
        Usage();
        return 1;
    }

    for( int i = 1; i < argc; ++i )
    {
        if(strcmp(argv[i], "-i") == 0)
        {
            if( i+1 < argc )
                inputfile = argv[i+1];
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-o") == 0)
        {
            if( i+1 < argc )
                outputfile = argv[i+1];
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-n") == 0)
        {
            if( i+1 < argc )
                count = (int)atol(argv[i+1]);
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-w") == 0)
        {
            if( i+1 < argc )
                width = (int)atol(argv[i+1]);
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-h") == 0)
        {
            if( i+1 < argc )
                height = (int)atol(argv[i+1]);
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            if( i+1 < argc )
                numrelaxations = (int)atol(argv[i+1]);
            else
            {
                Usage();
                return 1;
            }
        }
        if(strcmp(argv[i], "-c") == 0)
        {
            if( i+1 < argc )
                clipfile = argv[i+1];
            else
            {
                Usage();
                return 1;
            }
        }
        else if(strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0)
        {
            Usage();
            return 0;
        }
    }

    jcv_point* points = 0;
    jcv_rect* rect = 0;

    if( inputfile )
    {
        if( read_input(inputfile, &points, (uint32_t*)&count, &rect) )
        {
            fprintf(stderr, "Failed to read from %s\n", inputfile);
            return 1;
        }
    }
    else
    {
        points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)count);
        if( !points )
            return 1;

        int pointoffset = 10; // move the points inwards, for aestetic reasons

        srand(0);

        for( int i = 0; i < count; ++i )
        {
            points[i].x = (float)(pointoffset + rand() % (width-2*pointoffset));
            points[i].y = (float)(pointoffset + rand() % (height-2*pointoffset));
        }
    }


    jcv_point* clippoints = 0;
    int clipcount = 0;
    if( clipfile )
    {
        if( read_input(clipfile, &clippoints, (uint32_t*)&clipcount, 0) )
        {
            fprintf(stderr, "Failed to read from %s\n", clipfile);
            return 1;
        }
    }

    printf("Width/Height is %d, %d\n", width, height);
    printf("Count is %d, num relaxations is %d\n", count, numrelaxations);


    jcv_clipping_polygon polygon;
    jcv_clipper* clipper = 0;
    if (clippoints)
    {

        printf("Clip polygon '%s' used\n", clipfile);
        polygon.num_points = clipcount;
        polygon.points = clippoints;

        jcv_clipper polygonclipper;
        polygonclipper.test_fn = jcv_clip_polygon_test_point;
        polygonclipper.clip_fn = jcv_clip_polygon_clip_edge;
        polygonclipper.fill_fn = jcv_clip_polygon_fill_gaps;
        polygonclipper.ctx = &polygon;

        clipper = &polygonclipper;
    } else {
        polygon.num_points = 0;
        polygon.points = 0;
    }

    for( int i = 0; i < numrelaxations; ++i )
    {
        jcv_diagram diagram;
        memset(&diagram, 0, sizeof(jcv_diagram));
        jcv_diagram_generate(count, (const jcv_point*)points, rect, clipper, &diagram);

        relax_points(&diagram, points);

        jcv_diagram_free( &diagram );
    }

    size_t imagesize = (size_t)(width*height*3);
    unsigned char* image = (unsigned char*)malloc(imagesize);
    memset(image, 0, imagesize);

    unsigned char color_pt[] = {255, 255, 255};
    unsigned char color_line[] = {220, 220, 220};
    unsigned char color_delauney[] = {64, 64, 255};

    jcv_diagram diagram;
    jcv_point dimensions;
    dimensions.x = (jcv_real)width;
    dimensions.y = (jcv_real)height;
    {
        printf("Generating...\n");
        memset(&diagram, 0, sizeof(jcv_diagram));
        jcv_diagram_generate(count, (const jcv_point*)points, rect, clipper, &diagram);
        printf("Done.\n");

        printf("Rendering...\n");

        // If you want to draw triangles, or relax the diagram,
        // you can iterate over the sites and get all edges easily

        //if (0)
        {
            const jcv_site* sites = jcv_diagram_get_sites( &diagram );
            for( int i = 0; i < diagram.numsites; ++i )
            {
                const jcv_site* site = &sites[i];

                srand((unsigned int)site->index); // for generating colors for the triangles

                unsigned char color_tri[3];
                unsigned char basecolor = 120;
                color_tri[0] = basecolor + (unsigned char)(rand() % (235 - basecolor));
                color_tri[1] = basecolor + (unsigned char)(rand() % (235 - basecolor));
                color_tri[2] = basecolor + (unsigned char)(rand() % (235 - basecolor));

                jcv_point s = remap(&site->p, &diagram.min, &diagram.max, &dimensions );

                const jcv_graphedge* e = site->edges;
                while( e )
                {
                    jcv_point p0 = remap(&e->pos[0], &diagram.min, &diagram.max, &dimensions );
                    jcv_point p1 = remap(&e->pos[1], &diagram.min, &diagram.max, &dimensions );

                    draw_triangle( &s, &p0, &p1, image, width, height, 3, color_tri);
                    e = e->next;
                }
            }
        }

        // If all you need are the edges
        const jcv_edge* edge = jcv_diagram_get_edges( &diagram );
        while( edge )
        {
            jcv_point p0 = remap(&edge->pos[0], &diagram.min, &diagram.max, &dimensions );
            jcv_point p1 = remap(&edge->pos[1], &diagram.min, &diagram.max, &dimensions );
            draw_line((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, image, width, height, 3, color_line);
            edge = jcv_diagram_get_next_edge(edge);
        }

        jcv_delauney_iter delauney;
        jcv_delauney_begin( &diagram, &delauney );
        jcv_delauney_edge delauney_edge;
        while (jcv_delauney_next( &delauney, &delauney_edge ))
        {
            jcv_point p0 = remap(&delauney_edge.pos[0], &diagram.min, &diagram.max, &dimensions );
            jcv_point p1 = remap(&delauney_edge.pos[1], &diagram.min, &diagram.max, &dimensions );
            draw_line((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, image, width, height, 3, color_delauney);
        }
        printf("Done.\n"); // rendering

        jcv_diagram_free( &diagram );
    }

    // draw the clipping polygon
    for (int i = 0; i < polygon.num_points; ++i)
    {
        jcv_point p0 = remap(&polygon.points[i], &diagram.min, &diagram.max, &dimensions );
        jcv_point p1 = remap(&polygon.points[(i+1)%polygon.num_points], &diagram.min, &diagram.max, &dimensions );
        draw_line((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, image, width, height, 3, color_line);
    }

    // Plot the sites
    for( int i = 0; i < count; ++i )
    {
        if (clipper && !clipper->test_fn(clipper, points[i]))
            continue;
        jcv_point p = remap(&points[i], &diagram.min, &diagram.max, &dimensions );
        plot((int)p.x, (int)p.y, image, width, height, 3, color_pt);
    }

    free(clippoints);
    free(points);
    free(rect);

    // flip image
    int stride = width*3;
    uint8_t* row = (uint8_t*)malloc((size_t)stride);
    for( int y = 0; y < height/2; ++y )
    {
        memcpy(row, &image[y*stride], (size_t)stride);
        memcpy(&image[y*stride], &image[(height-1-y)*stride], (size_t)stride);
        memcpy(&image[(height-1-y)*stride], row, (size_t)stride);
    }

    char path[512];
    sprintf(path, "%s", outputfile);
    printf("Writing %s\n", path);

    wrap_stbi_write_png(path, width, height, 3, image, stride);
    printf("Done.\n");

    free(image);

    return 0;
}
