/*
 * A simple test program to display the output of the voronoi generator
 */

#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define JC_VORONOI_IMPLEMENTATION
// If you wish to use doubles
//#define JCV_REAL_TYPE double
//#define JCV_FABS fabs
#include "jc_voronoi.h"

#include <vector>

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
static inline int orient2d(const jcv_point& a, const jcv_point& b, const jcv_point& c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

static inline int min3(int a, int b, int c)
{
	return std::min(a, std::min(b, c));
}
static inline int max3(int a, int b, int c)
{
	return std::max(a, std::max(b, c));
}

static void draw_triangle(const jcv_point& v0, const jcv_point& v1, const jcv_point& v2, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
{
    float area = orient2d(v0, v1, v2);
    if( area == 0 )
        return;

    // Compute triangle bounding box
    int minX = min3(v0.x, v1.x, v2.x);
    int minY = min3(v0.y, v1.y, v2.y);
    int maxX = max3(v0.x, v1.x, v2.x);
    int maxY = max3(v0.y, v1.y, v2.y);

    // Clip against screen bounds
    minX = std::max(minX, 0);
    minY = std::max(minY, 0);
    maxX = std::min(maxX, width - 1);
    maxY = std::min(maxY, height - 1);

    // Rasterize
    jcv_point p;
    for (p.y = minY; p.y <= maxY; p.y++) {
        for (p.x = minX; p.x <= maxX; p.x++) {
            // Determine barycentric coordinates
            int w0 = orient2d(v1, v2, p);
            int w1 = orient2d(v2, v0, p);
            int w2 = orient2d(v0, v1, p);

            // If p is on or inside all edges, render pixel.
            if (w0 >= 0 && w1 >= 0 && w2 >= 0)
            {
                plot(p.x, p.y, image, width, height, nchannels, color);
            }
        }
    }
}

static void Usage()
{
	printf("Usage: main [options]\n");
	printf("\t-n <num points>\n");
	printf("\t-i <input>\t\tA list of 2-tuples (float, float) representing 2-d coordinates\n");
	printf("\t-w <width>\n");
	printf("\t-h <height>\n");
}

int main(int argc, const char** argv)
{
	// Number of sites to generate
	int count = 200;
	// Image dimension
	int width = 512;
	int height = 512;
	int mode = 0;
	const char* inputfile = 0;

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
		else if(strcmp(argv[i], "-n") == 0)
		{
			if( i+1 < argc )
				count = atol(argv[i+1]);
			else
			{
				Usage();
				return 1;
			}
		}
		else if(strcmp(argv[i], "-w") == 0)
		{
			if( i+1 < argc )
				width = atol(argv[i+1]);
			else
			{
				Usage();
				return 1;
			}
		}
		else if(strcmp(argv[i], "-h") == 0)
		{
			if( i+1 < argc )
				height = atol(argv[i+1]);
			else
			{
				Usage();
				return 1;
			}
		}
		else if(strcmp(argv[i], "-m") == 0)
		{
			if( i+1 < argc )
				mode = atol(argv[i+1]);
			else
			{
				Usage();
				return 1;
			}
		}
	}

	FILE* file = 0;
	if( inputfile )
	{
		if( strcmp(inputfile, "-") == 0 )
			file = stdin;
		file = fopen(inputfile, "rb");
		if( !file )
		{
			fprintf(stderr, "Failed to open %s for reading\n", inputfile);
			return 1;
		}

		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		count = size / sizeof(jcv_point);
	}

	jcv_point* points = new jcv_point[count];
	if( !points )
		return 1;

	if( inputfile )
	{
		int nread = fread(points, count * sizeof(jcv_point), 1, file);
		if( nread != 1 )
		{
			fprintf(stderr, "Failed to read %lu bytes from %s\n", count * sizeof(jcv_point), inputfile);
			fclose(file);
			delete[] points;
			return 1;
		}

		printf("Read %d points from %s\n", count, inputfile);
	}
	else
	{
		int pointoffset = 10; // move the points inwards, for aestetic reasons

		srand(0);

		for( int i = 0; i < count; ++i )
		{
			points[i].x = float(pointoffset + rand() % (width-2*pointoffset));
			points[i].y = float(pointoffset + rand() % (height-2*pointoffset));
		}
	}

	printf("Width/Height is %d, %d\n", width, height);
	printf("Count is %d\n", count);

	size_t imagesize = width*height*3;
	unsigned char* image = (unsigned char*)malloc(imagesize);
	memset(image, 0, imagesize);

	unsigned char color_pt[] = {255, 255, 255};
	unsigned char color_line[] = {255, 255, 255};
	unsigned char color_line2[] = {127, 127, 255};

	jcv_diagram diagram = { 0 };
	jcv_diagram_generate(count, (const jcv_point*)points, width, height, &diagram );

	srand(count); // for generating colors for the triangles

	// If you want to draw triangles, or relax the diagram,
	// you can iterate over the sites and get all edges easily
	const struct jcv_site* sites = jcv_diagram_get_sites( &diagram );
	for( int i = 0; i < count; ++i )
	{
		const struct jcv_site& site = sites[i];

		unsigned char color_tri[3];
		int basecolor = 120;
		color_tri[0] = basecolor + rand() % (235 - basecolor);
		color_tri[1] = basecolor + rand() % (235 - basecolor);
		color_tri[2] = basecolor + rand() % (235 - basecolor);

		const struct jcv_graphedge* e = site.edges;
		while( e )
		{
			draw_triangle( site.p, e->pos[0], e->pos[1], image, width, height, 3, color_tri);
			e = e->next;
		}
	}

	// If all you need are the edges
	const struct jcv_edge* edge = jcv_diagram_get_edges( &diagram );
	while( edge )
	{
		draw_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image, width, height, 3, color_line);
		edge = edge->next;
	}

	jcv_diagram_free( &diagram );

	// Plot the sites
	for( int i = 0; i < count; ++i )
	{
		plot(points[i].x, points[i].y, image, width, height, 3, color_pt);
	}

	delete[] points;

	char path[512];
	sprintf(path, "example.png");
	stbi_write_png(path, width, height, 3, image, width*3);
	printf("wrote %s\n", path);

	free(image);

	return 0;
}
