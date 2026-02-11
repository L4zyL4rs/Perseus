#include <Quickmath.h>
//using namespace Windows::Foundation::Numerics;
//
//float Quickmath::acos(float x) {
//	float negate = float(x < 0);
//	x = abs(x);
//	float ret = -0.0187293;
//	ret = ret * x;
//	ret = ret + 0.0742610;
//	ret = ret * x;
//	ret = ret - 0.2121144;
//	ret = ret * x;
//	ret = ret + 1.5707288;
//	ret = ret * sqrt(1.0 - x);
//	ret = ret - 2 * negate * ret;
//	return negate * 3.14159265358979 + ret;
//}
//
//float quickmath::sin(float a)
//{
//	/* c simulation gives a max absolute error of less than 1.8e-7 */
//	float4 c0 = float4(0.0, 0.5,
//		1.0, 0.0);
//	float4 c1 = float4(0.25, -9.0,
//		0.75, 0.159154943091);
//	float4 c2 = float4(24.9808039603, -24.9808039603,
//		-60.1458091736, 60.1458091736);
//	float4 c3 = float4(85.4537887573, -85.4537887573,
//		-64.9393539429, 64.9393539429);
//	float4 c4 = float4(19.7392082214, -19.7392082214,
//		-1.0, 1.0);
//
//	/* r0.x = sin(a) */
//	float3 r0, r1, r2;
//
//	r1.x = c1.w * a - c1.x;                // only difference from cos!
//	r1.y = frac(r1.x);                   // and extract fraction
//	r2.x = (float)(r1.y < c1.x);        // range check: 0.0 to 0.25
//	r2.yz = (float2)(r1.yy >= c1.yz);    // range check: 0.75 to 1.0
//	r2.y = dot(r2, c4.zwz);              // range check: 0.25 to 0.75
//	r0 = c0.xyz - r1.yyy;                // range centering
//	r0 = r0 * r0;
//	r1 = c2.xyx * r0 + c2.zwz;           // start power series
//	r1 = r1 * r0 + c3.xyx;
//	r1 = r1 * r0 + c3.zwz;
//	r1 = r1 * r0 + c4.xyx;
//	r1 = r1 * r0 + c4.zwz;
//	r0.x = dot(r1, -r2);                 // range extract
//
//	return r0.x;
//}
//
//float quickmath::frac(float v)
//{
//	return v - floor(v);
//}