/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014 Markus Schöngart
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef ROCKETCORETRANSFORMPRIMITIVE_H
#define ROCKETCORETRANSFORMPRIMITIVE_H

#include <Rocket/Core/Header.h>
#include <Rocket/Core/Types.h>
#include <Rocket/Core/Property.h>

namespace Rocket {
namespace Core {
namespace Transforms {

struct NumericValue
{
	/// Non-initializing constructor.
	NumericValue() throw();
	/// Construct from a float and a Unit.
	NumericValue(float number, Property::Unit unit) throw();

	/// Resolve a numeric property value for an element.
	float Resolve(Element& e, float base) const throw();
	/// Resolve a numeric property value with the element's width as relative base value.
	float ResolveWidth(Element& e) const throw();
	/// Resolve a numeric property value with the element's height as relative base value.
	float ResolveHeight(Element& e) const throw();
	/// Resolve a numeric property value with the element's depth as relative base value.
	float ResolveDepth(Element& e) const throw();

	float number;
	Rocket::Core::Property::Unit unit;
};

/**
	The Primitive class is the base class of geometric transforms such as rotations, scalings and translations.
	Instances of this class are added to a Rocket::Core::Transform instance
	by the Rocket::Core::PropertyParserTransform, which is responsible for
	parsing the `transform' property.

	@author Markus Schöngart
	@see Rocket::Core::Transform
	@see Rocket::Core::PropertyParserTransform
 */
class Primitive
{
	public:
		virtual ~Primitive() { }

		virtual Primitive* Clone() const = 0;

		/// Resolve the transformation matrix encoded by the primitive.
		/// @param m The transformation matrix to resolve the Primitive to.
		/// @param e The Element which to resolve the Primitive for.
		/// @return true if the Primitive encodes a transformation.
		virtual bool ResolveTransform(Matrix4f& m, Element& e) const throw()
			{ return false; }
		/// Resolve the perspective value encoded by the primitive.
		/// @param p The perspective value to resolve the Primitive to.
		/// @param e The Element which to resolve the Primitive for.
		/// @return true if the Primitive encodes a perspective value.
		virtual bool ResolvePerspective(float &p, Element& e) const throw()
			{ return false; }
};

template< size_t N >
class ResolvedValuesPrimitive : public Primitive
{
	public:
		ResolvedValuesPrimitive(const NumericValue* values) throw()
			{ for (size_t i = 0; i < N; ++i) this->values[i] = values[i].number; }

	protected:
		float values[N];
};

template< size_t N >
class UnresolvedValuesPrimitive : public Primitive
{
	public:
		UnresolvedValuesPrimitive(const NumericValue* values) throw()
			{ memcpy(this->values, values, sizeof(this->values)); }

	protected:
		NumericValue values[N];
};

class Matrix2D : public ResolvedValuesPrimitive< 6 >
{
	public:
		Matrix2D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Matrix2D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Matrix3D : public ResolvedValuesPrimitive< 16 >
{
	public:
		Matrix3D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Matrix3D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class TranslateX : public UnresolvedValuesPrimitive< 1 >
{
	public:
		TranslateX(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new TranslateX(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class TranslateY : public UnresolvedValuesPrimitive< 1 >
{
	public:
		TranslateY(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new TranslateY(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class TranslateZ : public UnresolvedValuesPrimitive< 1 >
{
	public:
		TranslateZ(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new TranslateZ(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Translate2D : public UnresolvedValuesPrimitive< 2 >
{
	public:
		Translate2D(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Translate2D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Translate3D : public UnresolvedValuesPrimitive< 3 >
{
	public:
		Translate3D(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Translate3D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class ScaleX : public ResolvedValuesPrimitive< 1 >
{
	public:
		ScaleX(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new ScaleX(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class ScaleY : public ResolvedValuesPrimitive< 1 >
{
	public:
		ScaleY(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new ScaleY(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class ScaleZ : public ResolvedValuesPrimitive< 1 >
{
	public:
		ScaleZ(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new ScaleZ(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Scale2D : public ResolvedValuesPrimitive< 2 >
{
	public:
		Scale2D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Scale2D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Scale3D : public ResolvedValuesPrimitive< 3 >
{
	public:
		Scale3D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Scale3D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class RotateX : public ResolvedValuesPrimitive< 1 >
{
	public:
		RotateX(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new RotateX(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class RotateY : public ResolvedValuesPrimitive< 1 >
{
	public:
		RotateY(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new RotateY(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class RotateZ : public ResolvedValuesPrimitive< 1 >
{
	public:
		RotateZ(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new RotateZ(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Rotate2D : public ResolvedValuesPrimitive< 1 >
{
	public:
		Rotate2D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Rotate2D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Rotate3D : public ResolvedValuesPrimitive< 4 >
{
	public:
		Rotate3D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Rotate3D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class SkewX : public ResolvedValuesPrimitive< 1 >
{
	public:
		SkewX(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new SkewX(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class SkewY : public ResolvedValuesPrimitive< 1 >
{
	public:
		SkewY(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new SkewY(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Skew2D : public ResolvedValuesPrimitive< 2 >
{
	public:
		Skew2D(const NumericValue* values) throw()
			: ResolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Skew2D(*this); }
		bool ResolveTransform(Matrix4f& m, Element& e) const throw();
};

class Perspective : public UnresolvedValuesPrimitive< 1 >
{
	public:
		Perspective(const NumericValue* values) throw()
			: UnresolvedValuesPrimitive(values) { }

		inline Primitive* Clone() const { return new Perspective(*this); }
		bool ResolvePerspective(float& p, Element& e) const throw();
};

}
}
}

#endif
