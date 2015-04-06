# voronoi
A fast C++ implementation for creating 2D Voronoi diagrams from a point set

Uses Fortune's sweep algorithm.

![50 points](example1.png)


Goals
=====

Given the alternative solutions out there, they usually lack one aspect or the other.
So this lib set out to achieve a combination of the good things the other libs provide.

* Robustness
* Speed
* Small or Fixed memory footprint
* Single/Double floating point implementation
* Readable code
* Small code (single source file)
* No external dependencies
* Cells have a list of edges (for easier relaxation)
* A clear license


License
=======

The MIT license


Usage
=====

```C++

#include "voronoi.h"

void generate(int count, voronoi::Point* points, int imagewidth, int imageheight)
{
    voronoi::Voronoi generator;
    generator.generate(count, points, imagewidth, imageheight);
    
    const struct voronoi::Edge* edge = generator.get_edges();
    while( edge )
    {
        printf("%f, %f -> %f, %f\n", edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y);
        edge = edge->next;
    }
}

```


Comparisons
===========


General thoughts
================

Fastjet
=======

The Fastjet version is built upon Steven Fortune's original C version, which Shane O'Sullivan improved upon. 
Given the robustness and speed improvements of the implementation done by Fastjet,
that should be the base line to compare other implementations with.

Unfortunately, the code is not very readable, and the license is a bit unclear.

Boost
=====

Using boost might be convenient for some, but the sheer amount of code is too great in many cases.
I had to install 5 modules of boost to compile (config, core, mpl, preprocessor and polygon).

It is ~2x as slow as the fastest algorithms, and takes ~10x as much memory.

The code consists of only templated headers, so they'll get recompiled each time you recompile.
For simply generating a 2D voronoi diagram using points as input, it is clearly overkill.

Voronoi++
=========

The speed of it is simply too slow to be used in a time critical application.
And it uses ~10x more memory than the fastest algorithms.


Ivan K
======

Even though I started out using this, after reading a nice blog post about it,
it turns out to be among the slowest but more importantly, it is broken.
It simply doesn't handle many of the edge cases. For instance,
it cannot handle vertical edges due to the fact that it represents lines as 'y = mx + b'




Contact
=======

http://sizeofvoid.blogspot.com

https://twitter.com/mwesterdahl76
