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
//#define JCV_ATAN2 atan2
#include "jc_voronoi.h"

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
    for (p.y = minY; p.y <= maxY; p.y++) {
        for (p.x = minX; p.x <= maxX; p.x++) {
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

		points[site->index].x = sum.x / count;
		points[site->index].y = sum.y / count;
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

int main(int argc, const char** argv)
{
	// Number of sites to generate
	int count = 200;
	// Image dimension
	int width = 512;
	int height = 512;
	int mode = 0;
	int numrelaxations = 0;
	const char* inputfile = 0;
	const char* outputfile = "example.png";

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
		else if(strcmp(argv[i], "-m") == 0)
		{
			if( i+1 < argc )
				mode = (int)atol(argv[i+1]);
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
		size_t size = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
		count = (int)(size / sizeof(jcv_point));
	}

	jcv_point* points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)count);
	if( !points )
		return 1;

	if( inputfile )
	{
		size_t nread = fread(points, (size_t)count * sizeof(jcv_point), 1, file);
		if( nread != 1 )
		{
			fprintf(stderr, "Failed to read %lu bytes from %s\n", (size_t)count * sizeof(jcv_point), inputfile);
			fclose(file);
			free(points);
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
			points[i].x = (float)(pointoffset + rand() % (width-2*pointoffset));
			points[i].y = (float)(pointoffset + rand() % (height-2*pointoffset));
		}
	}

	printf("Width/Height is %d, %d\n", width, height);
	printf("Count is %d, num relaxations is %d\n", count, numrelaxations);

	for( int i = 0; i < numrelaxations; ++i )
	{
		jcv_diagram diagram;
		memset(&diagram, 0, sizeof(jcv_diagram));
		jcv_diagram_generate(count, (const jcv_point*)points, width, height, &diagram );

		relax_points(&diagram, points);

		jcv_diagram_free( &diagram );
	}

	size_t imagesize = (size_t)(width*height*3);
	unsigned char* image = (unsigned char*)malloc(imagesize);
	memset(image, 0, imagesize);

	unsigned char color_pt[] = {255, 255, 255};
	unsigned char color_line[] = {255, 255, 255};

	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(count, (const jcv_point*)points, width, height, &diagram );

	// If you want to draw triangles, or relax the diagram,
	// you can iterate over the sites and get all edges easily
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

		const jcv_graphedge* e = site->edges;
		while( e )
		{
			draw_triangle( &site->p, &e->pos[0], &e->pos[1], image, width, height, 3, color_tri);
			e = e->next;
		}
	}

	// If all you need are the edges
	const jcv_edge* edge = jcv_diagram_get_edges( &diagram );
	while( edge )
	{
		draw_line((int)edge->pos[0].x, (int)edge->pos[0].y, (int)edge->pos[1].x, (int)edge->pos[1].y, image, width, height, 3, color_line);
		edge = edge->next;
	}

	jcv_diagram_free( &diagram );

	// Plot the sites
	for( int i = 0; i < count; ++i )
	{
		plot((int)points[i].x, (int)points[i].y, image, width, height, 3, color_pt);
	}

	free(points);

	char path[512];
	sprintf(path, "%s", outputfile);
	stbi_write_png(path, width, height, 3, image, width*3);
	printf("wrote %s\n", path);

	free(image);

	return 0;
}
