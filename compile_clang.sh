IVANK="test/ivank/Voronoi.cpp test/ivank/VParabola.cpp"
FASTJET="test/fastjet/voronoi.cpp"

clang++ -o main -g -O0 -m64 -std=c++11 -Isrc src/main.cpp src/voronoi.cpp -Itest/ivank $IVANK $FASTJET
