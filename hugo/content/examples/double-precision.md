---
title: Double precision
weight: 7
---

Override the scalar type and matching math operations before emitting the implementation. Use the same configuration in every translation unit that includes the header.

```c
#define JCV_REAL_TYPE double
#define JCV_REAL_TYPE_EPSILON DBL_EPSILON
#define JCV_ATAN2 atan2
#define JCV_SQRT sqrt
#define JCV_FLT_MAX DBL_MAX
#define JCV_PI 3.14159265358979323846264338327950288
#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"
```
