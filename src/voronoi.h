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

#pragma once

#include <stdlib.h>
#include <stdint.h>

namespace voronoi
{

#ifndef VORONOI_REAL_TYPE
	typedef float real_t;
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
	const Point& operator=( const Point& rhs )
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	bool operator==( const Point& rhs ) const
	{
		return x == rhs.x && y == rhs.y;
	}

	bool operator!=( const Point& rhs ) const
	{
		return !(*this == rhs);
	}
};

struct GraphEdge
{
	GraphEdge* 		next;
	struct Edge*	edge;
	struct Site*	neighbor;
	Point			pos[2];
};

struct Site
{
	Point	p;

	struct GraphEdge* edges;	// The half edges owned by the cell
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
	bool clipline(real_t width, real_t height);
};

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

	Site*	nextsite();
	void 	site_event(struct Site* site);
	void 	circle_event();
	bool 	check_circle_event(struct HalfEdge* he1, struct HalfEdge* he2, Point& vertex);

	struct HalfEdge* get_edge_above_x(const Point& p);
	bool edge_intersect(const struct HalfEdge& e1, const struct HalfEdge& e2, Point& out) const;
	void endpos(struct Edge* e, const Point& p, int direction);
	void finishline(struct Edge* e);

	struct Edge*		edges;
	struct HalfEdge*	beachline_start;
	struct HalfEdge*	beachline_end;
	struct HalfEdge*	last_inserted;
	struct PriorityQueue* eventqueue;

	struct Site*		sites;
	struct Site*		bottomsite;
	int					numsites;
	int					numsites_sqrt;
	int					currentsite;

	real_t 				width;
	real_t 				height;

	struct MemoryBlock* memblocks;
	struct Edge*		edgepool;
	struct HalfEdge*	halfedgepool;

	void*				alloc(size_t size);
	struct Edge*		alloc_edge();
	struct HalfEdge*	alloc_halfedge();
	struct GraphEdge*	alloc_graphedge();

	void**				eventmem;

	struct Edge*		new_edge(struct Site* s1, struct Site* s2);
	void				delete_edge(struct Edge*);

	struct HalfEdge*	new_halfedge(struct Edge* e, int direction);
	void				delete_halfedge(struct HalfEdge*);
};


typedef bool (*FPriorityQueueCompare)(const void* node1, const void* node2);
typedef void (*FPriorityQueueSetpos)(const void* node, int pos);
typedef int  (*FPriorityQueueGetpos)(const void* node);
typedef int  (*FPriorityQueuePrint)(const void* node, int pos);

struct PriorityQueue
{
	// Implements a binary heap
	FPriorityQueueCompare	compare;		// implement the < operator
	FPriorityQueueSetpos	setpos;
	FPriorityQueueGetpos	getpos;
	int						maxnumitems;
	int						numitems;
	const void**			items;
};

void 		pq_create(PriorityQueue* pq, int capacity, const void** buffer,
						FPriorityQueueCompare cmp,
						FPriorityQueueSetpos setpos,
						FPriorityQueueGetpos getpos);
bool 		pq_empty(PriorityQueue* pq);
int 		pq_push(PriorityQueue* pq, const void* node);
const void* pq_pop(PriorityQueue* pq);
const void* pq_top(PriorityQueue* pq);
void 		pq_remove(PriorityQueue* pq, const void* node);
void		pq_print(PriorityQueue* pq, FPriorityQueuePrint printfn);

};
