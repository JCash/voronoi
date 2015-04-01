# voronoi
A C++ implementation for creating 2D voronoi diagrams


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
* No external dependencies (other than standard library files)
* Cells have a list of edges (for easier relaxation)
* A clear license


Comparisons
===========


General thoughts
================

Fastjet
=======

The Fastjet version is built upon Steven Fortune's original C version, which Shane O'Sullivan improved upon. 
Given the robustness and speed improvements of the implementation done by Fastjet,
that should be the base line to compare other implementations with.

Unfortunately, the code is not very readable, and the license seems to be   

Boost
=====

Using boost might be convenient for some, but the sheer amount of code is too great in many cases.
I had to install 5 modules of boost to compile (config, core, mpl, preprocessor and polygon).
The code consists of only templated headers, so they'll get recompiled each time you recompile.
For simply generating a 2D voronoi diagram using points as input, it is clearly overkill.

Ivan K
======

Even though I started out using this, after reading a nice blog post about it,
it turns out to be among the slowest but more importantly, it is broken.
It simply doesn't many of the edge cases. For instance, it cannot handle vertical edges.

 