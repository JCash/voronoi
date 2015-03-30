#pragma once

#include <stdint.h>
#include <queue>

namespace voronoi
{

#ifndef VORONOI_REAL_TYPE
	typedef float real_t;
	//typedef double real_t;
#else
	typedef VORONOI_REAL_TYPE real_t;
#endif

struct Point
{
	real_t x;
	real_t y;

	Point() {}
	Point(const Point& p) : x(p.x), y(p.y) {}
	Point(real_t _x, real_t _y) : x(_x), y(_y) {}
};

struct Site
{
	Point	p;
	//int 	id;			// index into original list

	struct HalfEdge*	edges;	// The half edges owned by the cell
};


struct Edge
{
	Site* 		sites[2];
	Point		pos[2];
	real_t		a;
	real_t		b;
	real_t		c;
	Edge*		next;

	void create(struct Site* s1, struct Site* s2);
	void clipline(real_t width, real_t height);
};


struct CompareEvent : public std::binary_function<struct Event*, struct Event*, bool>
{
	bool operator()(const struct Event* l, const struct Event* r) const;
};

typedef std::priority_queue<struct Event*, std::vector<struct Event*>, CompareEvent> priorityqueue_t;

class Voronoi
{
public:
	Voronoi();
	~Voronoi();

	size_t get_required_mem();

	void generate(size_t num_sites, const Point* sites, int _width, int _height);

	const struct Site* get_cells() const;
	const struct Edge* get_edges() const;

private:
	void cleanup();
	void site_event(const struct Event* e);
	void circle_event(const struct Event* e);

	bool check_circle_event(struct Arc* arc);

	struct Arc* get_arc_from_x(const Point& p);
	bool edge_intersect(const struct HalfEdge& e1, const struct HalfEdge& e2, Point& out) const;
	void endpos(struct HalfEdge* e, const Point& p, int direction);

	//void debugdraw();

	struct Edge*		edges;
	struct Arc*			beachline;		// The binary tree of arcs
	priorityqueue_t		eventqueue;
	struct Site*		sites;

	real_t 				sweepline;		// The y value of the sweep line
	real_t 				width;
	real_t 				height;

	struct Edge*		edgemem;
	struct Edge*		edgepool;

	struct Arc*			arcmem;
	struct Arc*			arcpool;

	struct HalfEdge*	halfedgemem;
	struct HalfEdge*	halfedgepool;

	struct Event*		eventmem;
	struct Event*		eventpool;

	struct Edge*		new_edge(struct Site* s1, struct Site* s2);
	void				delete_edge(struct Edge*);

	struct HalfEdge*	new_halfedge(struct Edge* e, int direction);
	void				delete_halfedge(struct HalfEdge*);

	struct Arc*			new_arc(struct Site* s);
	void				delete_arc(struct Arc*);

	struct Event*		new_event(struct Point& p, real_t y);
	void				delete_event(struct Event*);
};

};
