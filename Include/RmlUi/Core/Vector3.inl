#include <type_traits>

namespace Rml {

template <typename Type>
Vector3<Type>::Vector3() : x{}, y{}, z{}
{}

template <typename Type>
Vector3<Type>::Vector3(Type v) : x(v), y(v), z(v)
{}

template <typename Type>
Vector3<Type>::Vector3(Type x, Type y, Type z) : x(x), y(y), z(z)
{}

template <typename Type>
float Vector3<Type>::Magnitude() const
{
	return Math::SquareRoot(static_cast<float>(SquaredMagnitude()));
}

template <typename Type>
Type Vector3<Type>::SquaredMagnitude() const
{
	return x * x + y * y + z * z;
}

template <typename Type>
inline Vector3<Type> Vector3<Type>::Normalise() const
{
	static_assert(std::is_same<Type, float>::value, "Normalise only implemented for Vector<float>.");
	return *this;
}

template <>
inline Vector3<float> Vector3<float>::Normalise() const
{
	const float magnitude = Magnitude();
	if (Math::IsCloseToZero(magnitude))
		return *this;

	return *this / magnitude;
}

template <typename Type>
Type Vector3<Type>::DotProduct(const Vector3<Type>& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

template <typename Type>
Vector3<Type> Vector3<Type>::CrossProduct(const Vector3<Type>& rhs) const
{
	return Vector3<Type>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

template <typename Type>
Vector3<Type> Vector3<Type>::operator-() const
{
	return Vector3(-x, -y, -z);
}

template <typename Type>
Vector3<Type> Vector3<Type>::operator+(const Vector3<Type>& rhs) const
{
	return Vector3<Type>(x + rhs.x, y + rhs.y, z + rhs.z);
}

template <typename Type>
Vector3<Type> Vector3<Type>::operator-(const Vector3<Type>& rhs) const
{
	return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
}

template <typename Type>
Vector3<Type> Vector3<Type>::operator*(Type rhs) const
{
	return Vector3(x * rhs, y * rhs, z * rhs);
}

template <typename Type>
Vector3<Type> Vector3<Type>::operator/(Type rhs) const
{
	return Vector3(x / rhs, y / rhs, z / rhs);
}

template <typename Type>
Vector3<Type>& Vector3<Type>::operator+=(const Vector3& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;

	return *this;
}

template <typename Type>
Vector3<Type>& Vector3<Type>::operator-=(const Vector3& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;

	return *this;
}

template <typename Type>
Vector3<Type>& Vector3<Type>::operator*=(const Type& rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;

	return *this;
}

template <typename Type>
Vector3<Type>& Vector3<Type>::operator/=(const Type& rhs)
{
	x /= rhs;
	y /= rhs;
	z /= rhs;

	return *this;
}

template <typename Type>
bool Vector3<Type>::operator==(const Vector3& rhs) const
{
	return (x == rhs.x && y == rhs.y && z == rhs.z);
}

template <typename Type>
bool Vector3<Type>::operator!=(const Vector3& rhs) const
{
	return (x != rhs.x || y != rhs.y || z != rhs.z);
}

template <typename Type>
Vector3<Type>::operator const Type*() const
{
	return &x;
}

template <typename Type>
Vector3<Type>::operator Type*()
{
	return &x;
}

template <typename Type>
template <typename U>
inline Vector3<Type>::operator Vector3<U>() const
{
	return Vector3<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z));
}

template <typename Type>
Vector3<Type>::operator Vector2<Type>() const
{
	return Vector2<Type>(x, y);
}

template <typename Type>
inline Vector3<Type> operator*(Type lhs, Vector3<Type> rhs)
{
	return rhs * lhs;
}

} // namespace Rml
