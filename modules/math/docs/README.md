# wk::math

`wk::math` is a header-only C++23 library for fixed-size vectors, matrices, transforms, projection ...

Include the public entry point:

```cpp
#include <wk/math.hpp>
```

Vectors use contiguous storage and are available as `vec<N, T>` plus `vec2`, `vec3`, and `vec4` aliases. 
Components are accessed by index or with `x()`, `y()`, `z()`, and `w()` accessors. 
Matrices use contiguous column-major storage; `matrix(row, column)`, `matrix[column][row]`, and `matrix.data()` all refer to that layout. 
Constructor input order is selected explicitly with `layout::rowm` or `layout::colm`.

```cpp
const wk::math::vec3f direction = wk::math::normalize(wk::math::vec3f(1.0F, 2.0F, 0.0F));
const wk::math::mat4f model = wk::math::translate(wk::math::vec3f(2.0F, 1.0F, -3.0F))
                           * wk::math::rotate_z(wk::math::radians(30.0F));
```
