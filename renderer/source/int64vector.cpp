#include "int64vector.h"

Int64vector operator+(const Int64vector& vec1, const Int64vector& vec2)
{
	Int64vector temp;
	temp.x = vec1.x + vec2.x;
	temp.y = vec1.y + vec2.y;
	temp.z = vec1.z + vec2.z;

	return temp;
}

Int64vector operator-(const Int64vector& vec1, const Int64vector& vec2)
{
	Int64vector temp;
	temp.x = vec1.x - vec2.x;
	temp.y = vec1.y - vec2.y;
	temp.z = vec1.z - vec2.z;

	return temp;
}

double operator*(const Int64vector& vec1, const Int64vector& vec2)
{
	double vec1x = static_cast<double>(vec1.x);
	double vec1y = static_cast<double>(vec1.y);
	double vec1z = static_cast<double>(vec1.z);

	double vec2x = static_cast<double>(vec2.x);
	double vec2y = static_cast<double>(vec2.y);
	double vec2z = static_cast<double>(vec2.z);

	return vec1x * vec2x + vec1y * vec2y + vec1z * vec2z;
}

Int64vector operator*(const int64_t& a, const Int64vector& vec)
{
	Int64vector temp;
	temp.x = a * vec.x;
	temp.y = a * vec.y;
	temp.z = a * vec.z;

	return temp;
}

Int64vector operator*(const Int64vector& vec, const double& a)
{
	Int64vector temp;
	temp.x = int(a) * vec.x;
	temp.y = int(a) * vec.y;
	temp.z = int(a) * vec.z;

	return temp;
}

Int64vector operator*(const double& a, const Int64vector& vec)
{
	return vec * a;
}

Int64vector::Int64vector(int64_t x /*= 0*/, int64_t y /*= 0*/, int64_t z /*= 0*/) : x(x), y(y), z(z) {}

double Int64vector::length()
{
	double vecx = static_cast<double>((*this).x);
	double vecy = static_cast<double>((*this).y);
	double vecz = static_cast<double>((*this).z);
	return std::sqrt(vecx * vecx + vecy * vecy + vecz * vecz);
}

Int64vector Int64vector::normalized()
{
	Int64vector res;
	res.x = int((1 / this->length()) * this->x);
	res.y = int((1 / this->length()) * this->y);
	res.z = int((1 / this->length()) * this->z);
	return res;
}
