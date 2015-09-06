/*
The MIT License (MIT)

Copyright (c) 2015 Mathias Westerdahl

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "voronoi.h"
#include <cstdlib>
#include <math.h>	// sqrt
#include <queue>
#include <assert.h>
#include <new>		// in place new

// Originally based off on http://nms.lcs.mit.edu/~aklmiu/6.838/L7.pdf
// but after more and more optimizations, the code implemented more and more of
// Steven Fortune/Shane O'Sullivan's algorithms (http://www.skynet.ie/~sos/mapviewer/voronoi.php)
//

// good reads:
// http://nms.lcs.mit.edu/~aklmiu/6.838/L7.pdf
// http://www.ams.org/samplings/feature-column/fcarc-voronoi


namespace voronoi
{

static const int DIRECTION_LEFT  = 0;
static const int DIRECTION_RIGHT = 1;
static const real_t INVALID_VALUE = real_t(-1);

struct MemoryBlock
{
	size_t sizefree;
	struct MemoryBlock* next;
	char* memory;
};

void Edge::create(Site* s1, Site* s2)
{
	Edge* e = this;
	e->next = 0;
	sites[0] = s1;
	sites[1] = s2;
	pos[0].x = INVALID_VALUE;
	pos[1].x = INVALID_VALUE;

	// Create line equation between S1 and S2:
	// real_t a = -1 * (s2->p.y - s1->p.y);
	// real_t b = s2->p.x - s1->p.x;
	// //real_t c = -1 * (s2->p.x - s1->p.x) * s1->p.y + (s2->p.y - s1->p.y) * s1->p.x;
	//
	// // create perpendicular line
	// real_t pa = b;
	// real_t pb = -a;
	// //real_t pc = pa * s1->p.x + pb * s1->p.y;
	//
	// // Move to the mid point
	// real_t mx = s1->p.x + dx * real_t(0.5);
	// real_t my = s1->p.y + dy * real_t(0.5);
	// real_t pc = ( pa * mx + pb * my );

	real_t dx = s2->p.x - s1->p.x;
	real_t dy = s2->p.y - s1->p.y;

	// Simplify it, using dx and dy
	e->c = dx * (s1->p.x + dx * real_t(0.5)) + dy * (s1->p.y + dy * real_t(0.5));

	if( fabsf(dx) > fabsf(dy) )
	{
		e->a = real_t(1);
		e->b = dy / dx;
		e->c /= dx;
	}
	else
	{
		e->a = dx / dy;
		e->b = real_t(1);
		e->c /= dy;
	}
}

bool Edge::clipline(real_t width, real_t height)
{
	Edge* e = this;

	real_t pxmin = 0;
	real_t pxmax = width;
	real_t pymin = 0;
	real_t pymax = height;

	real_t x1, y1, x2, y2;
	Point* s1;
	Point* s2;
	if (e->a == real_t(1) && e->b >= real_t(0))
	{
		s1 = e->pos[1].x != INVALID_VALUE ? &e->pos[1] : 0;
		s2 = e->pos[0].x != INVALID_VALUE ? &e->pos[0] : 0;
	}
	else
	{
		s1 = e->pos[0].x != INVALID_VALUE ? &e->pos[0] : 0;
		s2 = e->pos[1].x != INVALID_VALUE ? &e->pos[1] : 0;
	};

	if (e->a == real_t(1))
	{
		y1 = pymin;
		if (s1 != 0 && s1->y > pymin)
		{
			y1 = s1->y;
		}
		if( y1 > pymax )
		{
			y1 = pymax;
		}
		x1 = e->c - e->b * y1;
		y2 = pymax;
		if (s2 != 0 && s2->y < pymax)
			y2 = s2->y;

		if( y2 < pymin )
		{
			y2 = pymin;
		}
		x2 = (e->c) - (e->b) * y2;
		if( ((x1 > pxmax) & (x2 > pxmax)) | ((x1 < pxmin) & (x2 < pxmin)) )
		{
			return false;
		}
		if (x1 > pxmax)
		{
			x1 = pxmax;
			y1 = (e->c - x1) / e->b;
		}
		else if (x1 < pxmin)
		{
			x1 = pxmin;
			y1 = (e->c - x1) / e->b;
		}
		if (x2 > pxmax)
		{
			x2 = pxmax;
			y2 = (e->c - x2) / e->b;
		}
		else if (x2 < pxmin)
		{
			x2 = pxmin;
			y2 = (e->c - x2) / e->b;
		}
	}
	else
	{
		x1 = pxmin;
		if( s1 != 0 && s1->x > pxmin )
			x1 = s1->x;
		if( x1 > pxmax )
		{
			x1 = pxmax;
		}
		y1 = e->c - e->a * x1;
		x2 = pxmax;
		if( s2 != 0 && s2->x < pxmax )
			x2 = s2->x;
		if( x2 < pxmin )
		{
			x2 = pxmin;
		}
		y2 = e->c - e->a * x2;
		if( ((y1 > pymax) & (y2 > pymax)) | ((y1 < pymin) & (y2 < pymin)) )
		{
			return false;
		}
		if( y1 > pymax )
		{
			y1 = pymax;
			x1 = (e->c - y1) / e->a;
		}
		else if( y1 < pymin )
		{
			y1 = pymin;
			x1 = (e->c - y1) / e->a;
		}
		if( y2 > pymax )
		{
			y2 = pymax;
			x2 = (e->c - y2) / e->a;
		}
		else if( y2 < pymin )
		{
			y2 = pymin;
			x2 = (e->c - y2) / e->a;
		};
	};

	pos[0].x = x1;
	pos[0].y = y1;
	pos[1].x = x2;
	pos[1].y = y2;
    return true;
}

struct HalfEdge
{
	Edge*		edge;
	HalfEdge*	left;
	HalfEdge*	right;
	Point		vertex;
	real_t		y;
	int 		direction; // 0=left, 1=right
	int			pqpos;

	void			create(Edge* edge, int direction);
	struct Site*	leftsite() const;
	struct Site*	rightsite() const;
	bool		 	rightof(const struct Point& p) const;

	void link(HalfEdge* newedge)
	{
		newedge->left = this;
		newedge->right = this->right;
		this->right->left = newedge;
		this->right = newedge;
	}

	void unlink()
	{
		left->right = right;
		right->left = left;
		left  = 0;
		right = 0;
	}
};

void HalfEdge::create(Edge* e, int dir)
{
	edge 		= e;
	left 		= 0;
	right		= 0;
	direction 	= dir;
	pqpos		= 0;
	y			= 0;
}

struct Site* HalfEdge::leftsite() const
{
	return edge->sites[direction];
}

struct Site* HalfEdge::rightsite() const
{
	return edge ? edge->sites[1-direction] : 0;
}

bool HalfEdge::rightof(const struct Point& p) const
{
	const HalfEdge* he 		= this;
	const Edge*		e		= he->edge;
	const Site*		topsite = e->sites[1];

	bool right_of_site = p.x > topsite->p.x;
	if (right_of_site && he->direction == DIRECTION_LEFT)
		return true;
	if (!right_of_site && he->direction == DIRECTION_RIGHT)
		return false;

	real_t dxp, dyp, dxs, t1, t2, t3, yl;

	int above;
	if (e->a == real_t(1))
	{
		dyp = p.y - topsite->p.y;
		dxp = p.x - topsite->p.x;
		int fast = 0;
		if( (!right_of_site & (e->b < real_t(0))) | (right_of_site & (e->b >= real_t(0))) )
		{
			above = dyp >= e->b * dxp;
			fast = above;
		}
		else
		{
			above = p.x + p.y * e->b > e->c;
			if (e->b < real_t(0))
				above = !above;
			if (!above)
				fast = 1;
		};
		if (!fast)
		{
			dxs = topsite->p.x - e->sites[0]->p.x;
			above = e->b * (dxp * dxp - dyp * dyp)
					< dxs * dyp * (real_t(1) + real_t(2) * dxp / dxs + e->b * e->b);
			if (e->b < real_t(0))
				above = !above;
		};
	}
	else // e->b == 1
	{
		yl = e->c - e->a * p.x;
		t1 = p.y - yl;
		t2 = p.x - topsite->p.x;
		t3 = yl - topsite->p.y;
		above = t1 * t1 > t2 * t2 + t3 * t3;
	};
	return (he->direction == DIRECTION_LEFT ? above : !above);
}



static int pq_moveup(PriorityQueue* pq, int pos)
{
	const void* node = pq->items[pos];

	for( int parent = (pos >> 1);
		 pos > 1 && pq->compare(pq->items[parent], node);
		 pos = parent, parent = parent >> 1)
	{
		pq->items[pos] = pq->items[parent];
		pq->setpos( (void*)pq->items[pos], pos );
	}

	pq->items[pos] = node;
	pq->setpos( (void*)pq->items[pos], pos );
	return pos;
}

static int pq_maxchild(PriorityQueue* pq, int pos)
{
	int child = pos << 1;
	if( child >= pq->numitems )
		return 0;
	if( (child + 1) < pq->numitems && pq->compare(pq->items[child], pq->items[child+1]) )
		return child+1;
	return child;
}

static int pq_movedown(PriorityQueue* pq, int pos)
{
	const void* node = pq->items[pos];

	int child;
	while( (child = pq_maxchild(pq, pos)) &&
			pq->compare( node, pq->items[child] ) )
	{
		pq->items[pos] = pq->items[child];
		pq->setpos( (void*)pq->items[pos], pos );
		pos = child;
	}

	pq->items[pos] = node;
	pq->setpos( (void*)pq->items[pos], pos );
	return pos;
}

void pq_create(PriorityQueue* pq, int capacity, const void** buffer,
				FPriorityQueueCompare cmp,
				FPriorityQueueSetpos setpos,
				FPriorityQueueGetpos getpos)
{
	pq->compare 	= cmp;
	pq->setpos		= setpos;
	pq->getpos		= getpos;
	pq->maxnumitems = capacity;
	pq->numitems	= 1;
	pq->items 		= buffer;
}

bool pq_empty(PriorityQueue* pq)
{
	return pq->numitems == 1;
}

int pq_push(PriorityQueue* pq, const void* node)
{
	int n = pq->numitems++;
	pq->items[n] = node;
	return pq_moveup(pq, n);
}

const void* pq_pop(PriorityQueue* pq)
{
	if( pq->numitems == 1 )
		return 0;

	const void* node = pq->items[1];
	pq->items[1] = pq->items[--pq->numitems];
	pq_movedown(pq, 1);
	return node;
}

const void* pq_top(PriorityQueue* pq)
{
	if( pq->numitems == 1 )
		return 0;
	return pq->items[1];
}

void pq_remove(PriorityQueue* pq, const void* node)
{
	if( pq->numitems == 1 )
		return;
	int pos = pq->getpos(node);
	if( pos == 0 )
		return;

	pq->items[pos] = pq->items[--pq->numitems];
	if( pq->compare( node, pq->items[pos] ) )
		pq_moveup( pq, pos );
	else
		pq_movedown( pq, pos );
	pq->setpos( (void*)node, 0 );
}

void pq_print(PriorityQueue* pq, FPriorityQueuePrint printfn)
{
	printf("\tPQ\n");
	for( int i = 1; i < pq->numitems; ++i )
	{
		printfn(pq->items[i], i);
	}
	printf("\t-----\n");
}

static void he_print(const HalfEdge* he, int pos)
{
	printf("\t%g, %g  y: %g\n", he->vertex.x, he->vertex.y, he->y);
}

static inline bool he_compare( const HalfEdge* he1, const HalfEdge* he2 )
{
	return (he1->y > he2->y) || ((he1->y == he2->y) && (he1->vertex.x > he2->vertex.x));
}

static inline void he_setpos( HalfEdge* he, int pos )
{
	he->pqpos = pos;
}

static inline int he_getpos( const HalfEdge* he )
{
	return he->pqpos;
}

static inline int point_cmp(const void *p1, const void *p2)
{
	const Point& s1 = *(Point*) p1;
	const Point& s2 = *(Point*) p2;
	return (s1.y != s2.y) ? (s1.y < s2.y ? -1 : 1) : (s1.x < s2.x ? -1 : 1);
}

static inline bool pt_less( const Point& pt1, const Point& pt2 )
{
	return (pt1.y < pt2.y) || ((pt1.y == pt2.y) && (pt1.x < pt2.x));
}

static inline real_t pt_dist( const Point& pt1, const Point& pt2 )
{
	real_t dx = pt1.x - pt2.x;
	real_t dy = pt1.y - pt2.y;
	return (real_t) sqrt(dx*dx + dy*dy);
}


static void printbeach(const HalfEdge* he)
{
	printf("(%g, %g, y: %g), ", he->vertex.x, he->vertex.y, he->y );
	if( he->right )
		printbeach(he->right);
}


Voronoi::Voronoi()
{
	sites 			= 0;
	beachline_start = 0;
	beachline_end	= 0;
	edges			= 0;
	eventmem		= 0;
	eventqueue		= 0;
	memblocks		= 0;
	edgepool		= 0;
	halfedgepool	= 0;
}


void Voronoi::cleanup()
{
	while(memblocks)
	{
		struct MemoryBlock* p = memblocks;
		memblocks = memblocks->next;
		//free(p);
		delete[] p;
	}

	sites 			= 0;
	beachline_start = 0;
	beachline_end	= 0;
	edges			= 0;
	eventqueue		= 0;
	eventmem		= 0;
}

Voronoi::~Voronoi()
{
	cleanup();
}

Site* Voronoi::nextsite()
{
	return (currentsite < numsites) ? &sites[currentsite++] : 0;
}

void Voronoi::generate(size_t num_sites, const Point* _sites, int _width, int _height)
{
	cleanup();
	beachline_start = alloc_halfedge();
	beachline_end	= alloc_halfedge();

	beachline_start->left 	= 0;
	beachline_start->right 	= beachline_end;
	beachline_end->left		= beachline_start;
	beachline_end->right	= 0;

	last_inserted = 0;

	//int max_num_events = num_sites * 2 + 1;
	int max_num_events = sqrt(num_sites) * 4;
	size_t sitessize = num_sites * sizeof(Site);
	size_t memsize = max_num_events * sizeof(void*) + sizeof(PriorityQueue) + sitessize;

	char* mem = (char*)alloc(memsize);

	sites = (Site*) mem;
	mem += sitessize;

	eventqueue = (PriorityQueue*)mem;
	mem += sizeof(PriorityQueue);

	eventmem = (void**) mem;

	//eventqueue = (PriorityQueue*)malloc(sizeof(PriorityQueue));
	//eventqueue = new PriorityQueue;
	pq_create(eventqueue, max_num_events, (const void**)eventmem,
				(FPriorityQueueCompare)he_compare,
				(FPriorityQueueSetpos)he_setpos,
				(FPriorityQueueGetpos)he_getpos);

	width = _width;
	height = _height;

	for( size_t i = 0; i < num_sites; ++i )
	{
		sites[i].p 		= _sites[i];
        sites[i].edges 	= 0;
	}

	// Remove duplicates, to avoid anomalies
	qsort(sites, num_sites, sizeof(Site), point_cmp);

	unsigned int offset = 0;
	for (int is = 1; is < num_sites; is++)
	{
		if( sites[is].p.y == sites[is - 1].p.y && sites[is].p.x == sites[is - 1].p.x )
		{
			offset++;
			continue;
		}
		else if (offset > 0)
		{
			sites[is - offset] = sites[is];
		}
	}
	num_sites 		-= offset;
	numsites 		= num_sites;
	numsites_sqrt	= sqrt(num_sites);
	currentsite 	= 0;

	bottomsite = nextsite();

	Site* site = nextsite();

	while( true )
	{
		Point lowest_pq_point;
		if( !pq_empty(eventqueue) )
		{
			HalfEdge* he = (HalfEdge*)pq_top(eventqueue);
			lowest_pq_point.x = he->vertex.x;
			lowest_pq_point.y = he->y;
		}

		if( site != 0 && (pq_empty(eventqueue) || pt_less(site->p, lowest_pq_point) ) )
		{
			site_event(site);
			site = nextsite();
		}
		else if( !pq_empty(eventqueue) )
		{
			circle_event();
		}
		else
		{
			break;
		}
	}

	for( struct HalfEdge* he = beachline_start->right; he != beachline_end; he = he->right )
	{
		finishline(he->edge);
	}
}

void Voronoi::finishline(Edge* e)
{
	if( !e->clipline(width, height) )
		return;

	for( int i = 0; i < 2; ++i )
	{
        GraphEdge* ge = alloc_graphedge();

		ge->edge = e;
		ge->next = 0;
		ge->neighbor = e->sites[1-i];
		ge->pos[0] = e->pos[i];
		ge->pos[1] = e->pos[1-i];

		ge->next = e->sites[i]->edges;
		e->sites[i]->edges = ge;
	}
}

void* Voronoi::alloc(size_t size)
{
	if( !memblocks || memblocks->sizefree < size )
	{
		size_t blocksize = 16 * 1024;
		if( size + sizeof(MemoryBlock) > blocksize )
			blocksize = size + sizeof(MemoryBlock);
		//struct MemoryBlock* block = (struct MemoryBlock*)malloc(blocksize);
		struct MemoryBlock* block = (struct MemoryBlock*)new char[blocksize];
		size_t offset = sizeof(MemoryBlock);
		block->sizefree = blocksize - offset;
		block->next = memblocks;
		block->memory = ((char*)block) + offset;
		memblocks = block;
	}
	void* p = memblocks->memory;
	memblocks->memory += size;
	memblocks->sizefree -= size;
	assert(p != 0);
	return p;
}


struct Edge* Voronoi::alloc_edge()
{
	if( edgepool )
	{
		Edge* edge = edgepool;
		edgepool = edgepool->next;
		edge = new(edge) Edge;
		return edge;
	}

	Edge* edge = (struct Edge*)alloc(sizeof(struct Edge));
	edge = new(edge) Edge;
	//memset(edge, 0, sizeof(struct Edge));
	return edge;
	//return (struct Edge*)alloc(sizeof(struct Edge));
	//Edge* edge = new Edge;
	//memset(edge, 0, sizeof(struct Edge));
	//return edge;
}

struct HalfEdge* Voronoi::alloc_halfedge()
{
	if( halfedgepool )
	{
		HalfEdge* edge = halfedgepool;
		halfedgepool = halfedgepool->right;
		return edge;
	}

	HalfEdge* edge = (struct HalfEdge*)alloc(sizeof(struct HalfEdge));
	memset(edge, 0, sizeof(struct HalfEdge));
	edge = new(edge) HalfEdge;
	return edge;

	//return (struct HalfEdge*)alloc(sizeof(struct HalfEdge));
	//HalfEdge* edge = new HalfEdge;
	//memset(edge, 0, sizeof(struct HalfEdge));
	//return edge;
}

struct GraphEdge* Voronoi::alloc_graphedge()
{
	return (struct GraphEdge*)alloc(sizeof(struct GraphEdge));
}

const struct Site* Voronoi::get_cells() const
{
	return sites;
}

const struct Edge* Voronoi::get_edges() const
{
	return edges;
}

void Voronoi::site_event(struct Site* site)
{
	HalfEdge* left 	 = get_edge_above_x(site->p);
	HalfEdge* right	 = left->right;
	Site*	  bottom = left->rightsite();
	if( !bottom )
		bottom = bottomsite;

	Edge* edge = new_edge(bottom, site);
	edge->next = edges;
	edges = edge;

	HalfEdge* edge1 = new_halfedge(edge, DIRECTION_LEFT);
	HalfEdge* edge2 = new_halfedge(edge, DIRECTION_RIGHT);

	HalfEdge* leftleft = left->left;

	left->link(edge1);
	edge1->link(edge2);

	last_inserted = edge1;

	Point p;
	if( check_circle_event( left, edge1, p ) )
	{
		pq_remove(eventqueue, left);
		left->vertex 	= p;
		left->y		 	= p.y + pt_dist(site->p, p);
		pq_push(eventqueue, left);
	}
	if( check_circle_event( edge2, right, p ) )
	{
		edge2->vertex	= p;
		edge2->y		= p.y + pt_dist(site->p, p);
		pq_push(eventqueue, edge2);
	}
}

void Voronoi::circle_event()
{
	HalfEdge* left 		= (HalfEdge*)pq_pop(eventqueue);

	HalfEdge* leftleft 	= left->left;
	HalfEdge* right		= left->right;
	HalfEdge* rightright= right->right;
	Site* bottom = left->leftsite();
	Site* top 	 = right->rightsite();

	Point vertex = left->vertex;
	endpos(left->edge, vertex, left->direction);
	endpos(right->edge, vertex, right->direction);

	if( last_inserted == left )
		last_inserted = leftleft;
	else if( last_inserted == right )
		last_inserted = rightright;

	pq_remove(eventqueue, right);
	left->unlink();
	right->unlink();
    delete_halfedge(left);
    delete_halfedge(right);

	int direction = DIRECTION_LEFT;
	if( bottom->p.y > top->p.y )
	{
		Site* temp = bottom;
		bottom = top;
		top = temp;
		direction = DIRECTION_RIGHT;
	}

	Edge* edge = new_edge(bottom, top);
	edge->next = edges;
	edges = edge;

	HalfEdge* he = new_halfedge(edge, direction);
	leftleft->link(he);
	endpos(edge, vertex, DIRECTION_RIGHT - direction);

	Point p;
	if( check_circle_event( leftleft, he, p ) )
	{
		pq_remove(eventqueue, leftleft);
		leftleft->vertex 	= p;
		leftleft->y		 	= p.y + pt_dist(bottom->p, p);
		pq_push(eventqueue, leftleft);
	}
	if( check_circle_event( he, rightright, p ) )
	{
		he->vertex 		= p;
		he->y		 	= p.y + pt_dist(bottom->p, p);
		pq_push(eventqueue, he);
	}
}

void Voronoi::endpos(struct Edge* e, const Point& p, int direction)
{
	e->pos[direction] = p;

	if( e->pos[0].x != real_t(-1) && e->pos[1].x != real_t(-1) )
	{
		finishline(e);

		// TODO: Add to site
		//delete_edge(e);
		//e->next = edges;
		//edges = e;
	}
}

bool Voronoi::edge_intersect(const struct HalfEdge& he1, const struct HalfEdge& he2, Point& out) const
{
	const Edge& e1 = *he1.edge;
	const Edge& e2 = *he2.edge;

	real_t dx = e2.sites[1]->p.x - e1.sites[1]->p.x;
	real_t dy = e2.sites[1]->p.y - e1.sites[1]->p.y;

	if( dx == 0 && dy == 0 )
	{
		return false;
	}

	real_t d = e1.a * e2.b - e1.b * e2.a;
	if( fabsf(d) < real_t(0.00001f) )
	{
		return false;
	}
	out.x = (e1.c * e2.b - e1.b * e2.c) / d;
	out.y = (e1.a * e2.c - e1.c * e2.a) / d;

	const Edge* e;
	const HalfEdge* he;
	if( pt_less( e1.sites[1]->p, e2.sites[1]->p) )
	{
		he = &he1;
		e = &e1;
	}
	else
	{
		he = &he2;
		e = &e2;
	}

	int right_of_site = out.x >= e->sites[1]->p.x;
	if ((right_of_site && he->direction == DIRECTION_LEFT) || (!right_of_site && he->direction == DIRECTION_RIGHT))
	{
		return false;
	}

	return true;
}

bool Voronoi::check_circle_event(struct HalfEdge* he1, struct HalfEdge* he2, Point& vertex)
{
	Edge* e1 = he1->edge;
	Edge* e2 = he2->edge;
	if( e1 == 0 || e2 == 0 || e1->sites[1] == e2->sites[1] )
	{
		return false;
	}

	return edge_intersect(*he1, *he2, vertex);
}

struct HalfEdge* Voronoi::get_edge_above_x(const Point& p)
{
	// Gets the arc on the beach line at the x coordinate (i.e. right above the new site event)

	// A good guess it's close by (Can be optimized)
	HalfEdge* he = last_inserted;
    if( !he )
    {
        if( p.x < width / 2 )
            he = beachline_start;
        else
            he = beachline_end;
    }

	//
	if( he == beachline_start || (he != beachline_end && he->rightof(p)) )
	{
		do {
			he = he->right;
		}
		while( he != beachline_end && he->rightof(p) );

		he = he->left;
	}
	else
	{
		do {
			he = he->left;
		}
		while( he != beachline_start && !he->rightof(p) );
	}

	return he;
}


struct Edge* Voronoi::new_edge(struct Site* s1, struct Site* s2)
{
	struct Edge* e = alloc_edge();
	e->create(s1, s2);
	return e;
}

void Voronoi::delete_edge(struct Edge* e)
{
	e->next = edgepool;
	edgepool = e;
}

struct HalfEdge* Voronoi::new_halfedge(struct Edge* e, int direction)
{
	struct HalfEdge* he = alloc_halfedge();
	memset(he, 0, sizeof(struct HalfEdge));
	he->create(e, direction);
	return he;
}

void Voronoi::delete_halfedge(struct HalfEdge* he)
{
	he->right = halfedgepool;
    halfedgepool = he;
}


/*
#include "stb_image_write.h"

static void plot(int x, int y, unsigned char* image, int dimension, int nchannels, const unsigned char* color)
{
	if( x < 0 || y < 0 || x > (dimension-1) || y > (dimension-1) )
		return;
	int index = y * dimension * nchannels + x * nchannels;
	for( int i = 0; i < nchannels; ++i )
	{
		image[index+i] = color[i];
	}
}

static void plot_line(int x0, int y0, int x1, int y1, unsigned char* image, int dimension, int nchannels, const unsigned char* color)
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

static inline real_t get_y_from_parabola(real_t cx, real_t cy, real_t x, real_t directrix)
{
	real_t dp = 2 * (cy - directrix);
	if( dp == 0.0f )
		return 100000;
	real_t a1 = 1 / dp;
	real_t b1 = -2 * cx / dp;
	real_t c1 = directrix + dp / 4 + cx * cx / dp;

	return(a1*x*x + b1*x + c1);
}

static void draw_parabola( int cx, int cy, int directrix, unsigned char* image, int image_width, int image_height, int nchannels, const unsigned char* color )
{
	for( int x = 0; x < image_width; ++x )
	{
		real_t y = get_y_from_parabola(cx, cy, x, directrix);
		plot(x, int(y), image, image_width, nchannels, color);
	}
}

static void plot_tree( Arc* arc, real_t sweepline, unsigned char* image, int width, int height)
{
	unsigned char color_leaf_arc[] = {127, 255, 255};
	unsigned char color_edge_arc[] = {127, 127, 127};
	unsigned char color_pt[] = {255, 255, 255};

	Site* s = arc->site ? arc->site : arc->edge->leftsite();
	int x = s->p.x;
	int y = s->p.y;
	plot( x, y, image, width, 3, color_pt);

	if( arc->isleaf() )
	{
		draw_parabola( x, y, sweepline, image, width, height, 3, color_leaf_arc);
	}
	else
	{
		draw_parabola( x, y, sweepline, image, width, height, 3, color_leaf_arc);
	}

	if( arc->isleaf() )
		return;

	plot_tree( arc->left, sweepline, image, width, height );
	plot_tree( arc->right, sweepline, image, width, height );
}

void Voronoi::debugdraw()
{
	if( beachline == 0 )
		return;

	static int counter = 0;

	size_t datasize = width*height*3;
	unsigned char* data = new unsigned char[datasize];
	memset(data, 0, datasize);

	unsigned char color_blue[] = {127, 127, 255};
	unsigned char color_white[] = {255, 255, 255};

	plot_line(0, sweepline, width, sweepline, data, width, 3, color_blue);

	plot_tree(beachline, sweepline, data, width, height);

	char path[512];
	sprintf(path, "debugout_%d.png", counter++);
	stbi_write_png(path, width, height, 3, data, width*3);
	printf("wrote %s\n", path);

	delete[] data;
}
*/

} // namespace

