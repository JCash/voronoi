
# VORONOIPPINC=voronoi++/source/voronoi/include
# VORONOIPPSRC=voronoi++/source/voronoi/source
# VORONOIPP="$VORONOIPPSRC/fortune/Arc.cpp $VORONOIPPSRC/fortune/BeachLine.cpp $VORONOIPPSRC/fortune/Event.cpp $VORONOIPPSRC/fortune/Fortune.cpp $VORONOIPPSRC/geometry/Circle.cpp $VORONOIPPSRC/geometry/ConvexPolygon.cpp $VORONOIPPSRC/geometry/Line.cpp $VORONOIPPSRC/geometry/Parabola.cpp $VORONOIPPSRC/geometry/Point.cpp $VORONOIPPSRC/geometry/Rectangle.cpp $VORONOIPPSRC/geometry/Vector.cpp $VORONOIPPSRC/VoronoiCell.cpp $VORONOIPPSRC/VoronoiDiagram.cpp $VORONOIPPSRC/VoronoiEdge.cpp $VORONOIPPSRC/VoronoiSite.cpp"

# BOOSTINC=boost/polygon/include

# clang++ -o ../build/perftest -g -O3 -m64 -std=c++11 -I.. perftest.cpp fastjet/voronoi.cpp shaneosullivan/VoronoiDiagramGenerator.cpp -I$VORONOIPPINC -DVORONOI_STATIC $VORONOIPP -I$BOOSTINC

clang -o ../build/test -std=c99 -g -O0 -m64 -Wall -Weverything -Wno-float-equal -pedantic -I../src -lm test.c
