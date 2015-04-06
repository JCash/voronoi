/*
 * A simple test program to display the output of the voronoi generator
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "voronoi.h"

static void plot(int x, int y, unsigned char* image, int dimension, int nchannels, unsigned char* color)
{
	if( x < 0 || y < 0 || x > (dimension-1) || y > (dimension-1) )
		return;
	int index = y * dimension * nchannels + x * nchannels;
	for( int i = 0; i < nchannels; ++i )
	{
		image[index+i] = color[i];
	}
}

// http://members.chello.at/~easyfilter/bresenham.html
static void plot_line(int x0, int y0, int x1, int y1, unsigned char* image, int dimension, int nchannels, unsigned char* color)
{
	int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = dx+dy, e2; // error value e_xy

	for(;;)
	{  // loop
		plot(x0,y0, image, dimension, nchannels, color);
		if (x0==x1 && y0==y1) break;
		e2 = 2*err;
		if (e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
		if (e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
	}
}

int main(int argc, const char** argv)
{
	// Number of sites to generate
	int count = 200;
	if( argc > 1 )
		count = atol(argv[1]);

	// Image dimension
	int dimension = 512;
	if( argc > 2 )
		dimension = atol(argv[2]);

	voronoi::Point* points = new voronoi::Point[count];
	if( !points )
		return 1;

	int pointoffset = 10; // move the points inwards, for aestetic reasons

	srand(0);

	for( int i = 0; i < count; ++i )
	{
		points[i].x = float(pointoffset + rand() % (dimension-2*pointoffset));
		points[i].y = float(pointoffset + rand() % (dimension-2*pointoffset));
	}

	voronoi::Voronoi generator;
	generator.generate(count, points, dimension, dimension);

	size_t imagesize = dimension*dimension*3;
	unsigned char* image = (unsigned char*)malloc(imagesize);
	memset(image, 0, imagesize);

	unsigned char color[] = {127, 127, 255};
	unsigned char color_pt[] = {255, 255, 255};

	const struct voronoi::Edge* edge = generator.get_edges();
	while( edge )
	{
		plot_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image, dimension, 3, color);
		edge = edge->next;
	}

	// Plot the sites
	for( int i = 0; i < count; ++i )
	{
		int index = points[i].y * dimension * 3 + points[i].x * 3;
		image[index+0] = 255;
		image[index+1] = 255;
		image[index+2] = 255;
	}

	delete[] points;

	char path[512];
	sprintf(path, "example.png");
	stbi_write_png(path, dimension, dimension, 3, image, dimension*3);
	printf("wrote %s\n", path);

	free(image);

	return 0;
}
