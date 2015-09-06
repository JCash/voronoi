/*
 * A simple test program to display the output of the voronoi generator
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "voronoi.h"

#include <vector>
#include "../test/fastjet/voronoi.h"

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
static void plot_line(int x0, int y0, int x1, int y1, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
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
		count = size / sizeof(voronoi::Point);
	}

	voronoi::Point* points = new voronoi::Point[count];
	if( !points )
		return 1;

	if( inputfile )
	{
		int nread = fread(points, count * sizeof(voronoi::Point), 1, file);
		if( nread != 1 )
		{
			fprintf(stderr, "Failed to read %lu bytes from %s\n", count * sizeof(voronoi::Point), inputfile);
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

	size_t imagesize = width*height*3;
	unsigned char* image = (unsigned char*)malloc(imagesize);
	memset(image, 0, imagesize);

	unsigned char color[] = {127, 127, 255};
	unsigned char color_pt[] = {255, 255, 255};

	if( mode == 0 )
	{
		voronoi::Voronoi generator;
		generator.generate(count, points, width, height);

		const struct voronoi::Edge* e = generator.get_edges();
		/*
		while( e )
		{
			plot_line(e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, image, width, height, 3, color);
			e = e->next;
		}*/

		const struct voronoi::Site* sites = generator.get_cells();
		for( int i = 0; i < count; ++i )
		{
			const struct voronoi::Site& site = sites[i];

			const struct voronoi::GraphEdge* e = site.edges;
			while( e )
			{
				plot_line(e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, image, width, height, 3, color);
				e = e->next;
			}
		}
	}
	else if( mode == 1 )
	{
		std::vector<fastjet::VPoint> fastjet_sites;
		fastjet_sites.resize(count);
		for( int i = 0; i < count; ++i )
		{
			fastjet_sites[i].x = points[i].x;
			fastjet_sites[i].y = points[i].y;
		}
		fastjet::VoronoiDiagramGenerator generator;
		generator.generateVoronoi(&fastjet_sites, 0, width, 0, height );

		generator.resetIterator();

		fastjet::GraphEdge* e = 0;
		while( generator.getNext(&e) )
		{
			plot_line(e->x1, e->y1, e->x2, e->y2, image, width, height, 3, color);
		}
	}

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
