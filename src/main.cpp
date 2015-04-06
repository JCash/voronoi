/*
 * A simple test program to display the output of the voronoi generator
 */

#include <tuple>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "voronoi.h"

#include "../test/fastjet/voronoi.h"

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
	int count = 200;
	if( argc > 1 )
		count = atol(argv[1]);

	int dimension = 512;
	if( argc > 2 )
		dimension = atol(argv[2]);

	int mode = 0;
	if( argc > 3 )
		mode = atol(argv[3]);

	size_t datasize = dimension*dimension*3;
	unsigned char* data = new unsigned char[datasize];
	memset(data, 0, datasize);

	unsigned char color[] = {127, 127, 255};
	unsigned char color_pt[] = {255, 255, 255};

	voronoi::Point* points = new voronoi::Point[count];
	if( !points )
		return 1;

	int pointoffset = 10; // move the points inwards, for aestetic reasons
	srand(0);

	// DEBUG THIS CASE LATER with n = 56
	int keepcount = 0;

	for( int i = 0; i < count; ++i )
	{
		float x = float(pointoffset + rand() % (dimension-2*pointoffset));
		float y = float(pointoffset + rand() % (dimension-2*pointoffset));
		points[i].x = x;
		points[i].y = y;


		// debug with 37 512 0
		/*
		if( x > 340 && y > 200 && y < 395)
		{
			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}*/



		/*
		// debug with 76 512 0
		if( x < 150 && y < 178 )
		{
			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}*/

		/*
		// debug with 438 512 0
		if( x > 440 && y > 147 && y < 225 )
		{
			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}
		*/

		/*
		// debug with 500 512 0
		if( x > 31 && x < 57 && y > 145 && y < 168 )
		{
			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}
		*/

		/*
		// debug with 500 512 0
		if( x > 222 && x < 252 && y > 160 && y < 220 )
		{

			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}
		*/

/*
 	 	// DEBUG THIS CASE LATER with n = 56   w = 512
		if( x < 350 && y > 430 )
		{
			if( x == 226 && y == 460 )
				continue;
			//if( x == 285 && y == 469 )
			//	continue;
			//if( x == 313 && y == 482 )
			//	continue;
			//if( x == 171 && y == 437 )
			//	continue;
			//if( x == 10 && y == 493 )
			//	continue;
			points[keepcount].x = x;
			points[keepcount].y = y;
			++keepcount;
		}
 */


	}
	//count = keepcount;


if(mode == 0)
{
	printf("MAWE\n");

	voronoi::Voronoi generator;
	generator.generate(count, points, dimension, dimension);

	const struct voronoi::Edge* edge = generator.get_edges();
	while( edge )
	{
		plot_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, data, dimension, 3, color);
		edge = edge->next;
	}

	for( int i = 0; i < count; ++i )
	{
		int index = points[i].y * dimension * 3 + points[i].x * 3;
		data[index+0] = 255;
		data[index+1] = 255;
		data[index+2] = 255;
	}

	delete[] points;

}
else if(mode == 2)
{
	printf("FASTJET\n");

	std::vector<fastjet::VPoint> vertices;

	for( int i = 0; i < count; ++i )
	{
		vertices.push_back(fastjet::VPoint(points[i].x, points[i].y));
	}

	fastjet::VoronoiDiagramGenerator generator;
	generator.generateVoronoi(&vertices, 0, dimension, 0, dimension);
	fastjet::GraphEdge* edge = 0;

	generator.resetIterator();
	while( generator.getNext(&edge) )
	{
		//printf("l: %f, %f -> %f, %f\n", edge->x1, edge->y1, edge->x2, edge->y2);
		plot_line( edge->x1, edge->y1, edge->x2, edge->y2, data, dimension, 3, color );
	}

	for( std::vector<fastjet::VPoint>::iterator it = vertices.begin(); it != vertices.end(); ++it )
	{
		plot( it->x, it->y, data, dimension, 3, color_pt );
	}
}

	char path[512];
	sprintf(path, "out_%d.png", mode);
	stbi_write_png(path, dimension, dimension, 3, data, dimension*3);
	printf("wrote %s\n", path);

	delete[] data;

	return 0;
}
