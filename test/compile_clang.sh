
VORONOIPPINC=voronoi++/source/voronoi/include
VORONOIPPSRC=voronoi++/source/voronoi/source
VORONOIPP="$VORONOIPPSRC/fortune/Arc.cpp $VORONOIPPSRC/fortune/BeachLine.cpp $VORONOIPPSRC/fortune/Event.cpp $VORONOIPPSRC/fortune/Fortune.cpp $VORONOIPPSRC/geometry/Circle.cpp $VORONOIPPSRC/geometry/ConvexPolygon.cpp $VORONOIPPSRC/geometry/Line.cpp $VORONOIPPSRC/geometry/Parabola.cpp $VORONOIPPSRC/geometry/Point.cpp $VORONOIPPSRC/geometry/Rectangle.cpp $VORONOIPPSRC/geometry/Vector.cpp $VORONOIPPSRC/VoronoiCell.cpp $VORONOIPPSRC/VoronoiDiagram.cpp $VORONOIPPSRC/VoronoiEdge.cpp $VORONOIPPSRC/VoronoiSite.cpp"

BOOSTINC=boost/polygon/include

clang++ -o test -g -O3 -m64 -std=c++11 -I.. test.cpp fastjet/voronoi.cpp shaneosullivan/VoronoiDiagramGenerator.cpp -I$VORONOIPPINC -DVORONOI_STATIC $VORONOIPP -I$BOOSTINC
