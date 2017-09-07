mkdir -p build

clang -c src/stb_wrapper.c -o build/stb_wrapper.o
#clang -c src/mk_cell_size_distribution.c -o build/mk_cell_size_distribution.o
#clang -o build/main -g -O2 -m64 -std=c99 -Wall -Weverything -Wno-float-equal -pedantic -lm build/stb_wrapper.o build/mk_cell_size_distribution.o src/main.c
clang -o build/main -g -O2 -m64 -std=c99 -Wall -Weverything -Wno-float-equal -pedantic -lm build/stb_wrapper.o src/main.c
