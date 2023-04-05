#ifndef __MATH_UTIL__H_
#define __MATH_UTIL__H_
#include <math.h>

#define PI	3.14159265

namespace Jay
{
	inline double RadianToDegree(double radian)
	{
		return (radian * 180) / PI;
	}
	inline double DegreeToRadian(double degree)
	{
		return (degree * PI) / 180;
	}
}

#endif
