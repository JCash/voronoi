---
title: Documentation
weight: 1
---

`jc_voronoi` is a header-only C library for creating clipped 2D Voronoi diagrams from a point set. It uses Fortune's sweep algorithm and has no external dependencies.

## Add the library

Copy [`src/jc_voronoi.h`](https://github.com/JCash/voronoi/blob/dev/src/jc_voronoi.h) into your project. Emit the implementation from exactly one C or C++ translation unit:

```c
#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"
```

Other translation units should include the header without defining `JC_VORONOI_IMPLEMENTATION`.

## Language support

The source supports C99, C11, C17, and C23, and can also be included from C++. Compilers that use the pre-standard C23 name should select `c2x`.

## Next steps

- [Build the command-line example](build/)
- [Review the API](api/)
- [Start with a complete diagram example](../examples/generate-diagram/)
- [Use the browser and Node.js WebAssembly wrapper](../examples/webassembly/)
