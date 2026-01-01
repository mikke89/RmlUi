#include <type_traits>

namespace Rml {

template <typename Type>
Vector4<Type>::Vector4() : x{}, y{}, z{}, w{}
{}

template <typename Type>
Vector4<Type>::Vector4(Type v) : x(v), y(v), z(v), w(v)
{}

template <typename Type>
Vector4<Type>::Vector4(Type x, Type y, Type z, Type w) : x(x), y(y), z(z), w(w)
{}

template <typename Type>
Vector4<Type>::Vector4(Vector3<Type> const& v, Type w) : x(v.x), y(v.y), z(v.z), w(w)
{}

template <typename Type>
float Vector4<Type>::Magnitude() const
{
	return Math::SquareRoot(static_cast<float>(SquaredMagnitude()));
}

template <typename Type>
Type Vector4<Type>::SquaredMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

template <typename Type>
inline Vector4<Type> Vector4<Type>::Normalise() const
{
	static_assert(std::is_same<Type, float>::value, "Normalise only implemented for Vector<float>.");
	return *this;
}

template <>
inline Vector4<float> Vector4<float>::Normalise() const
{
	const float magnitude = Magnitude();
	if (Math::IsCloseToZero(magnitude))
		return *this;

	return *this / magnitude;
}

template <typename Type>
Type Vector4<Type>::DotProduct(const Vector4<Type>& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

template <typename Type>
Vector3<Type> Vector4<Type>::PerspectiveDivide() const
{
	return Vector3<Type>(x / w, y / w, z / w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator-() const
{
	return Vector4(-x, -y, -z, -w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator+(const Vector4<Type>& rhs) const
{
	return Vector4<Type>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator-(const Vector4<Type>& rhs) const
{
	return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator*(Type rhs) const
{
	return Vector4(x * rhs, y * rhs, z * rhs, w * rhs);
}

template <typename Type>
Vector4<Type> Vector4<Type>::operator/(Type rhs) const
{
	return Vector4(x / rhs, y / rhs, z / rhs, w / rhs);
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator+=(const Vector4& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator-=(const Vector4& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator*=(const Type& rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;

	return *this;
}

template <typename Type>
Vector4<Type>& Vector4<Type>::operator/=(const Type& rhs)
{
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;

	return *this;
}

template <typename Type>
bool Vector4<Type>::operator==(const Vector4& rhs) const
{
	return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
}

template <typename Type>
bool Vector4<Type>::operator!=(const Vector4& rhs) const
{
	return (x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w);
}

template <typename Type>
Vector4<Type>::operator const Type*() const
{
	return &x;
}

template <typename Type>
Vector4<Type>::operator Type*()
{
	return &x;
}

template <typename Type>
template <typename U>
inline Vector4<Type>::operator Vector4<U>() const
{
	return Vector4<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w));
}

template <typename Type>
Vector4<Type>::operator Vector3<Type>() const
{
	return Vector3<Type>(x, y, z);
}

template <typename Type>
Vector4<Type>::operator Vector2<Type>() const
{
	return Vector2<Type>(x, y);
}

template <typename Type>
inline Vector4<Type> operator*(Type lhs, Vector4<Type> rhs)
{
	return rhs * lhs;
}

} // namespace Rml
