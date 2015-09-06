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

[The MIT license](http://choosealicense.com/licenses/mit/)


Usage
=====

```C++

#include "voronoi.h"

void generate(int count, voronoi::Point* points, int imagewidth, int imageheight)
{
    voronoi::Voronoi generator;
    generator.generate(count, points, imagewidth, imageheight);
    
    const struct voronoi::Edge* e = generator.get_edges();
    while( e )
    {
        printf("%f, %f -> %f, %f\n", e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y);
        e = e->next;
    }
}

```


Comparisons
===========


General thoughts
================

O'Sullivan
==========

A C++ version of the original C version from Steven Fortune.

Although fast, it's not completely robust and will produce errors.


Fastjet
=======

The Fastjet version is built upon Steven Fortune's original C version, which Shane O'Sullivan improved upon. 
Given the robustness and speed improvements of the implementation done by Fastjet,
that should be the base line to compare other implementations with.

Unfortunately, the code is not very readable, and the license is a bit unclear (GPL?)


Boost
=====

Using boost might be convenient for some, but the sheer amount of code is too great in many cases.
I had to install 5 modules of boost to compile (config, core, mpl, preprocessor and polygon).

It is ~3x as slow as the fastest algorithms, and takes ~2.5x as much memory.

The code consists of only templated headers, so they'll get recompiled each time you recompile.
For simply generating a 2D voronoi diagram using points as input, it is clearly overkill.

The boost implementation also puts the burden of clipping the final edges on the client. 

Voronoi++
=========

The speed of it is simply too slow to be used in a time critical application.
And it uses ~16x more memory than the fastest algorithms.

Using the same data sets as the other algorithms, it breaks under some conditions.

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
