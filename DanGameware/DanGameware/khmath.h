#pragma once;
#include <math.h>
#include <cstring>
#define H3D_RAD2DEG 57.324840764f
#define H3D_DEG2RAD  0.017453292f
typedef float f32;
typedef double f64;
typedef int s32;
typedef unsigned int u32;
const f32 ROUNDING_ERROR_f32 = 0.00001f;
#define FLT_MAX 3.402823466E+38F
inline bool iszero(const f32 a)
{
	return fabsf(a) <= ROUNDING_ERROR_f32;
}
inline f32 reciprocal(f32 v)
{
	return 1.0f / v;
}
inline f32 clamp(f32 v, f32 low, f32 high)
{
	if (v > high)return high;
	if (v < low)return low;
	return v;
}
inline f32 khmax(f32 v1, f32 v2)
{
	if (v1 > v2)
		return v1;
	return v2;
}
inline bool equalsTolf(const f32 a, const f32 b, const f32 tolerance = ROUNDING_ERROR_f32)
{
	return (a + tolerance >= b) && (a - tolerance <= b);
}
template <typename T>
inline void swap(T& v1, T& v2)
{
	T temp;
	temp = v1;
	v1 = v2;
	v2 = temp;
}

inline bool getLowestRoot(f32 a, f32 b, f32 c, f32 maxR, f32* root)
{
	// check if solution exists
	const f32 determinant = b*b - 4.0f*a*c;

	// if determinant is negative, no solution
	if (determinant < 0.0f || a == 0.f)
		return false;

	// calculate two roots: (if det==0 then x1==x2
	// but lets disregard that slight optimization)

	const f32 sqrtD = sqrtf(determinant);
	const f32 invDA = reciprocal(2 * a);
	f32 r1 = (-b - sqrtD) * invDA;
	f32 r2 = (-b + sqrtD) * invDA;

	// sort so x1 <= x2
	if (r1 > r2)
		swap(r1, r2);

	// get lowest root
	if (r1 > 0 && r1 < maxR)
	{
		*root = r1;
		return true;
	}

	// its possible that we want x2, this can happen if x1 < 0
	if (r2 > 0 && r2 < maxR)
	{
		*root = r2;
		return true;
	}

	return false;
}
class Vector2d
{
public:
	f32 x, y;
	Vector2d(f32 _x, f32 _y): x(_x),y(_y){};
};

template <class T>
class Vector3d
{
public:
	T x, y, z;
	Vector3d() = default;
	Vector3d(T _x, T _y, T _z) :x(_x), y(_y), z(_z) {}
	T dot(const Vector3d<T>& b)const
	{
		return x*b.x + y*b.y + z*b.z;
	}
	Vector3d<T> cross(const Vector3d<T>& p) const
	{
		return Vector3d<T>(y * p.z - z * p.y, z * p.x - x * p.z, x * p.y - y * p.x);
	}
	T getLengthSQ() const
	{
		return x*x + y*y + z*z;
	}
	T getLength() const
	{
		return sqrtl(x*x + y*y + z*z);
	}
	Vector3d<T>& normalize()
	{
		f64 length = x*x + y*y + z*z;
		if (length == 0) // this check isn't an optimization but prevents getting NAN in the sqrt.
			return *this;
		length = 1 / sqrtl(length);

		x = (T)(x * length);
		y = (T)(y * length);
		z = (T)(z * length);
		return *this;
	}
	Vector3d<T>& setLength(T newlength)
	{
		normalize();
		return (*this *= newlength);
	}
	bool equals(const Vector3d<T>& other, const T tolerance = (T)ROUNDING_ERROR_f32) const
	{
		return equalsTolf(x, other.x, tolerance) &&
			equalsTolf(y, other.y, tolerance) &&
			equalsTolf(z, other.z, tolerance);
	}
	void set(const T nx, const T ny, const T nz) { x = nx; y = ny; z = nz; }

	Vector3d<T>& operator=(const Vector3d<T>& other) { x = other.x; y = other.y; z = other.z; return *this; }

	Vector3d<T> operator+(const Vector3d<T>& other) const { return Vector3d<T>(x + other.x, y + other.y, z + other.z); }
	Vector3d<T>& operator+=(const Vector3d<T>& other) { x += other.x; y += other.y; z += other.z; return *this; }
	Vector3d<T> operator+(const T val) const { return Vector3d<T>(x + val, y + val, z + val); }
	Vector3d<T>& operator+=(const T val) { x += val; y += val; z += val; return *this; }

	Vector3d<T> operator-(const Vector3d<T>& other) const { return Vector3d<T>(x - other.x, y - other.y, z - other.z); }
	Vector3d<T>& operator-=(const Vector3d<T>& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
	Vector3d<T> operator-(const T val) const { return Vector3d<T>(x - val, y - val, z - val); }
	Vector3d<T>& operator-=(const T val) { x -= val; y -= val; z -= val; return *this; }

	Vector3d<T> operator*(const Vector3d<T>& other) const { return Vector3d<T>(x * other.x, y * other.y, z * other.z); }
	Vector3d<T>& operator*=(const Vector3d<T>& other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
	Vector3d<T> operator*(const T v) const { return Vector3d<T>(x * v, y * v, z * v); }
	Vector3d<T>& operator*=(const T v) { x *= v; y *= v; z *= v; return *this; }

	Vector3d<T> operator/(const Vector3d<T>& other) const { return Vector3d<T>(x / other.x, y / other.y, z / other.z); }
	Vector3d<T>& operator/=(const Vector3d<T>& other) { x /= other.x; y /= other.y; z /= other.z; return *this; }
	Vector3d<T> operator/(const T v) const { T i = (T)1.0 / v; return Vector3d<T>(x * i, y * i, z * i); }
	Vector3d<T>& operator/=(const T v) { T i = (T)1.0 / v; x *= i; y *= i; z *= i; return *this; }

};
typedef Vector3d<f32> Vector3df;
typedef Vector3d<f64> Vector3df64;

struct Matrix
{
	f32 M[16];
	inline Matrix& setScale(const Vector3df& scale)
	{
		memset(M, 0, 16 * sizeof(f32));
		M[0] = scale.x;
		M[5] = scale.y;
		M[10] = scale.z;
		return *this;
	}
	inline void makeIdentity()
	{
		memset(M, 0, 16 * sizeof(f32));
		M[0] = M[5] = M[10] = M[15] = 1;
	}
	inline void transformVect(Vector3df& out, const Vector3df& in) const
	{
		out.x = in.x*M[0] + in.y*M[4] + in.z*M[8] + M[12];
		out.y = in.x*M[1] + in.y*M[5] + in.z*M[9] + M[13];
		out.z = in.x*M[2] + in.y*M[6] + in.z*M[10] + M[14];
	}
};
struct Aabbox3d
{
	Vector3df minEdge;
	Vector3df maxEdge;
	Aabbox3d() : minEdge(-1, -1, -1), maxEdge(1, 1, 1) {}
	Aabbox3d(const Vector3df init) : minEdge(init), maxEdge(init) {}
	void addInternalPoint(const Vector3df& point)
	{
		if (point.x>maxEdge.x) maxEdge.x = point.x;
		if (point.y>maxEdge.y) maxEdge.y = point.y;
		if (point.z>maxEdge.z) maxEdge.z = point.z;

		if (point.x<minEdge.x) minEdge.x = point.x;
		if (point.y<minEdge.y) minEdge.y = point.y;
		if (point.z<minEdge.z) minEdge.z = point.z;
	}
	void reset(const Vector3df& initValue)
	{
		maxEdge = initValue;
		minEdge = initValue;
	}
	void getEdges(Vector3df *edges) const
	{
		const Vector3df middle = (maxEdge + minEdge) / 2;
		const Vector3df diag = middle - maxEdge;
		edges[0].set(middle.x + diag.x, middle.y + diag.y, middle.z + diag.z);
		edges[1].set(middle.x + diag.x, middle.y - diag.y, middle.z + diag.z);
		edges[2].set(middle.x + diag.x, middle.y + diag.y, middle.z - diag.z);
		edges[3].set(middle.x + diag.x, middle.y - diag.y, middle.z - diag.z);
		edges[4].set(middle.x - diag.x, middle.y + diag.y, middle.z + diag.z);
		edges[5].set(middle.x - diag.x, middle.y - diag.y, middle.z + diag.z);
		edges[6].set(middle.x - diag.x, middle.y + diag.y, middle.z - diag.z);
		edges[7].set(middle.x - diag.x, middle.y - diag.y, middle.z - diag.z);
	}
	bool isEmpty() const
	{
		return minEdge.equals(maxEdge);
	}
	bool isPointInside(const Vector3df& p) const
	{
		return (p.x >= minEdge.x && p.x <= maxEdge.x &&
			p.y >= minEdge.y && p.y <= maxEdge.y &&
			p.z >= minEdge.z && p.z <= maxEdge.z);
	}
	bool intersectsWithBox(const Aabbox3d& other) const
	{
		return (minEdge.x <= other.maxEdge.x && minEdge.y <= other.maxEdge.y && minEdge.z <= other.maxEdge.z &&
			maxEdge.x >= other.minEdge.x && maxEdge.y >= other.minEdge.y && maxEdge.z >= other.minEdge.z);
	}
};
struct Plane
{
	Vector3df normal;
	f32 d;
	Plane(const Vector3df& MPoint, const Vector3df& Normal) : normal(Normal) { recalculateD(MPoint); }
	Plane(const Vector3df& point1, const Vector3df& point2, const Vector3df& point3)
	{
		normal = (point2 - point1).cross(point3 - point1);
		normal.normalize();
		recalculateD(point1);
	}
	void recalculateD(const Vector3df& MPoint)
	{
		d = -MPoint.dot(normal);
	}
	bool isFrontFacing(const Vector3df& lookDirection)const
	{
		return normal.dot(lookDirection) <= 0;
	}
	f32 getDistanceTo(const Vector3df& point)
	{
		return point.dot(normal) + d;
	}
	inline bool isPointFront(const Vector3df& p)const
	{
		return (p - (normal*d)).dot(p) > 0;
	}
};
struct Triangle
{
	Vector3df p1, p2, p3;
	Plane getPlane() const
	{
		return Plane(p1, p2, p3);
	}
	bool isPointInside(const Vector3df& p) const
	{
		Vector3df64 af64((f64)p1.x, (f64)p1.y, (f64)p1.z);
		Vector3df64 bf64((f64)p2.x, (f64)p2.y, (f64)p2.z);
		Vector3df64 cf64((f64)p3.x, (f64)p3.y, (f64)p3.z);
		Vector3df64 pf64((f64)p.x, (f64)p.y, (f64)p.z);
		return (isOnSameSide(pf64, af64, bf64, cf64) &&
			isOnSameSide(pf64, bf64, af64, cf64) &&
			isOnSameSide(pf64, cf64, af64, bf64));
	}
	bool isTotalInsideBox(const Aabbox3d& box) const
	{
		return (box.isPointInside(p1) &&
			box.isPointInside(p2) &&
			box.isPointInside(p3));
	}
	bool isOnSameSide(const Vector3df64& p1, const Vector3df64& p2, const Vector3df64& a, const Vector3df64& b) const
	{
		Vector3df64 bminusa = b - a;
		Vector3df64 cp1 = bminusa.cross(p1 - a);
		Vector3df64 cp2 = bminusa.cross(p2 - a);
		f64 res = cp1.dot(cp2);
		if (res < 0)
		{
			// This catches some floating point troubles.
			// Unfortunately slightly expensive and we don't really know the best epsilon for iszero.
			Vector3df64 cp1 = bminusa.normalize().cross((p1 - a).normalize());
			if (iszero(cp1.x) && iszero(cp1.y) && iszero(cp1.z))
			{
				res = 0.f;
			}
		}
		return (res >= 0.0f);
	}
	bool isTotalOutsideBox(const Aabbox3d& box) const
	{
		return ((p1.x > box.maxEdge.x && p2.x > box.maxEdge.x && p3.x > box.maxEdge.x) ||

			(p1.y > box.maxEdge.y && p2.y > box.maxEdge.y && p3.y > box.maxEdge.y) ||
			(p1.z > box.maxEdge.z && p2.z > box.maxEdge.z && p3.z > box.maxEdge.z) ||
			(p1.x < box.minEdge.x && p2.x < box.minEdge.x && p3.x < box.minEdge.x) ||
			(p1.y < box.minEdge.y && p2.y < box.minEdge.y && p3.y < box.minEdge.y) ||
			(p1.z < box.minEdge.z && p2.z < box.minEdge.z && p3.z < box.minEdge.z));
	}


};