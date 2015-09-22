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


#define JC_VORONOI_IMPLEMENTATION
#include "src/jc_voronoi.h"

//#define ONLYNEWIMPL
#ifndef ONLYNEWIMPL

// Before the fastjet version pollutes the namespace
#include "boost/polygon/voronoi.hpp"

#include "shaneosullivan/VoronoiDiagramGenerator.h"
#include "fastjet/voronoi.h"

#include "voronoi/VoronoiDiagram.h"

#endif // ONLYNEWIMPL

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

#ifndef ONLYNEWIMPL
	std::vector<voronoi::VoronoiSite*> vpp_sites;

	std::vector<boost::polygon::point_data<float> > boost_points;
#endif

	std::vector< std::pair<PointF, PointF> > collectededges;
	std::vector< std::pair<PointF, std::vector<std::pair<PointF, PointF> > > > collectedcells;
};



#define MAP_DIMENSION 4096

void setup_sites(int count, Context* context)
{
	context->count = count;
	context->fsites = new PointF[count];
	context->dsites = new PointD[count];
	context->sitesx = new float[count];
	context->sitesy = new float[count];
	context->dpoints.resize(count);
#ifndef ONLYNEWIMPL
	context->vpp_sites.resize(count);
	context->boost_points.resize(count);
#endif

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

#ifndef ONLYNEWIMPL
		context->vpp_sites[i] = new voronoi::VoronoiSite(x, y);

		context->boost_points[i] = boost::polygon::point_data<float>(x, y);
#endif
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
	jcv_diagram diagram = { 0 };
	jcv_diagram_generate(context->count, (const jcv_point*)context->fsites, MAP_DIMENSION, MAP_DIMENSION, &diagram );

	if( context->collectedges )
	{
		const jcv_edge* edge = jcv_diagram_get_edges( &diagram );
		while( edge )
		{
			context->collectededges.push_back( std::make_pair( PointF(edge->pos[0].x, edge->pos[0].y), PointF(edge->pos[1].x, edge->pos[1].y) ) );
			edge = edge->next;
		}

		context->collectedcells.reserve(context->count);

		const jcv_site* sites = jcv_diagram_get_sites( &diagram );
		for( int i = 0; i < context->count; ++i )
		{
			const jcv_site& site = sites[i];

			std::vector< std::pair<PointF, PointF> > collectedsiteedges;

			const jcv_graphedge* e = site.edges;
			while( e )
			{
				collectedsiteedges.push_back( std::make_pair(PointF(e->pos[0].x, e->pos[0].y), PointF(e->pos[1].x, e->pos[1].y)) );
				e = e->next;
			}

			context->collectedcells.push_back( std::make_pair(PointF(site.p.x, site.p.y), collectedsiteedges) );
		}

	}

	jcv_diagram_free( &diagram );
	return 0;
}

#ifndef ONLYNEWIMPL
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

		const std::map< voronoi::VoronoiSite*, voronoi::VoronoiCell*>& cells = diagram.cells();
		//for( size_t i = 0; i < cells.size(); ++i )
		for( std::map< voronoi::VoronoiSite*, voronoi::VoronoiCell*>::const_iterator it = cells.begin();
				it != cells.end(); ++it )
		{
			std::vector< std::pair<PointF, PointF> > collectedsiteedges;

			const voronoi::VoronoiSite& site = *it->first;
			const voronoi::VoronoiCell& cell = *it->second;
			for( size_t i = 0; i < cell.edges.size(); ++i )
			{
				const geometry::Line& line = cell.edges[i]->getRenderLine(boundingPolygon);
				collectedsiteedges.push_back( std::make_pair( PointF(line.startPoint().x(), line.startPoint().y()), PointF(line.endPoint().x(), line.endPoint().y()) ) );
			}

			context->collectedcells.push_back( std::make_pair(PointF(site.position().x(), site.position().y()), collectedsiteedges) );
		}
	}
	return 0;
}

// http://www.boost.org/doc/libs/1_55_0/libs/polygon/example/voronoi_visualizer.cpp
static void clip_infinite_edge( Context* context, const boost::polygon::voronoi_diagram<double>::edge_type& edge, std::vector<PointF>& clipped_edge)
{
	using namespace boost::polygon;
	typedef double coordinate_type;
	typedef point_data<coordinate_type> point_type;
	const voronoi_diagram<coordinate_type>::cell_type& cell1 = *edge.cell();
	const voronoi_diagram<coordinate_type>::cell_type& cell2 = *edge.twin()->cell();
	point_type origin, direction;
	// Infinite edges could not be created by two segment sites.
	if (cell1.contains_point() && cell2.contains_point())
	{
		std::size_t index = cell1.source_index();
		PointF p1 = context->fsites[index];
		index = cell2.source_index();
		PointF p2 = context->fsites[index];
		origin.x((p1.x + p2.x) * 0.5);
		origin.y((p1.y + p2.y) * 0.5);
		direction.x(p1.y - p2.y);
		direction.y(p2.x - p1.x);
	} else {
	}
	coordinate_type side = MAP_DIMENSION;
	coordinate_type koef = side / (std::max)(fabs(direction.x()), fabs(direction.y()));
	if (edge.vertex0() == NULL) {
	  clipped_edge.push_back( PointF(origin.x() - direction.x() * koef, origin.y() - direction.y() * koef));
	} else {
	  clipped_edge.push_back( PointF(edge.vertex0()->x(), edge.vertex0()->y()));
	}
	if (edge.vertex1() == NULL)
	{
		clipped_edge.push_back(PointF( origin.x() + direction.x() * koef, origin.y() + direction.y() * koef));
	}
	else
	{
	  clipped_edge.push_back( PointF(edge.vertex1()->x(), edge.vertex1()->y()));
	}
}

static int boost_voronoi(Context* context)
{
	boost::polygon::voronoi_diagram<double> vd;
	boost::polygon::construct_voronoi(context->boost_points.begin(), context->boost_points.end(), &vd);

	if( context->collectedges )
	{
		for( boost::polygon::voronoi_diagram<double>::const_edge_iterator it = vd.edges().begin(); it != vd.edges().end(); ++it )
		{
			const boost::polygon::voronoi_diagram<double>::edge_type& edge = *it;
			if( !edge.is_primary() )
				continue;
			if( edge.is_finite() )
			{
				context->collectededges.push_back( std::make_pair( PointF(edge.vertex0()->x(), edge.vertex0()->y()), PointF(edge.vertex1()->x(), edge.vertex1()->y()) ) );
			}
			else
			{
				std::vector<PointF> points;
				clip_infinite_edge( context, edge, points );
				context->collectededges.push_back( std::make_pair( points[0], points[1] ) );
			}
		}

		for( boost::polygon::voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it )
		{
			if(!it->contains_point())
				continue;
			const boost::polygon::voronoi_diagram<double>::cell_type& cell = *it;
			const boost::polygon::voronoi_diagram<double>::edge_type* edge = cell.incident_edge();

			std::vector< std::pair<PointF, PointF> > collectedgraphedges;
			do {
				//if(edge->is_primary())
				{
					if( edge->is_finite() )
					{
						collectedgraphedges.push_back( std::make_pair( PointF(edge->vertex0()->x(), edge->vertex0()->y()), PointF(edge->vertex1()->x(), edge->vertex1()->y()) ) );
					}
					else
					{
						std::vector<PointF> points;
						clip_infinite_edge( context, *edge, points );
						collectedgraphedges.push_back( std::make_pair( points[0], points[1] ) );
					}
				}
				edge = edge->next();
			} while (edge != cell.incident_edge());

			std::size_t index = it->source_index();
			context->collectedcells.push_back( std::make_pair( context->fsites[index], collectedgraphedges ) );
		}
	}
	return 0;
}
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
static void plot_line(int x0, int y0, int x1, int y1, unsigned char* image, int dimension, int nchannels, unsigned char* color)
{
	int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = dx+dy, e2; // error value e_xy

	for(;;)
	{  // loop
		plot(x0,y0, image, dimension, dimension, nchannels, color);
		if (x0==x1 && y0==y1) break;
		e2 = 2*err;
		if (e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
		if (e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
	}
}
// http://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
template<typename PT>
static inline int orient2d(const PT& a, const PT& b, const PT& c)
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

template<typename PT>
static void draw_triangle(const PT& v0, const PT& v1, const PT& v2, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
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
    PT p;
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

static void output_image(const char* name, Context* context)
{
	size_t datasize = MAP_DIMENSION*MAP_DIMENSION*3;
	unsigned char* data = new unsigned char[datasize];
	memset(data, 0, datasize);

	unsigned char color_blue[] = {127, 127, 255};
	unsigned char color_white[] = {255, 255, 255};

	srand(0);

	for( size_t i = 0; i < context->collectedcells.size(); ++i )
	{
		const PointF& site = context->collectedcells[i].first;
		const std::vector< std::pair<PointF, PointF> >& edges = context->collectedcells[i].second;

		unsigned char color_tri[3];
		int colorcount = 0;

		for( size_t i = 0; i < edges.size(); ++i )
		{
			color_tri[0] = 100 + colorcount * 10;
			color_tri[1] = 100 + colorcount * 10;
			color_tri[2] = 100 + colorcount * 10;
			colorcount++;

			// Needed for voronoi++
			float det = orient2d(site, edges[i].first, edges[i].second);
			if( det > 0 )
				draw_triangle( site, edges[i].first, edges[i].second, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_tri);
			else
				draw_triangle( site, edges[i].second, edges[i].first, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_tri);
		}
	}

	for( size_t i = 0; i < context->collectededges.size(); ++i )
	{
		plot_line( (int)context->collectededges[i].first.x, (int)context->collectededges[i].first.y,
				   (int)context->collectededges[i].second.x, (int)context->collectededges[i].second.y, data, MAP_DIMENSION, 3, color_blue);
	}

	for( int i = 0; i < context->count; ++i )
	{
		plot( (int)context->fsites[i].x, (int)context->fsites[i].y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
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
	context->collectedcells.clear();
	context->collectedcells.resize(0);
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

	run_test("jc_voronoi", &context, null_setup, new_voronoi);

#ifndef ONLYNEWIMPL

	fflush(stdout);
	run_test("fastjet", &context, null_setup, fastjet_voronoi);

	fflush(stdout);
	run_test("boost", &context, null_setup, boost_voronoi);

	fflush(stdout);
	run_test("voronoi++", &context, null_setup, voronoiplusplus_voronoi);

	//fflush(stdout);
	//run_test("osullivan", &context, null_setup, shaneosullivan_voronoi);

	fflush(stdout);
#endif

	return 0;
}
