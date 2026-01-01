namespace Rml {

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha>::Colour(ColourType rgb, ColourType alpha) : red(rgb), green(rgb), blue(rgb), alpha(alpha)
{}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha>::Colour(ColourType red, ColourType green, ColourType blue, ColourType alpha) :
	red(red), green(green), blue(blue), alpha(alpha)
{}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator+(
	const Colour<ColourType, AlphaDefault, PremultipliedAlpha> rhs) const
{
	return Colour<ColourType, AlphaDefault, PremultipliedAlpha>(red + rhs.red, green + rhs.green, blue + rhs.blue, alpha + rhs.alpha);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator-(
	const Colour<ColourType, AlphaDefault, PremultipliedAlpha> rhs) const
{
	return Colour<ColourType, AlphaDefault, PremultipliedAlpha>(red - rhs.red, green - rhs.green, blue - rhs.blue, alpha - rhs.alpha);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator*(float rhs) const
{
	return Colour((ColourType)(red * rhs), (ColourType)(green * rhs), (ColourType)(blue * rhs), (ColourType)(alpha * rhs));
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
Colour<ColourType, AlphaDefault, PremultipliedAlpha> Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator/(float rhs) const
{
	return Colour((ColourType)(red / rhs), (ColourType)(green / rhs), (ColourType)(blue / rhs), (ColourType)(alpha / rhs));
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator+=(const Colour rhs)
{
	red += rhs.red;
	green += rhs.green;
	blue += rhs.blue;
	alpha += rhs.alpha;
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator-=(const Colour rhs)
{
	red -= rhs.red;
	green -= rhs.green;
	blue -= rhs.blue;
	alpha -= rhs.alpha;
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator*=(float rhs)
{
	red = (ColourType)(red * rhs);
	green = (ColourType)(green * rhs);
	blue = (ColourType)(blue * rhs);
	alpha = (ColourType)(alpha * rhs);
}

template <typename ColourType, int AlphaDefault, bool PremultipliedAlpha>
void Colour<ColourType, AlphaDefault, PremultipliedAlpha>::operator/=(float rhs)
{
	*this *= (1.0f / rhs);
}

} // namespace Rml
