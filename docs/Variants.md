# Variants

- [Overview](/docs/Variants.md#overview)
    - [Signed Integers](/docs/Variants.md#signed-integers), [Unsigned Integers](/docs/Variants.md#unsigned-integers)
    - [AABB2](/docs/Variants.md#aabb), [AABB3](/docs/Variants.md#aabb)
    - [OOB2](/docs/Variants.md#aabb), [OOB3](/docs/Variants.md#aabb)
    - [Color](/docs/Variants.md#color)
    - [Rect](/docs/Variants.md#rect), [iRect](/docs/Variants.md#recti)
    - [Curve](/docs/Variants.md#curve), [Curve2](/docs/Variants.md#curve2), [Curve3](/docs/Variants.md#curve3)
    - [Vec2](/docs/Variants.md#vec2), [Vec3](Variants.md#vec3), [Vec4](/docs/Variants.md#vec4)
    - [iVec2](/docs/Variants.md#vec2i), [iVec3](/docs/Variants.md#vec3i), [iVec4](/docs/Variants.md#vec4i)
    - [uVec2](/docs/Variants.md#uvec2), [uVec3](/docs/Variants.md#uvec3), [uVec4](/docs/Variants.md#uvec4)
    - [dVec2](/docs/Variants.md#bvec2), [dVec3](/docs/Variants.md#bvec3), [dVec4](/docs/Variants.md#bvec4)
    - [Float2](/docs/Variants.md#float2), [Float3](/docs/Variants.md#float3), [Float4](/docs/Variants.md#float4)
    - [Int2](/docs/Variants.md#float2), [Int3](/docs/Variants.md#float3), [Int4](/docs/Variants.md#float4)
    - [Double2](/docs/Variants.md#double2), [Double3](/docs/Variants.md#double3), [Double4](/docs/Variants.md#double4)
    - [Mat2x2](/docs/Variants.md#mat2x2), [Mat2x3](/docs/Variants.md#mat2x3), [Mat2x4](/docs/Variants.md#mat2x4), [Mat3x2](/docs/Variants.md#mat3x2), [Mat3x3](/docs/Variants.md#mat3x3), [Mat3x4](/docs/Variants.md#mat3x4), [Mat4x2](/docs/Variants.md#mat4x2), [Mat4x3](/docs/Variants.md#mat4x3), [Mat4x4](/docs/Variants.md#mat4x4)
    - [Quat](/docs/Variants.md#quat), [Basis](/docs/Variants.md#basis),
    - [TArray](/docs/Variants.md#), [TMap](/docs/Variants.md#), [TSet](/docs/Variants.md#), [TStaticArray](/docs/Variants.md#), [TMultiMap](/docs/Variants.md#), [TPair](/docs/Variants.md#)
    - [Hash](/docs/Variants.md#)
    

## Signed Integers & Unsigned Integers
```cpp
// Provided by Moss
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef unsigned char uint8_t;
typedef unsigned short uint16;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
```
## Signed Integers & Unsigned Integers
```cpp
// Provided by Moss
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef unsigned char uint8_t;
typedef unsigned short uint16;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
```
#### Macros
```cpp
// Provided by Moss
#define MAX_INT8    ((int8)(0x7F))
#define MAX_INT16   ((int16)(0x7FFF))
#define MAX_INT32   ((int32)(0x7FFFFFFF))
#define MAX_INT64   ((int64)(0x7FFFFFFFFFFFFFFF))
#define MIN_INT8    ((int8)(~0x7F))
#define MIN_INT16   ((int16)~0x7FFF)
#define MIN_INT32   ((int32)(~0x7FFFFFFF))
#define MIN_INT64   ((int64)(~0x7FFFFFFFFFFFFFFF))

#define MAX_UINT8   ((uint8_t)(0xFF))
#define MAX_UINT16  ((uint16)(0xFFFF))
#define MAX_UINT32  ((uint32_t)(0xFFFFFFFFu))
#define MAX_UINT64  ((uint64_t)(0xFFFFFFFFFFFFFFFF))
#define MIN_UINT8   ((uint8_t)0x00)
#define MIN_UINT16  ((uint16)0x0000)
#define MIN_UINT32  ((uint32_t)0x00000000)
#define MIN_UINT64  ((uint64_t)(0x0000000000000000))
```
## Geometry Types
| Type | Purpose |
| --- | --- |
| `Rect`, `iRect` | 2D rectangles. |
| `AABB3`, `AABox3` | Axis-aligned bounds 3D. |
| `AABB2`, `AABox2` | Axis-aligned bounds 2D. |
| `OOB3`, `AABox3` | Oriented bounds 3D. |
| `OOB2`, `AABox2` | Oriented bounds 2D. |
| `Curve`, `Curve2`, `Curve3` | Curve data for interpolation and authored paths. |

## Basis
```cpp
Basis();
Basis(axis: Vector3, angle: float);
Basis(from: Quaternion);
Basis(x_axis: Vec3, y_axis: Vec3, z_axis: Vec3);
```
| Variant | Operator |
| --- | --- |
| `git status` | List all *new or modified* files |
| `git diff` | Show file differences that **haven't been** staged |

## Color
```cpp
Color();
Color(float r, float g, float b);
Color(float r, float g, float b, float a);
```

```cpp
Color color(1.0f, 0.5f, 0.25f, 1.0f);
float intensity = color.GetIntensity();
```

## Rect
```cpp
Rect();
```
| Variant | Operator |
| --- | --- |
| `git status` | List all *new or modified* files |
| `git diff` | Show file differences that **haven't been** staged |

## iRect
```cpp
Recti();
```
| Variable | Operator |
| --- | --- |
| `git status` | List all *new or modified* files |
| `git diff` | Show file differences that **haven't been** staged |

## Quat
```cpp
Quat();
```
| Variant | Operator |
| --- | --- |
| `git status` | List all *new or modified* files |
| `git diff` | Show file differences that **haven't been** staged |

## Vector Types
| Type | Purpose |
| --- | --- |
| `Float2`, `Float3`, `Float4` | Plain float vector storage used by public descriptors. |
| `Int2`, `Int3`, `Int4` | Plain integer vector storage. |
| `Double2`, `Double3`, `Double4` | Plain double vector storage. |
| `Vec2`, `Vec3`, `Vec4` | Math vector types with operators and SIMD-friendly implementations. |
| `iVec2`, `iVec3`, `iVec4` | Integer math vector types. |
| `UVec2`, `UVec3`, `UVec4` | Unsigned integer math vector types. |
| `DVec2`, `DVec3`, `DVec4` | Double precision math vector types. |

## Matrix And Rotation Types
| Type | Purpose |
| --- | --- |
| `Mat2x2`, `Mat2x3`, `Mat2x4` | 2-row matrix variants. |
| `Mat3x2`, `Mat3x3`, `Mat3x4` | 3-row matrix variants. |
| `Mat4x2`, `Mat4x3`, `Mat4x4`, `Mat44` | 4-row and transform matrix variants. |
| `Quat` | Quaternion rotation. |
| `Basis` | 3D orientation basis. |

## Containers
| Type | Purpose |
| --- | --- |
| `TArray<T>` | Dynamic array. |
| `TMap<K, V>` | Key/value map. |
| `TSet<T>` | Set container. |
| `TStaticArray<T, N>` | Fixed-size array. |
| `TMultiMap<K, V>` | Multi-value map. |
| `TPair<A, B>` | Pair value. |


## Notes
- Public C-like APIs prefer plain descriptor-friendly types such as `Float2`, `Float3`, `Float4`, `Color`, and scalar integers.
- Internal math and physics code may use richer vector/matrix types such as `Vec3`, `Quat`, and `Mat44`.
- Keep ownership rules outside variant types; variants should be cheap value objects.