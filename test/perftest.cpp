#include <cstring>
#include <cstdlib>
#include <cmath>
#include <float.h>

static size_t g_MallocCount = 0;
static size_t g_MallocSize = 0;
static size_t g_CurrentMallocSize = 0;
static size_t g_PeakMallocSize = 0;
static size_t g_RetainedDiagramSize = 0;

struct AllocationHeader
{
	size_t size;
	size_t marker;
};

static const size_t ALLOCATION_MARKER = (size_t)0x4a43564d;

void* override_alloc(size_t sz)
{
	++g_MallocCount;
	g_MallocSize += sz;
	g_CurrentMallocSize += sz;
	g_PeakMallocSize = g_PeakMallocSize > g_CurrentMallocSize ? g_PeakMallocSize : g_CurrentMallocSize;
	AllocationHeader* header = (AllocationHeader*)malloc(sizeof(AllocationHeader) + sz);
	header->size = sz;
	header->marker = ALLOCATION_MARKER;
	return header + 1;
}

void override_free(void* p)
{
	if( !p )
		return;
	AllocationHeader* header = ((AllocationHeader*)p) - 1;
	if( header->marker == ALLOCATION_MARKER )
	{
		g_CurrentMallocSize = header->size <= g_CurrentMallocSize ? g_CurrentMallocSize - header->size : 0;
		header->marker = 0;
	}
	free(header);
}
#define malloc(_X)	override_alloc(_X)
#define free(_X)	override_free(_X)

void* operator new(std::size_t sz)
{
    return override_alloc(sz);
}

void* operator new[](std::size_t sz)
{
    return override_alloc(sz);
}

void operator delete(void* p) noexcept
{
	override_free(p);
}

void operator delete[](void* p) noexcept
{
	override_free(p);
}

void operator delete(void* p, std::size_t) noexcept
{
	override_free(p);
}

void operator delete[](void* p, std::size_t) noexcept
{
	override_free(p);
}

#if defined(USE_JC_VORONOI)
#define JC_VORONOI_IMPLEMENTATION
#include "src/jc_voronoi.h"

#if defined(USE_JC_VORONOI_LEGACY_API)
typedef jcv_graphedge PerfGraphEdge;

struct PerfEdgeIter
{
	const jcv_edge* current;
};

struct PerfGraphEdgeIter
{
	const jcv_graphedge* current;
};

static void perf_edge_begin(const jcv_diagram* diagram, PerfEdgeIter* iter)
{
	iter->current = jcv_diagram_get_edges(diagram);
}

static const jcv_edge* perf_edge_next(PerfEdgeIter* iter)
{
	const jcv_edge* edge = iter->current;
	if( edge )
		iter->current = jcv_diagram_get_next_edge(edge);
	return edge;
}

static void perf_graph_edge_begin(const jcv_diagram*, const jcv_site* site, PerfGraphEdgeIter* iter)
{
	iter->current = site->edges;
}

static const PerfGraphEdge* perf_graph_edge_next(PerfGraphEdgeIter* iter)
{
	const jcv_graphedge* edge = iter->current;
	if( edge )
		iter->current = edge->next;
	return edge;
}

static const jcv_point* perf_graph_edge_position(const jcv_diagram*, const PerfGraphEdge* edge, int endpoint)
{
	return &edge->pos[endpoint];
}
#else
typedef jcv_edge PerfGraphEdge;

struct PerfEdgeIter
{
	jcv_edge_iter iterator;
	jcv_edge current;
};

struct PerfGraphEdgeIter
{
	jcv_edge_iter iterator;
	jcv_edge current;
};

static void perf_edge_begin(const jcv_diagram* diagram, PerfEdgeIter* iter)
{
	jcv_diagram_get_edges(diagram, &iter->iterator);
}

static const jcv_edge* perf_edge_next(PerfEdgeIter* iter)
{
	return jcv_edge_next(&iter->iterator, &iter->current) ? &iter->current : 0;
}

static void perf_graph_edge_begin(const jcv_diagram* diagram, const jcv_site* site, PerfGraphEdgeIter* iter)
{
	jcv_site_get_edges(diagram, site, &iter->iterator);
}

static const jcv_edge* perf_graph_edge_next(PerfGraphEdgeIter* iter)
{
	return jcv_edge_next(&iter->iterator, &iter->current) ? &iter->current : 0;
}

static const jcv_point* perf_graph_edge_position(const jcv_diagram*, const jcv_edge* edge, int endpoint)
{
	return &edge->pos[endpoint];
}
#endif
#endif

#if defined(USE_BOOST)
#include "boost/polygon/voronoi.hpp"
#endif

#if defined(USE_SHANEOSULLIVAN)
#include "shaneosullivan/VoronoiDiagramGenerator.h"
#endif

#if defined(USE_FASTJET)
#include "fastjet/internal/Voronoi.hh"
#endif

#if defined(USE_VORONOIPP)
#include "voronoi/VoronoiDiagram.h"
#endif

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
	int generate_images;
	PointF* fsites;
	PointD* dsites;
	float* sitesx;
	float* sitesy;
	PointF dgmin;
	PointF dgmax;
	volatile double totalcellarea;
#if defined(USE_JC_VORONOI)
	volatile jcv_real vertexchecksum;
	jcv_diagram vertexdiagram;
	jcv_point* vertices;
#endif

	const char* testname;

	std::vector<PointD> dpoints;

#if defined(USE_VORONOIPP)
	std::vector<voronoi::VoronoiSite*> vpp_sites;
#endif
#if defined(USE_BOOST)
	std::vector<boost::polygon::point_data<float> > boost_points;
	boost::polygon::voronoi_diagram<double> boost_vertexdiagram;
	std::vector<PointD> boost_vertices;
	volatile double boost_vertexchecksum;
#endif

	std::vector< std::pair<PointF, PointF> > collectededges;
	std::vector< std::pair<PointF, std::vector<std::pair<PointF, PointF> > > > collectedcells;
};



#define MAP_DIMENSION 4096

void fill_random_sites(PointF* sites, int count)
{
	const int pointoffset = 10; // move the points inwards, for aesthetic reasons
	srand(0);
	for( int i = 0; i < count; ++i )
	{
		sites[i].x = float(pointoffset + rand() % (MAP_DIMENSION-2*pointoffset));
		sites[i].y = float(pointoffset + rand() % (MAP_DIMENSION-2*pointoffset));
	}
}

void fill_symmetric_diagonal_pairs(PointF* sites, int count)
{
	for( int i = 0; i < count; ++i )
	{
		const float value = float(i / 2 + 1);
		sites[i].x = (i & 1) ? -value : value;
		sites[i].y = -value;
	}
}

void populate_site_arrays(Context* context)
{
	for( int i = 0; i < context->count; ++i )
	{
		const float x = context->fsites[i].x;
		const float y = context->fsites[i].y;
		context->dsites[i].x = x;
		context->dsites[i].y = y;
		context->sitesx[i] = x;
		context->sitesy[i] = y;
		context->dpoints[i].x = x;
		context->dpoints[i].y = y;

		context->dgmin.x = std::min(context->dgmin.x, x);
		context->dgmin.y = std::min(context->dgmin.y, y);
		context->dgmax.x = std::max(context->dgmax.x, x);
		context->dgmax.y = std::max(context->dgmax.y, y);

#if defined(USE_VORONOIPP)
		context->vpp_sites[i] = new voronoi::VoronoiSite(x, y);
#endif
#if defined(USE_BOOST)
		context->boost_points[i] = boost::polygon::point_data<float>(x, y);
#endif
	}
}

void setup_sites(int count, Context* context)
{
	context->count = count;
	context->fsites = new PointF[count];
	context->dsites = new PointD[count];
	context->sitesx = new float[count];
	context->sitesy = new float[count];
	context->dpoints.resize(count);
#if defined(USE_VORONOIPP)
	context->vpp_sites.resize(count);
#endif
#if defined(USE_BOOST)
	context->boost_points.resize(count);
#endif

	context->dgmin = PointF(FLT_MAX, FLT_MAX);
	context->dgmax = PointF(-FLT_MAX, -FLT_MAX);

	const bool symmetric_diagonal_pairs = context->testname && strcmp(context->testname, "symmetric_diagonal_pairs") == 0;
	if (symmetric_diagonal_pairs)
		fill_symmetric_diagonal_pairs(context->fsites, count);
	else
		fill_random_sites(context->fsites, count);
	populate_site_arrays(context);

	context->dgmin.x -= 1;
	context->dgmin.y -= 1;
	context->dgmax.x += 1;
	context->dgmax.y += 1;

	context->collectedges = false;
#if defined(USE_JC_VORONOI)
	memset(&context->vertexdiagram, 0, sizeof(context->vertexdiagram));
	context->vertices = 0;
#endif
}

void start_test(const char* name, Context* context)
{
	g_MallocCount = 0;
	g_MallocSize  = 0;
	g_CurrentMallocSize = 0;
	g_PeakMallocSize = 0;
	g_RetainedDiagramSize = 0;
}

void stop_test(const char* name, Context* context)
{
	size_t overheadsize = (size_t)context->numiterations * sizeof(std::chrono::duration<double>);
	size_t usedsize = g_PeakMallocSize >= overheadsize ? g_PeakMallocSize - overheadsize : 0;
	size_t retainedsize = g_RetainedDiagramSize >= overheadsize ? g_RetainedDiagramSize - overheadsize : 0;
	size_t allocations = g_MallocCount > 0 ? g_MallocCount - 1 : 0;
	printf("%s\tpeak %lu bytes, retained %lu bytes in %lu allocations\n",
		name, usedsize, retainedsize, allocations/context->numiterations);
}

void null_setup(Context* context)
{
}

#if defined(USE_JC_VORONOI)
int jc_voronoi_impl(Context* context, bool calculatecellarea)
{
	jcv_diagram diagram = { 0 };
	jcv_rect rect = { {context->dgmin.x, context->dgmin.y}, {context->dgmax.x, context->dgmax.y} };
	jcv_diagram_generate(context->count, (const jcv_point*)context->fsites, &rect, 0, &diagram );

	if( calculatecellarea )
	{
		double totalarea = 0.0;
		const jcv_site* sites = jcv_diagram_get_sites( &diagram );
		for( int i = 0; i < diagram.numsites; ++i )
		{
			double twicearea = 0.0;
			PerfGraphEdgeIter graph_iter;
			perf_graph_edge_begin(&diagram, &sites[i], &graph_iter);
			for( const PerfGraphEdge* edge = perf_graph_edge_next(&graph_iter); edge; edge = perf_graph_edge_next(&graph_iter) )
			{
				const jcv_point* pos0 = perf_graph_edge_position(&diagram, edge, 0);
				const jcv_point* pos1 = perf_graph_edge_position(&diagram, edge, 1);
				twicearea += (double)pos0->x * (double)pos1->y -
				             (double)pos1->x * (double)pos0->y;
			}
			totalarea += std::fabs(twicearea) * 0.5;
		}
		context->totalcellarea = totalarea;
	}

	if( context->collectedges )
	{
		PerfEdgeIter edge_iter;
		perf_edge_begin(&diagram, &edge_iter);
		const jcv_edge* edge = perf_edge_next(&edge_iter);
		while( edge )
		{
			context->collectededges.push_back( std::make_pair( PointF(edge->pos[0].x, edge->pos[0].y), PointF(edge->pos[1].x, edge->pos[1].y) ) );
			edge = perf_edge_next(&edge_iter);
		}

		context->collectedcells.reserve(context->count);

		const jcv_site* sites = jcv_diagram_get_sites( &diagram );
		for( int i = 0; i < context->count; ++i )
		{
			const jcv_site& site = sites[i];

			std::vector< std::pair<PointF, PointF> > collectedsiteedges;

			PerfGraphEdgeIter graph_iter;
			perf_graph_edge_begin(&diagram, &site, &graph_iter);
			const PerfGraphEdge* e;
			while( (e = perf_graph_edge_next(&graph_iter)) != 0 )
			{
				const jcv_point* pos0 = perf_graph_edge_position(&diagram, e, 0);
				const jcv_point* pos1 = perf_graph_edge_position(&diagram, e, 1);
				collectedsiteedges.push_back( std::make_pair(PointF(pos0->x, pos0->y), PointF(pos1->x, pos1->y)) );
			}

			context->collectedcells.push_back( std::make_pair(PointF(site.p.x, site.p.y), collectedsiteedges) );
		}

	}

	g_RetainedDiagramSize = std::max(g_RetainedDiagramSize, g_CurrentMallocSize);
	jcv_diagram_free( &diagram );
	return 0;
}

int jc_voronoi(Context* context)
{
	return jc_voronoi_impl(context, false);
}

int jc_voronoi_cell_areas(Context* context)
{
	return jc_voronoi_impl(context, true);
}

void setup_jc_voronoi_vertices(Context* context)
{
	jcv_rect rect = { {context->dgmin.x, context->dgmin.y}, {context->dgmax.x, context->dgmax.y} };
	jcv_diagram_generate(context->count, (const jcv_point*)context->fsites, &rect, 0, &context->vertexdiagram);
	context->vertices = new jcv_point[jcv_get_num_vertices(&context->vertexdiagram)];
}

int jc_voronoi_get_vertices(Context* context)
{
	jcv_diagram_get_vertices(&context->vertexdiagram, context->vertices);
	int numvertices = jcv_get_num_vertices(&context->vertexdiagram);
	if( numvertices > 0 )
		context->vertexchecksum = context->vertices[numvertices-1].x;
	return 0;
}

void teardown_jc_voronoi_vertices(Context* context)
{
	delete[] context->vertices;
	context->vertices = 0;
	jcv_diagram_free(&context->vertexdiagram);
	memset(&context->vertexdiagram, 0, sizeof(context->vertexdiagram));
}
#endif

#if defined(USE_SHANEOSULLIVAN)
int shaneosullivan_voronoi(Context* context)
{
	VoronoiDiagramGenerator generator;
	generator.generateVoronoi(context->sitesx, context->sitesy, context->count, context->dgmin.x, context->dgmax.x, context->dgmin.y, context->dgmax.y);

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
#endif

#if defined(USE_FASTJET)
int fastjet_voronoi(Context* context)
{
	fastjet::VoronoiDiagramGenerator generator;
	generator.generateVoronoi((std::vector<fastjet::VPoint>*)&context->dpoints, context->dgmin.x, context->dgmax.x, context->dgmin.y, context->dgmax.y);

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
#endif

#if defined(USE_VORONOIPP)
int voronoiplusplus_voronoi(Context* context)
{
	voronoi::VoronoiDiagram diagram;
	//diagram.removeDuplicates(context->vpp_sites);
	diagram.initialize(context->vpp_sites);
	diagram.calculate();

	if( context->collectedges )
	{
		const geometry::Rectangle rect(context->dgmin.x, context->dgmin.y, context->dgmax.x, context->dgmax.y);
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
#endif


#if defined(USE_BOOST)
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
	coordinate_type side = 2.0 * (std::max)(context->dgmax.x - context->dgmin.x,
										 context->dgmax.y - context->dgmin.y);
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
	g_RetainedDiagramSize = std::max(g_RetainedDiagramSize, g_CurrentMallocSize);
	return 0;
}

static void setup_boost_vertices(Context* context)
{
	boost::polygon::construct_voronoi(context->boost_points.begin(), context->boost_points.end(), &context->boost_vertexdiagram);
	context->boost_vertices.resize(context->boost_vertexdiagram.vertices().size());
}

static int boost_get_vertices(Context* context)
{
	size_t index = 0;
	for( boost::polygon::voronoi_diagram<double>::const_vertex_iterator it = context->boost_vertexdiagram.vertices().begin();
		 it != context->boost_vertexdiagram.vertices().end(); ++it, ++index )
	{
		context->boost_vertices[index].x = it->x();
		context->boost_vertices[index].y = it->y();
	}
	if( !context->boost_vertices.empty() )
		context->boost_vertexchecksum = context->boost_vertices.back().x;
	return 0;
}

static void teardown_boost_vertices(Context* context)
{
	context->boost_vertices.clear();
	context->boost_vertexdiagram.clear();
}
#endif

static void plot(int x, int y, unsigned char* image, int width, int height, int nchannels, unsigned char* color)
{
	if( x < 0 || y < 0 || x > (width-1) || y > (height-1) )
		return;
	int index = (height - y) * width * nchannels + x * nchannels;
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

// Remaps the point from the input space to image space
template<typename PT>
static inline PT remap(const PT& pt, const PT& min, const PT& max, const PT& scale)
{
	PT p;
	p.x = (pt.x - min.x)/(max.x - min.x) * scale.x;
	p.y = (pt.y - min.y)/(max.y - min.y) * scale.y;
	return p;
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
	printf("# Generating image: %s\n", name);
	size_t datasize = MAP_DIMENSION*MAP_DIMENSION*3;
	unsigned char* data = new unsigned char[datasize];
	memset(data, 0, datasize);

	unsigned char color_blue[] = {127, 127, 255};
	unsigned char color_white[] = {255, 255, 255};

	srand(0);

	PointF dimensions(MAP_DIMENSION, MAP_DIMENSION);

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

			const PointF& _p0 = site;
			const PointF& _p1 = det > 0 ? edges[i].first : edges[i].second;
			const PointF& _p2 = det > 0 ? edges[i].second : edges[i].first;

			PointF p0 = remap(_p0, context->dgmin, context->dgmax, dimensions);
			PointF p1 = remap(_p1, context->dgmin, context->dgmax, dimensions);
			PointF p2 = remap(_p2, context->dgmin, context->dgmax, dimensions);
			draw_triangle( p0, p1, p2, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_tri);
		}
	}

	for( size_t i = 0; i < context->collectededges.size(); ++i )
	{
		PointF p0 = remap(context->collectededges[i].first, context->dgmin, context->dgmax, dimensions);
		PointF p1 = remap(context->collectededges[i].second, context->dgmin, context->dgmax, dimensions);
		plot_line( (int)p0.x, (int)p0.y,
				   (int)p1.x, (int)p1.y, data, MAP_DIMENSION, 3, color_white);
	}

	for( int i = 0; i < context->count; ++i )
	{
		PointF p = remap(context->fsites[i], context->dgmin, context->dgmax, dimensions);
		plot( (int)p.x, (int)p.y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
		plot( (int)p.x+1, (int)p.y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
		plot( (int)p.x-1, (int)p.y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
		plot( (int)p.x, 1+(int)p.y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
		plot( (int)p.x, -1+(int)p.y, data, MAP_DIMENSION, MAP_DIMENSION, 3, color_white);
	}

	char path[512];
	snprintf(path, sizeof(path), "images/voronoi_%s_%d.png", name, context->count);
	stbi_write_png(path, MAP_DIMENSION, MAP_DIMENSION, 3, data, MAP_DIMENSION*3);
	printf("wrote %s\n", path);

	delete[] data;
}

template<typename SetupFunc, typename Func>
void generate_diagram(const char* name, Context* context, SetupFunc setupfunc, Func func)
{
	context->collectededges.clear();
	context->collectededges.resize(0);
	context->collectedcells.clear();
	context->collectedcells.resize(0);
	context->collectedges = true;

	setupfunc(context);
	func(context);

	context->collectedges = false;

	if (context->generate_images)
		output_image(name, context);
}

template<typename SetupFunc, typename Func>
void run_test(const char* implname, const char* testname, Context* context, SetupFunc setupfunc, Func func)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%s %s", implname, testname);
	buffer[sizeof(buffer)-1] = 0;

    printf("# n %d  it %d\n", context->count, context->numiterations);

	CTimeIt timeit;
	start_test(buffer, context);
	timeit.run<int>(context->numiterations, setupfunc, func, context);
	stop_test(buffer, context);

	timeit.report(std::cout, buffer, 0.0f);
	generate_diagram(implname, context, null_setup, func);
	printf("# collected %zu edges across %zu cells\n",
		   context->collectededges.size(), context->collectedcells.size());
}

template<typename Func>
void run_postprocess_test(const char* name, const char* testname, Context* context, Func func)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%s %s", name, testname);
	buffer[sizeof(buffer)-1] = 0;

	printf("# n %d  it %d\n", context->count, context->numiterations);

	CTimeIt timeit;
	start_test(buffer, context);
	timeit.run<int>(context->numiterations, null_setup, func, context);
	stop_test(buffer, context);
	timeit.report(std::cout, buffer, 0.0f);
}

int main(int argc, const char** argv)
{
	int count = 200;
	if( argc > 1 )
		count = atol(argv[1]);

	uint iterations = 20;
	if( argc > 2 )
		iterations = atol(argv[2]);

	Context context;
	context.testname = "random";
	if( argc > 3 )
		context.testname = argv[3];
	setup_sites(count, &context);
	context.numiterations = iterations;

	context.generate_images = 0;
	if( argc > 4 )
		context.generate_images = atol(argv[4]);

	std::cout << "# Generating voronoi diagrams for " << count << " sites..." << std::endl;

	fflush(stdout);


#if defined(USE_JC_VORONOI)
	run_test("jc_voronoi", context.testname, &context, null_setup, jc_voronoi);
	run_test("jc_voronoi_cell_areas", context.testname, &context, null_setup, jc_voronoi_cell_areas);
	printf("# total cell area %.17g\n", context.totalcellarea);
	setup_jc_voronoi_vertices(&context);
	run_postprocess_test("jc_voronoi_get_vertices", context.testname, &context, jc_voronoi_get_vertices);
	printf("# collected %d unique vertices\n", jcv_get_num_vertices(&context.vertexdiagram));
	teardown_jc_voronoi_vertices(&context);
#elif defined(USE_FASTJET)
	run_test("fastjet", context.testname, &context, null_setup, fastjet_voronoi);
#elif defined(USE_BOOST)
	run_test("boost", context.testname, &context, null_setup, boost_voronoi);
	setup_boost_vertices(&context);
	run_postprocess_test("boost_get_vertices", context.testname, &context, boost_get_vertices);
	printf("# collected %zu unique vertices\n", context.boost_vertexdiagram.vertices().size());
	teardown_boost_vertices(&context);
#elif defined(USE_VORONOIPP)
	run_test("voronoi++", context.testname, &context, null_setup, voronoiplusplus_voronoi);
#elif defined(USE_SHANEOSULLIVAN)
	run_test("osullivan", context.testname, &context, null_setup, shaneosullivan_voronoi);
#else
	#error "Unknown algorithm"
#endif
	fflush(stdout);

	return 0;
}
