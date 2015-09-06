#include <cstring>
#include <cstdlib>

static size_t g_MallocCount = 0;
static size_t g_MallocSize 	= 0;

void* override_alloc(size_t sz)
{
	++g_MallocCount;
	g_MallocSize += sz;
	return malloc(sz);
}
#define malloc(_X)	override_alloc(_X)

void* operator new(std::size_t sz)
{
    return override_alloc(sz);
}


#include "src/voronoi.h"

// Before the fastjet version pollutes the namespace
#include "boost/polygon/voronoi.hpp"

#include "shaneosullivan/VoronoiDiagramGenerator.h"
#include "fastjet/voronoi.h"

#include "voronoi/VoronoiDiagram.h"

#include "timeit.h"
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct PointF
{
	float x;
	float y;
	PointF() {};
	PointF(float _x, float _y) : x(_x), y(_y) {}
};

struct PointD
{
	double x;
	double y;
};

struct Context
{
	int numiterations;
	bool collectedges;
	int count;
	PointF* fsites;
	PointD* dsites;
	float* sitesx;
	float* sitesy;

	std::vector<PointD> dpoints;

	std::vector<voronoi::VoronoiSite*> vpp_sites;

	std::vector<boost::polygon::point_data<float> > boost_points;

	std::vector< std::pair<PointF, PointF> > collectededges;
};



#define MAP_DIMENSION 1024

void setup_sites(int count, Context* context)
{
	context->count = count;
	context->fsites = new PointF[count];
	context->dsites = new PointD[count];
	context->sitesx = new float[count];
	context->sitesy = new float[count];
	context->dpoints.resize(count);
	context->vpp_sites.resize(count);
	context->boost_points.resize(count);

    int pointoffset = 10; // move the points inwards, for aestetic reasons
	srand(0);
	for( int i = 0; i < count; ++i )
	{
		float x = float(pointoffset + rand() % (MAP_DIMENSION-2*pointoffset));
		float y = float(pointoffset + rand() % (MAP_DIMENSION-2*pointoffset));
		context->fsites[i].x = x;
		context->fsites[i].y = y;
		context->dsites[i].x = x;
		context->dsites[i].y = y;
		context->sitesx[i] = x;
		context->sitesy[i] = y;
		context->dpoints[i].x = x;
		context->dpoints[i].y = y;

		context->vpp_sites[i] = new voronoi::VoronoiSite(x, y);

		context->boost_points[i] = boost::polygon::point_data<float>(x, y);
	}

	context->collectedges = false;
}

void start_test(const char* name, Context* context)
{
	g_MallocCount = 0;
	g_MallocSize  = 0;
}

void stop_test(const char* name, Context* context)
{
	size_t overheadsize = context->count * sizeof(std::chrono::duration<float>);
	size_t overheadnumallocations = 1;
	printf("%s\tused %lu bytes in %lu allocations\n", name, g_MallocSize/context->numiterations - overheadsize, g_MallocCount/context->numiterations - overheadnumallocations);
}

void null_setup(Context* context)
{
}

int new_voronoi(Context* context)
{
	voronoi::Voronoi generator;
	generator.generate(context->count, (const voronoi::Point*)context->fsites, MAP_DIMENSION, MAP_DIMENSION);

	if( context->collectedges )
	{
		const voronoi::Edge* edge = generator.get_edges();
		while( edge )
		{
			context->collectededges.push_back( std::make_pair( PointF(edge->pos[0].x, edge->pos[0].y), PointF(edge->pos[1].x, edge->pos[1].y) ) );
			edge = edge->next;
		}
	}
	return 0;
}

int shaneosullivan_voronoi(Context* context)
{
	shaneosullivan::VoronoiDiagramGenerator generator;
	generator.generateVoronoi(context->sitesx, context->sitesy, context->count, 0, MAP_DIMENSION, 0, MAP_DIMENSION);

	if( context->collectedges )
	{
		generator.resetIterator();
		float x1,y1,x2,y2;
		while( generator.getNext(x1,y1,x2,y2) )
		{
			context->collectededges.push_back( std::make_pair( PointF(x1,y1), PointF(x2,y2) ) );
		}
	}

	return 0;
}

int fastjet_voronoi(Context* context)
{
	fastjet::VoronoiDiagramGenerator generator;
	generator.generateVoronoi((std::vector<fastjet::VPoint>*)&context->dpoints, 0, MAP_DIMENSION, 0, MAP_DIMENSION);

	if( context->collectedges )
	{
		generator.resetIterator();
		fastjet::GraphEdge* edge;
		while( generator.getNext(&edge) )
		{
			context->collectededges.push_back( std::make_pair( PointF(edge->x1,edge->y1), PointF(edge->x2,edge->y2) ) );
		}
	}
	return 0;
}

int voronoiplusplus_voronoi(Context* context)
{
	voronoi::VoronoiDiagram diagram;
	//diagram.removeDuplicates(context->vpp_sites);
	diagram.initialize(context->vpp_sites);
	diagram.calculate();

	if( context->collectedges )
	{
		const geometry::Rectangle rect(0, 0, MAP_DIMENSION, MAP_DIMENSION);
		geometry::ConvexPolygon boundingPolygon;
	    boundingPolygon << rect.topLeft();
	    boundingPolygon << rect.topRight();
	    boundingPolygon << rect.bottomRight();
	    boundingPolygon << rect.bottomLeft();

		const std::vector<voronoi::VoronoiEdge*>& edges = diagram.edges();
		for( size_t i = 0; i < edges.size(); ++i )
		{
			const geometry::Line& line = edges[i]->getRenderLine(boundingPolygon);
			context->collectededges.push_back( std::make_pair( PointF(line.startPoint().x(), line.startPoint().y()), PointF(line.endPoint().x(), line.endPoint().y()) ) );
		}
	}
	return 0;
}

int boost_voronoi(Context* context)
{
	boost::polygon::voronoi_diagram<double> vd;
	boost::polygon::construct_voronoi(context->boost_points.begin(), context->boost_points.end(), &vd);

	if( context->collectedges )
	{
		for( boost::polygon::voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it )
		{
			if( !it->is_primary() )
				continue;
			if( it->is_finite() )
			{
				const boost::polygon::voronoi_diagram<double>::edge_type& edge = *it;

				context->collectededges.push_back( std::make_pair( PointF(edge.vertex0()->x(), edge.vertex0()->y()), PointF(edge.vertex1()->x(), edge.vertex1()->y()) ) );
			}
		}
	}
	return 0;
}

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

static void output_image(const char* name, Context* context)
{
	size_t datasize = MAP_DIMENSION*MAP_DIMENSION*3;
	unsigned char* data = new unsigned char[datasize];
	memset(data, 0, datasize);

	unsigned char color_blue[] = {127, 127, 255};
	unsigned char color_white[] = {255, 255, 255};

	for( size_t i = 0; i < context->collectededges.size(); ++i )
	{
		plot_line( (int)context->collectededges[i].first.x, (int)context->collectededges[i].first.y,
				   (int)context->collectededges[i].second.x, (int)context->collectededges[i].second.y, data, MAP_DIMENSION, 3, color_blue);
	}

	for( int i = 0; i < context->count; ++i )
	{
		plot( (int)context->fsites[i].x, (int)context->fsites[i].y, data, MAP_DIMENSION, 3, color_white);
	}

	char path[512];
	sprintf(path, "images/voronoi_%s_%d.png", name, context->count);
	stbi_write_png(path, MAP_DIMENSION, MAP_DIMENSION, 3, data, MAP_DIMENSION*3);
	//printf("wrote %s\n", path);

	delete[] data;
}

template<typename SetupFunc, typename Func>
void generate_image(const char* name, Context* context, SetupFunc setupfunc, Func func)
{
	context->collectededges.clear();
	context->collectededges.resize(0);
	context->collectedges = true;

	setupfunc(context);
	func(context);

	context->collectedges = false;

	output_image(name, context);
}

template<typename SetupFunc, typename Func>
void run_test(const char* name, Context* context, SetupFunc setupfunc, Func func)
{
	size_t len = strlen(name);
	char buffer[18];
	memset(buffer, ' ', sizeof(buffer));
	memcpy( buffer, name, len < sizeof(buffer) ? len : sizeof(buffer)-1 );
	buffer[sizeof(buffer)-1] = 0;

	CTimeIt timeit;
	start_test(buffer, context);
	timeit.run<int>(context->numiterations, setupfunc, func, context);
	stop_test(buffer, context);

	timeit.report(std::cout, buffer, 0.0f);
	generate_image(name, context, null_setup, func);
}

int main(int argc, const char** argv)
{
	int count = 200;
	if( argc > 1 )
		count = atol(argv[1]);

	Context context;
	setup_sites(count, &context);
	context.numiterations = 20;

	std::cout << "Generating voronoi diagrams for " << count << " sites..." << std::endl;

	fflush(stdout);

	run_test("voronoi++", &context, null_setup, voronoiplusplus_voronoi);

	fflush(stdout);
	run_test("boost", &context, null_setup, boost_voronoi);

	fflush(stdout);
	run_test("fastjet", &context, null_setup, fastjet_voronoi);

	fflush(stdout);
	run_test("osullivan", &context, null_setup, shaneosullivan_voronoi);

	fflush(stdout);

	run_test("jc_voronoi", &context, null_setup, new_voronoi);

	return 0;
}
