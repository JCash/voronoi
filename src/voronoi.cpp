#include "voronoi.h"
#include <cstdlib>
#include <math.h>	// sqrt
#include <queue>
#include <assert.h>

#if __cplusplus > 199711L
#include <tuple>
#endif

#define DEBUG  printf

// Based off on http://nms.lcs.mit.edu/~aklmiu/6.838/L7.pdf

// good read: http://www.ams.org/samplings/feature-column/fcarc-voronoi

namespace voronoi
{

static const int DIRECTION_LEFT  = 0;
static const int DIRECTION_RIGHT = 1;

struct Event
{
	Site*		site;	// is this a site event (site != 0)? If not, it's a circle event
	struct Arc* arc;	// the arc causing the circle event
	Point		p;		// If a circle event, the center of the circle
	real_t		y;		// The y of the event. For site events, it's the sites' y value. For circle events, it's the top y value of the circle.
	bool		deleted;

	void create(const Point& _p, real_t _y)
	{
		site 	= 0;
		arc 	= 0;
		p 		= _p;
		y 		= _y;
		deleted = false;
	}
};


struct Arc
{
	struct Arc*	parent;
	struct Arc*	left;
	struct Arc*	right;

	// internal node
	HalfEdge*	edge;

	// Leaf node
	Site*		site;
	Event*		circleevent;

	void create(Site* _site);
	void setleft(struct Arc* child)		{ left = child; child->parent = this; }
	void setright(struct Arc* child)	{ right = child; child->parent = this; }
	bool isleaf() const					{ return left == 0 && right == 0; }
};

void Arc::create(Site* s)
{
	memset(this, 0, sizeof(Arc));
	site 	= s;
}

static const Arc* arc_get_left_child(const Arc* a)
{
	if(!a) return 0;
	const Arc* arc = a->left;
	while(!arc->isleaf()) arc = arc->right;
	return arc;
}

static const Arc* arc_get_right_child(const Arc* a)
{
	if(!a) return 0;
	const Arc* arc = a->right;
	while(!arc->isleaf()) arc = arc->left;
	return arc;
}

static const Arc* arc_get_left_parent(const Arc* a)
{
	const Arc* parent = a->parent;
	const Arc* last	= a;
	while(parent->left == last)
	{
		if(!parent->parent) return 0;
		last = parent;
		parent = parent->parent;
	}
	return parent;
}

static const Arc* arc_get_right_parent(const Arc* a)
{
	const Arc* parent = a->parent;
	const Arc* last	= a;
	while(parent->right == last)
	{
		if(!parent->parent) return 0;
		last = parent;
		parent = parent->parent;
	}
	return parent;
}

bool CompareEvent::operator()(const struct Event* l, const struct Event* r) const
{
	return (l->y != r->y) ? (l->y > r->y) : (l->p.x > r->p.x);
}

void Edge::create(Site* s1, Site* s2)
{
	Edge* e = this;
	e->next = 0;
	sites[0] = s1;
	sites[1] = s2;
	pos[0].x = real_t(-1);
	pos[1].x = real_t(-1);

	real_t dx = s2->p.x - s1->p.x;
	real_t dy = s2->p.y - s1->p.y;
	real_t adx = fabsf(dx);
	real_t ady = fabsf(dy);

	e->c = s1->p.x * dx + s1->p.y * dy + (dx*dx + dy*dy) * real_t(0.5);

	if( adx > ady )
	{
		e->a = real_t(1.0);
		e->b = dy / dx;
		e->c = e->c / dx;
	}
	else
	{
		e->a = dx / dy;
		e->b = real_t(1.0);
		e->c = e->c / dy;
	}
}

void Edge::clipline(real_t width, real_t height)
{
	Edge* e = this;

	real_t x1 = e->sites[0]->p.x;
	real_t y1 = e->sites[0]->p.y;
	real_t x2 = e->sites[1]->p.x;
	real_t y2 = e->sites[1]->p.y;

	//if the distance between the two points this line was created from is less than
	//the square root of 2, then ignore it
	//TODO improve/remove
	//if(sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1))) < minDistanceBetweenSites)
	//  {
	//    return;
	//  }
	real_t pxmin = 0;
	real_t pxmax = width;
	real_t pymin = 0;
	real_t pymax = height;

	Point* s1;
	Point* s2;
	if (e->a == 1.0 && e->b >= 0.0)
	{
		s1 = e->pos[1].x != real_t(-1) ? &e->pos[1] : 0;
		s2 = e->pos[0].x != real_t(-1) ? &e->pos[0] : 0;
	}
	else
	{
		s1 = e->pos[0].x != real_t(-1) ? &e->pos[0] : 0;
		s2 = e->pos[1].x != real_t(-1) ? &e->pos[1] : 0;
	};

	//printf("clip_line\n");
	//printf("\ta, b, c: %f, %f, %f\n", e->a, e->b, e->c);
	//printf("\t(x1, y1), (x2, y2): (%f, %f), (%f, %f)\n", x1, y1, x2, y2);

	//if (s1)
	//	printf("\ts1, %f, %f\n", s1->x, s1->y);
	//if (s2)
	//	printf("\ts2, %f, %f\n", s2->x, s2->y);

	//printf("\ts1, s2: %p  %p\n", s1, s2);

	if (e->a == 1.0)
	{
		y1 = pymin;
		if (s1 != 0 && s1->y > pymin)
		{
			y1 = s1->y;
		}
		if (y1 > pymax)
		{
			//printf("\nClipped (1) y1 = %f to %f", y1, pymax);
			y1 = pymax;
			//return;
		}
		x1 = e->c - e->b * y1;
		y2 = pymax;
		if (s2 != 0 && s2->y < pymax)
			y2 = s2->y;

		if (y2 < pymin)
		{
			//printf("\nClipped (2) y2 = %f to %f", y2, pymin);
			y2 = pymin;
			//return;
		}
		x2 = (e->c) - (e->b) * y2;
		if (((x1 > pxmax) & (x2 > pxmax)) | ((x1 < pxmin) & (x2 < pxmin)))
		{
			//printf("\nClipLine jumping out(3), x1 = %f, pxmin = %f, pxmax = %f\n", x1, pxmin, pxmax);
			return;
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
		if (s1 != 0 && s1->x > pxmin)
			x1 = s1->x;
		if (x1 > pxmax)
		{
			//printf("\nClipped (3) x1 = %f to %f", x1, pxmin);
			x1 = pxmax;
		}
		y1 = e->c - e->a * x1;
		x2 = pxmax;
		if (s2 != 0 && s2->x < pxmax)
			x2 = s2->x;
		if (x2 < pxmin)
		{
			//printf("Clipped (4) x2 = %f to %f\n", x2, pxmin);
			x2 = pxmin;
		}
		y2 = e->c - e->a * x2;
		if (((y1 > pymax) & (y2 > pymax)) | ((y1 < pymin) & (y2 < pymin)))
		{
			//printf("\nClipLine jumping out(6), y1 = %f, pymin = %f, pymax = %f", y2, pymin, pymax);
			return;
		}
		if (y1 > pymax)
		{
			y1 = pymax;
			x1 = (e->c - y1) / e->a;
		}
		else if (y1 < pymin)
		{
			y1 = pymin;
			x1 = (e->c - y1) / e->a;
		}
		if (y2 > pymax)
		{
			y2 = pymax;
			x2 = (e->c - y2) / e->a;
		}
		else if (y2 < pymin)
		{
			y2 = pymin;
			x2 = (e->c - y2) / e->a;
		};
	};

	pos[0].x = x1;
	pos[0].y = y1;
	pos[1].x = x2;
	pos[1].y = y2;
}

struct HalfEdge
{
	Edge*		edge;
	HalfEdge*	next;
	int 		direction; // 0=left, 1=right

	void			create(Edge* edge, int direction);
	struct Site*	leftsite() const;
	struct Site*	rightsite() const;
	bool		 	rightof(const struct Point& p) const;
};

void HalfEdge::create(Edge* e, int dir)
{
	edge = e;
	next = 0;
	direction = dir;
}

struct Site* HalfEdge::leftsite() const
{
	return edge->sites[direction];
}

struct Site* HalfEdge::rightsite() const
{
	return edge->sites[1-direction];
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
	if (e->a == 1.0)
	{
		dyp = p.y - topsite->p.y;
		dxp = p.x - topsite->p.x;
		int fast = 0;
		if ((!right_of_site & (e->b < 0.0)) | (right_of_site & (e->b >= 0.0)))
		{
			above = dyp >= e->b * dxp;
			fast = above;
		}
		else
		{
			above = p.x + p.y * e->b > e->c;
			if (e->b < 0.0)
				above = !above;
			if (!above)
				fast = 1;
		};
		if (!fast)
		{
			dxs = topsite->p.x - e->sites[0]->p.x;
			above = e->b * (dxp * dxp - dyp * dyp)
					< dxs * dyp * (1.0 + 2.0 * dxp / dxs + e->b * e->b);
			if (e->b < 0.0)
				above = !above;
		};
	}
	else /*e->b==1.0 */
	{
		yl = e->c - e->a * p.x;
		t1 = p.y - yl;
		t2 = p.x - topsite->p.x;
		t3 = yl - topsite->p.y;
		above = t1 * t1 > t2 * t2 + t3 * t3;
	};
	return (he->direction == DIRECTION_LEFT ? above : !above);
}

static void print_tree(const Arc* arc, const char* name, int indent)
{
	for( int i = 0; i < indent; ++i )
		printf("    ");

	if( arc->isleaf() )
	{
		//printf("%s %d: %f, %f\n", name, arc->site->id, arc->site->p.x, arc->site->p.y);
		return;
	}
	else
	{
		//printf("%s (%d, %d)\n", name, arc->edge->edge->sites[arc->edge->direction]->id, arc->edge->edge->sites[1-arc->edge->direction]->id);
	}

	if( arc->left)
	{
		printf("\n");
		print_tree(arc->left, "L", indent+1);
	}
	if( arc->right )
	{
		printf("\n");
		print_tree(arc->right, "R", indent+1);
	}
}

Voronoi::Voronoi()
{
	sites 		= 0;
	beachline 	= 0;
	edges		= 0;
}


void Voronoi::cleanup()
{
	if( sites )
	{
		delete[] sites;
		beachline = 0;
	}
}

Voronoi::~Voronoi()
{
	cleanup();
}

static int point_cmp(const void *p1, const void *p2)
{
	const Point* s1 = (Point*) p1;
	const Point* s2 = (Point*) p2;
	if (s1->y < s2->y)
		return (-1);
	if (s1->y > s2->y)
		return (1);
	if (s1->x < s2->x)
		return (-1);
	if (s1->x > s2->x)
		return (1);
	return (0);
}

void Voronoi::generate(size_t num_sites, const Point* _sites, int _width, int _height)
{
	cleanup();

	edges 		= 0;
	beachline 	= 0;
	sites 	  	= new Site[num_sites];

	size_t max_num_edges = num_sites * 3 - 6;

	edgemem		= new Edge[max_num_edges];
	edgepool	= edgemem;
	for( int i = 0; i < max_num_edges-1; ++i )
	{
		edgemem[i].next = &edgemem[i+1];
	}
	edgemem[max_num_edges-1].next = 0;


	size_t max_num_arcs = num_sites * 4;

	arcmem 		= new Arc[max_num_arcs];
	arcpool 	= arcmem;

	for( int i = 0; i < max_num_arcs-1; ++i )
	{
		arcmem[i].right = &arcmem[i+1];
	}
	arcmem[max_num_arcs-1].right = 0;



	size_t max_num_halfedges = max_num_edges * 2;

	halfedgemem 	= new HalfEdge[max_num_halfedges];
	halfedgepool 	= halfedgemem;

	for( int i = 0; i < max_num_halfedges-1; ++i )
	{
		halfedgemem[i].next = &halfedgemem[i+1];
	}
	halfedgemem[max_num_halfedges-1].next = 0;


	size_t max_num_events = num_sites * 2;

	eventmem		= new Event[max_num_events];
	eventpool		= eventmem;
	for( int i = 0; i < max_num_events-1; ++i )
	{
		eventmem[i].arc = (Arc*)&eventmem[i+1];
	}
	eventmem[max_num_events-1].arc = 0;


	for( size_t i = 0; i < num_sites; ++i )
	{
		const Point& p = _sites[i];
		sites[i].p 	= p;
		//sites[i].id = (int)i;
        sites[i].edges = 0;
	}

	// Remove duplicates, to avoid anomalies
	qsort(sites, num_sites, sizeof(Site), point_cmp);

	Event* e = new_event(sites[0].p, sites[0].p.y);
	e->site  = &sites[0];
	eventqueue.push( e );

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

		int newindex = is - offset;
		Event* e = new_event(sites[newindex].p, sites[newindex].p.y);
		e->site  = &sites[newindex];
		eventqueue.push( e );
	}
	num_sites -= offset;

	width = _width;
	height = _height;

	int i = 0;
	while( !eventqueue.empty() )
	{
		const Event* event = eventqueue.top();
		eventqueue.pop();

		//DEBUG("LOOP: (%f, %f),  %f, %s %s\n", event->p.x, event->p.y, event->y, event->site ? "site":"circle", event->deleted ? "deleted":"");

		if( event->deleted )
			continue;

		//debugdraw();

		sweepline = event->y;
		//DEBUG("Sweepline: %f\n", sweepline);

		if( event->site )
		{
			site_event(event);
		}
		else
		{
			circle_event(event);
		}

		delete_event( (Event*)event );
	}

	Edge* edge = edges;
	while(edge)
	{
		edge->clipline(width, height);
		edge = edge->next;
	}
}

const struct Site* Voronoi::get_cells() const
{
	return sites;
}

const struct Edge* Voronoi::get_edges() const
{
	return edges;
}


void Voronoi::site_event(const struct Event* event)
{
	if( beachline == 0 )
	{
		beachline = new_arc(event->site);
		return;
	}

	//DEBUG("%s  event %f, %f\n", __FUNCTION__, event->p.x, event->p.y);

	// Get the arc directly above
	Arc* arc = get_arc_from_x(event->p);

	if( arc->circleevent )
	{
		arc->circleevent->deleted = true;
		arc->circleevent = 0;
	}

	Arc* p0 = new_arc(arc->site);
	Arc* pp = new_arc(0);
	Arc* p1 = new_arc(event->site);
	Arc* p2 = new_arc(arc->site);
	pp->setleft(p1);
	pp->setright(p2);
	arc->setleft(p0);
	arc->setright(pp);

	Edge* edge = new_edge(arc->site, event->site);
	edge->next = edges;
	edges = edge;

	HalfEdge* edge1 = new_halfedge(edge, DIRECTION_LEFT);
	HalfEdge* edge2 = new_halfedge(edge, DIRECTION_RIGHT);
	arc->edge = edge1;
	pp->edge = edge2;


	//printf("\tTree\n");
	//print_tree(beachline, "B", 1);
	//printf("\n");

	if( !check_circle_event( p0 ) )
	{
		//printf("\tNO circle_event added from site  %f, %f\n", p0->site->p.x, p0->site->p.y);
	}
	if( !check_circle_event( p2 ) )
	{
		//printf("\tNO circle_event added from site  %f, %f\n", p2->site->p.x, p2->site->p.y);
	}
}

void Voronoi::circle_event(const struct Event* event)
{
	Arc* a1 = event->arc;
	Arc* pl = (Arc*)arc_get_left_parent(a1);
	Arc* pr = (Arc*)arc_get_right_parent(a1);

	Arc* a0 = (Arc*)arc_get_left_child(pl);
	Arc* a2 = (Arc*)arc_get_right_child(pr);

	if( a0->circleevent )
	{
		a0->circleevent->deleted = true;
		a0->circleevent = 0;
	}
	if( a2->circleevent )
	{
		a2->circleevent->deleted = true;
		a2->circleevent = 0;
	}

	endpos(pl->edge, event->p, pl->edge->direction);
	endpos(pr->edge, event->p, pr->edge->direction);

	Arc* bottom = (Arc*)a0;
	Arc* top    = (Arc*)a2;
	int direction = DIRECTION_LEFT;

	if( bottom->site->p.y > top->site->p.y )
	{
		Arc* temp = bottom;
		bottom = top;
		top = temp;
		direction = DIRECTION_RIGHT;
	}

	Edge* edge = new_edge(bottom->site, top->site);

	edge->next = edges;
	edges = edge;

	if(top->edge)
		delete_halfedge(top->edge);

	Arc* higher;
	Arc* par = a1;
	while(par != beachline)
	{
		par = par->parent;
		if(par == pl) higher = pl;
		if(par == pr) higher = pr;
	}
	higher->edge = new_halfedge(edge, direction);
	endpos(higher->edge, event->p, 1-direction);

	// relink hierarchy
	Arc* grandparent = a1->parent->parent;
	if( a1->parent->left == a1 )
	{
		if( grandparent->left == a1->parent )	grandparent->setleft(a1->parent->right);
		else if( grandparent->right == a1->parent )	grandparent->setright(a1->parent->right);
	}
	else
	{
		if( grandparent->left == a1->parent )	grandparent->setleft(a1->parent->left);
		else if( grandparent->right == a1->parent )	grandparent->setright(a1->parent->left);
	}

	delete_arc(a1->parent);
	delete_arc(a1);

	check_circle_event( (Arc*)a0 );
	check_circle_event( (Arc*)a2 );
}

void Voronoi::endpos(struct HalfEdge* he, const Point& p, int direction)
{
	Edge* e = he->edge;
	e->pos[direction] = p;

	if( e->pos[0].x != real_t(-1) && e->pos[1].x != real_t(-1) )
	{
		e->clipline(width, height);

		// TODO: Add to site

		//e->next = edges;
		//edges = e;
	}
}

static const Arc* arc_get_left_sibling(const Arc* a)
{
	const Arc* parent = a->parent;
	const Arc* last	= a;
	while( parent->left == last )
	{
		if( !parent->parent )
			return 0;
		last = parent;
		parent = parent->parent;
	}

	parent = parent->left;
	while(!parent->isleaf()) parent = parent->right;
	return parent;
}

static const Arc* arc_get_right_sibling(const Arc* a)
{
	const Arc* parent = a->parent;
	const Arc* last	= a;
	while( parent->right == last )
	{
		if( !parent->parent )
			return 0;
		last = parent;
		parent = parent->parent;
	}

	parent = parent->right;
	while(!parent->isleaf()) parent = parent->left;
	return parent;
}

static void find_circle(const Point& v1, const Point& v2, const Point& v3, Point& out, real_t& radiussq)
{
    float bx = v1.x; float by = v1.y;
    float cx = v2.x; float cy = v2.y;
    float dx = v3.x; float dy = v3.y;
    float temp = cx*cx+cy*cy;
    float bc = (bx*bx + by*by - temp)/2.0;
    float cd = (temp - dx*dx - dy*dy)/2.0;
    float det = (bx-cx)*(cy-dy)-(cx-dx)*(by-cy);
    if(fabs(det) < 1.0e-6)
    {
    	out = v2;
    	radiussq = real_t(0);
        return;
    }
    det = 1/det;
    out.x = (bc*(cy-dy)-cd*(by-cy))*det;
    out.y = ((bx-cx)*cd-(cx-dx)*bc)*det;
    radiussq = (out.x-bx)*(out.x-bx)+(out.y-by)*(out.y-by);
}

bool Voronoi::edge_intersect(const struct HalfEdge& he1, const struct HalfEdge& he2, Point& out) const
{
	const Edge& e1 = *he1.edge;
	const Edge& e2 = *he2.edge;

	/*
	real_t dx = e2.sites[1]->p.x - e1.sites[1]->p.x;
	real_t dy = e2.sites[1]->p.y - e1.sites[1]->p.y;

	printf("\tintersect  %f, %f  -> %f, %f\n", e1.sites[1]->p.x, e1.sites[1]->p.y, e2.sites[1]->p.x, e2.sites[1]->p.y);

	if( dx == 0 && dy == 0 )
	{
		printf("\t\tearly out 1\n");
		return false;
	}

	real_t adx = dx > 0 ? dx : -dx;
	real_t ady = dy > 0 ? dy : -dy;

	real_t c = e1.sites[1]->p.x * dx + e1.sites[1]->p.y * dy + (dx * dx + dy * dy) * real_t(0.5);
	real_t a, b;
	if( adx > ady )
	{
		a = 1.0;
		b = dy / dx;
		c /= dx;
	}
	else
	{
		b = 1.0;
		a = dx / dy;
		c /= dy;
	}
	real_t d = e1.a * b - e1.b * a;
	if (-1.0e-10 < d && d < 1.0e-10)
	{
		printf("\t\tearly out 2: d == %f\n", d);
		return false;
	}
	*/

	real_t dx = e2.sites[1]->p.x - e1.sites[1]->p.x;
	real_t dy = e2.sites[1]->p.y - e1.sites[1]->p.y;
	real_t dxref = e1.sites[1]->p.x - e1.sites[0]->p.x;
	real_t dyref = e1.sites[1]->p.y - e1.sites[0]->p.y;

	//printf("\tintersect  (%f,%f),(%f,%f)  v  (%f,%f),(%f,%f)\n",
	//		e1.sites[0]->p.x, e1.sites[0]->p.y,
	//		e1.sites[1]->p.x, e1.sites[1]->p.y,
	//		e2.sites[0]->p.x, e2.sites[0]->p.y,
	//		e2.sites[1]->p.x, e2.sites[1]->p.y);

	if( dx == 0 && dy == 0 )
	{
		//printf("\t\tearly out 1\n");
		return false;
	}

	if (dx * dx + dy * dy < 1e-14 * (dxref * dxref + dyref * dyref))
	{
		// make sure that the difference is positive
		real_t adx = dx > 0 ? dx : -dx;
		real_t ady = dy > 0 ? dy : -dy;

		// get the slope of the line
		real_t a, b;
		real_t c = (real_t) (e1.sites[1]->p.x * dx + e1.sites[1]->p.y * dy + (dx * dx + dy * dy) * 0.5);

		if (adx > ady)
		{
			a = 1.0;
			b = dy / dx;
			c /= dx;
		}
		else
		{
			b = 1.0;
			a = dx / dy;
			c /= dy;
		}

		real_t d = e1.a * b - e1.b * a;
		if (-1.0e-10 < d && d < 1.0e-10)
		{
			//printf("\t\tearly out 3\n");
			return false;
		}

		out.x = (e1.c * b - c * e1.b) / d;
		out.y = (c * e1.a - e1.c * a) / d;

	}
	else
	{
		real_t d = e1.a * e2.b - e1.b * e2.a;
		if (-1.0e-10 < d && d < 1.0e-10)
		{
			//printf("\t\tearly out 4\n");
			return false;
		}

		out.x = (e1.c * e2.b - e2.c * e1.b) / d;
		out.y = (e2.c * e1.a - e1.c * e2.a) / d;
	}
	// end of Gregory Soyez's modifications

	real_t local_y1 = e1.sites[1]->p.y;
	real_t local_y2 = e2.sites[1]->p.y;

	const Edge* e;
	const HalfEdge* he;
	if ((local_y1 < local_y2) || ((local_y1 == local_y2) && (e1.sites[1]->p.x < e2.sites[1]->p.x)))
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
		//printf("\t\tearly out 5\n");
		return false;
	}


	/*
	printf("\tintersection: e1.abc %f, %f, %f,  d = %f\n", e1.a, e1.b, e1.c, d);
	printf("\tintersection: abc    %f, %f, %f\n", a, b, c);
	printf("\tintersection: dx/dy  %f, %f\n", dx, dy);
	printf("\tintersection: adx/ady  %f, %f\n", adx, ady);
	printf("\tintersection new: %f, %f\n", out.x, out.y);
	*/

	return true;
}

bool Voronoi::check_circle_event(struct Arc* a1)
{
	const Arc* pl = arc_get_left_parent(a1);
	const Arc* pr = arc_get_right_parent(a1);

	Arc* a0 = (Arc*)arc_get_left_child(pl);
	Arc* a2 = (Arc*)arc_get_right_child(pr);

	if( !a0 || !a2 || a0->site == a2->site )
		return false;

	Point vertex;
	if( !edge_intersect(*pl->edge, *pr->edge, vertex) )
	{
		return false;
	}

	real_t dx = a1->site->p.x - vertex.x;
	real_t dy = a1->site->p.y - vertex.y;
	real_t radius = sqrt( (dx*dx) + (dy*dy) );
	real_t y = vertex.y + radius;

	Event* event = new_event(vertex, y);
	event->site = 0;
	event->arc 	= (Arc*)a1;
	a1->circleevent = event;

	eventqueue.push(event);
	return true;
}


struct Arc* Voronoi::get_arc_from_x(const Point& p)
{
	// Gets the arc on the beach line at the x coordinate (i.e. right above the new site event)
	struct Arc* arc = beachline;
	while( !arc->isleaf() )
	{
		bool right_of_edge = arc->edge->rightof(p);

		if( right_of_edge )
			arc = arc->right;
		else
			arc = arc->left;
	}
	return arc;
}


struct Edge* Voronoi::new_edge(struct Site* s1, struct Site* s2)
{
	assert(edgepool != 0);
	struct Edge* p = edgepool;
	edgepool = edgepool->next;
	p->create(s1, s2);
	return p;
}

void Voronoi::delete_edge(struct Edge* e)
{
	e->next = edgepool;
	edgepool = e;
}

struct HalfEdge* Voronoi::new_halfedge(struct Edge* e, int direction)
{
	assert(halfedgepool != 0);
	struct HalfEdge* p = halfedgepool;
	halfedgepool = halfedgepool->next;
	p->create(e, direction);
	return p;
}

void Voronoi::delete_halfedge(struct HalfEdge* e)
{
	e->next = halfedgepool;
	halfedgepool = e;
}

struct Arc* Voronoi::new_arc(struct Site* s)
{
	assert(arcpool != 0);
	struct Arc* a = arcpool;
	arcpool = arcpool->right;
	a->create(s);
	return a;
}

void Voronoi::delete_arc(struct Arc* a)
{
	a->right = arcpool;
	arcpool = a;
}

struct Event* Voronoi::new_event(struct Point& p, real_t y)
{
	assert(eventpool != 0);
	struct Event* e = eventpool;
	eventpool = (Event*)eventpool->arc;
	e->create(p, y);
	return e;
}

void Voronoi::delete_event(struct Event* e)
{
	e->arc = (Arc*)eventpool;
	eventpool = e;
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

