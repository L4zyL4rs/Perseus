#pragma once

#include <iostream>
#include <cmath>



class Int64vector {
public:
	int64_t x, y, z;

	Int64vector(int64_t x = 0, int64_t y = 0, int64_t z = 0);
	double length();
	Int64vector normalized();
};

Int64vector operator+(const Int64vector& vec1, const Int64vector& vec2);
Int64vector operator-(const Int64vector& vec1, const Int64vector& vec2);
double operator*(const Int64vector& vec1, const Int64vector& vec2);
Int64vector operator*(const int64_t& a, const Int64vector& vec);
Int64vector operator*(const Int64vector& vec, const double& a);
Int64vector operator*(const double& a, const Int64vector& vec);