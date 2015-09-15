FASTJET="test/fastjet/voronoi.cpp"

clang++ -o main -g -O0 -m64 -Isrc src/main.cpp src/voronoi.cpp $FASTJET
