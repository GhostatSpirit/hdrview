//
// Created by Wojciech Jarosz on 9/14/17.
//
#include "colorspace.h"
#include "common.h"
#include "color.h"
#include <cmath>

namespace
{
const float eps = 216.0f / 24389.0f;
const float kappa = 24389.0f / 27.0f;
const float refX = 0.950456f;
const float refZ = 1.08875f;
const float refU = (4.0f * refX) / (refX + 15.0f + 3.0f * refZ);
const float refV = 9.0f / (refX + 15.0f + 3.0f * refZ);

} // namespace

using namespace std;


float LinearToSRGB(float a)
{
	return a < 0.0031308 ? 12.92 * a : 1.055 * pow(a, 1.0/2.4) - 0.055;
}
float SRGBToLinear(float a)
{
	return a < 0.04045 ? (1.0 / 12.92) * a : pow((a + 0.055) * (1.0 / 1.055), 2.4);
}
float LinearToAdobeRGB(float a)
{
	return pow(a, 1.f / 2.19921875f);
}
float AdobeRGBToLinear(float a)
{
	return pow(a, 2.19921875f);
}

void sRGBToLinear(float * r, float * g, float * b)
{
	*r = SRGBToLinear(*r);
	*g = SRGBToLinear(*g);
	*b = SRGBToLinear(*b);
}
void LinearToSRGB(float *r, float *g, float *b)
{
	*r = LinearToSRGB(*r);
	*g = LinearToSRGB(*g);
	*b = LinearToSRGB(*b);
}
void AdobeRGBToLinear(float * r, float * g, float * b)
{
	*r = AdobeRGBToLinear(*r);
	*g = AdobeRGBToLinear(*g);
	*b = AdobeRGBToLinear(*b);
}
void LinearToAdobeRGB(float * r, float * g, float * b)
{
	*r = LinearToAdobeRGB(*r);
	*g = LinearToAdobeRGB(*g);
	*b = LinearToAdobeRGB(*b);
}

Color3 LinearToSRGB(const Color3 &c)
{
	return Color3(LinearToSRGB(c.r), LinearToSRGB(c.g), LinearToSRGB(c.b));
}

Color4 LinearToSRGB(const Color4 &c)
{
	return Color4(LinearToSRGB(reinterpret_cast<const Color3 &>(c)), c.a);
}

Color3 SRGBToLinear(const Color3 &c)
{
	return Color3(SRGBToLinear(c.r), SRGBToLinear(c.g), SRGBToLinear(c.b));
}

Color4 SRGBToLinear(const Color4 &c)
{
	return Color4(SRGBToLinear(reinterpret_cast<const Color3 &>(c)), c.a);
}



Color3 LinearToAdobeRGB(const Color3 &c)
{
	return Color3(LinearToAdobeRGB(c.r), LinearToAdobeRGB(c.g), LinearToAdobeRGB(c.b));
}

Color4 LinearToAdobeRGB(const Color4 &c)
{
	return Color4(LinearToAdobeRGB(reinterpret_cast<const Color3 &>(c)), c.a);
}

Color3 AdobeRGBToLinear(const Color3 &c)
{
	return Color3(AdobeRGBToLinear(c.r), AdobeRGBToLinear(c.g), AdobeRGBToLinear(c.b));
}

Color4 AdobeRGBToLinear(const Color4 &c)
{
	return Color4(AdobeRGBToLinear(reinterpret_cast<const Color3 &>(c)), c.a);
}


void XYZToLinearSRGB(float *R, float *G, float *B, float X, float Y, float Z)
{
	*R =  3.240479f * X - 1.537150f * Y - 0.498535f * Z;
	*G = -0.969256f * X + 1.875992f * Y + 0.041556f * Z;
	*B =  0.055648f * X - 0.204043f * Y + 1.057311f * Z;
}

void LinearSRGBToXYZ(float *X, float *Y, float *Z, float R, float G, float B)
{
	*X = 0.412453f * R + 0.357580f * G + 0.180423f * B;
	*Y = 0.212671f * R + 0.715160f * G + 0.072169f * B;
	*Z = 0.019334f * R + 0.119193f * G + 0.950227f * B;
}


void XYZToLinearAdobeRGB(float *R, float *G, float *B, float X, float Y, float Z)
{
	*R = X *  2.04159f + Y * -0.56501f + Z * -0.34473f;
	*G = X * -0.96924f + Y *  1.87597f + Z *  0.03342f;
	*B = X *  0.01344f + Y * -0.11836f + Z *  1.34926f;
}

void LinearAdobeRGBToXYZ(float *X, float *Y, float *Z, float R, float G, float B)
{
	*X = R * 0.57667f + G * 0.18556f + B * 0.18823f;
	*Y = R * 0.29734f + G * 0.62736f + B * 0.07529f;
	*Z = R * 0.02703f + G * 0.07069f + B * 0.99134f;
}

void XYZToLab(float *L, float *a, float *b, float X, float Y, float Z)
{
	X *= 1.0f / 0.95047f;
	Z *= 1.0f / 1.08883f;

	X = (X > eps) ? cbrt(X) : (kappa * X + 16.0f) / 116.0f;
	Y = (Y > eps) ? cbrt(Y) : (kappa * Y + 16.0f) / 116.0f;
	Z = (Z > eps) ? cbrt(Z) : (kappa * Z + 16.0f) / 116.0f;

	*L = (116.0f * Y) - 16.f;
	*a = 500.0f * (X - Y);
	*b = 200.0f * (Y - Z);
}


void LabToXYZ(float *X, float *Y, float *Z, float L, float a, float b)
{
	float yr = (L > kappa*eps) ? pow((L + 16.0f) / 116.0f, 3.f) : L / kappa;
	float fy = (yr > eps) ? (L + 16.0f) / 116.0f : (kappa*yr + 16.0f) / 116.0f;
	float fx = a / 500.0f + fy;
	float fz = fy - b / 200.0f;

	float fx3 = pow(fx, 3.f);
	float fz3 = pow(fz, 3.f);

	*X = (fx3 > eps) ? fx3 : (116.0f * fx - 16.0f) / kappa;
	*Y = yr;
	*Z = (fz3 > eps) ? fz3 : (116.0f * fz - 16.0f) / kappa;

	*X *= 0.950456f;
	*Z *= 1.08875f;
}

void normalizeLab(float * L, float * a, float * b)
{
	const float minLab[] = {0, -86.1846, -107.864};
	const float maxLab[] = {100, 98.2542, 94.4825};
	*L -= minLab[0];
	*a -= minLab[1];
	*b -= minLab[2];

	*L /= maxLab[0]-minLab[0];
	*a /= maxLab[1]-minLab[1];
	*b /= maxLab[2]-minLab[2];
}

void unnormalizeLab(float * L, float * a, float * b)
{
	const float minLab[] = {0, -86.1846, -107.864};
	const float maxLab[] = {100, 98.2542, 94.4825};

	*L *= maxLab[0]-minLab[0];
	*a *= maxLab[1]-minLab[1];
	*b *= maxLab[2]-minLab[2];

	*L += minLab[0];
	*a += minLab[1];
	*b += minLab[2];
}



void XYZToLuv(float *L, float *u, float *v, float X, float Y, float Z)
{
	float denom = 1.0f / (X + 15.0f * Y + 3.0f * Z);
	*u = (4.0f * X) * denom;
	*v = (9.0f * Y) * denom;

	*L = (Y > eps) ? (116.0f * cbrtf(Y)) - 16.0f : kappa * Y;
	*u = 13.0f * *L * (*u - refU);
	*v = 13.0f * *L * (*v - refV);
}


void LuvToXYZ(float *X, float *Y, float *Z, float L, float u, float v)
{
	*Y = (L > kappa * eps) ? pow((L + 16.0f) / 116.0f, 3.f) : L / kappa;

	float a = (1.0f/3.0f) * ((52.0f * L) / (u + 13.0f * L * refU) - 1.0f);
	float b = -5.0f * *Y;
	float d = *Y * ((39.0f * L) / (v + 13.0f * L * refV) - 5.0f);

	*X = (d - b) / (a + (1.0f/3.0f));
	*Z = *X * a + b;
}


void XYZToxy(float *x, float *y, float X, float Y, float Z)
{
	float denom = X + Y + Z;
	if (denom == 0.0f)
	{
		// set chromaticity to D65 whitepoint
		*x = 0.31271f;
		*y = 0.32902f;
	}
	else
	{
		*x = X / denom;
		*y = Y / denom;
	}
}


void xyYToXZ(float *X, float *Z, float x, float y, float Y)
{
	if (Y == 0.0f)
	{
		*X = 0.0f;
		*Z = 0.0f;
	}
	else
	{
		*X = x*Y;
		*Z = (1.0f - x - y) * Y / y;
	}
}


//! Convert a color in RGB colorspace to an equivalent color in HSV colorspace.
/*!
    This is derived from sample code in:

    Foley et al. Computer Graphics: Principles and Practice.
        Second edition in C. 592-596. July 1997.
*/
void RGBToHSV(float *H, float *S, float *V, float R, float G, float B)
{
	// Calculate the max and min of red, green and blue.
	float mx = max(R, G, B);
	float mn = min(R, G, B);
	float delta = mx - mn;

	// Set the saturation and value
	*S = (mx != 0) ? (delta)/mx : 0;
	*V = mx;

	if (*S == 0.0f)
		*H = 0.0f;
	else
	{
		if (R == mx)        *H = (G - B)/delta;
		else if (G == mx)   *H = 2 + (B - R)/delta;
		else if (B == mx)   *H = 4 + (R - G)/delta;

		*H /= 6.0f;

		if (*H < 0.0f)      *H += 1.0f;
	}
}


//! Convert a color in HSV colorspace to an equivalent color in RGB colorspace.
/*!
    This is derived from sample code in:

    Foley et al. Computer Graphics: Principles and Practice.
        Second edition in C. 592-596. July 1997.
*/
void HSVToRGB(float *R, float *G, float *B, float H, float S, float V)
{
	if (S == 0.0f)
	{
		// achromatic case
		*R = *G = *B = V;
	}
	else
	{
		if (H == 1.0f)
			H = 0.0f;
		else
			H *= 6.0f;

		int i = (int)floor(H);
		float f = H-i;
		float p = V * (1-S);
		float q = V * (1-(S*f));
		float t = V * (1-(S*(1-f)));

		switch (i)
		{
			case 0: *R = V; *G = t; *B = p; break;
			case 1: *R = q; *G = V; *B = p; break;
			case 2: *R = p; *G = V; *B = t; break;
			case 3: *R = p; *G = q; *B = V; break;
			case 4: *R = t; *G = p; *B = V; break;
			case 5: *R = V; *G = p; *B = q; break;
		}
	}
}


//! Convert a color in RGB colorspace to an equivalent color in HSS colorspace.
/*!
    This is derived from sample code in:

    Foley et al. Computer Graphics: Principles and Practice.
        Second edition in C. 592-596. July 1997.
*/
void RGBToHSL(float *H, float *S, float *L, float R, float G, float B)
{
	// Calculate the max and min of red, green and blue.
	float mx = max(R, G, B);
	float mn = min(R, G, B);

	// Set the saturation and value
	*L = 0.5f * (mx + mn);

	if (mx == mn)
	{
		*H = 0;
		*S = 0;
	}
	else
	{
		float delta = mx - mn;

		*S = (*L <= 0.5f) ? delta / (mx + mn) : delta / (2 - mx - mn);

		if (R == mx)
			*H = (G - B) / delta;
		else if (G == mx)
			*H = 2 + (B - R) / delta;
		else if (B == mx)
			*H = 4 + (R - G) / delta;

		*H /= 6.0f;

		if (*H < 0.0f)
			*H += 1.0f;
	}
}


inline float HueToRGB(float n1, float n2, float hue)
{
	if (hue > 1.0f)
		hue -= 1.0f;
	else if (hue < 0.0f)
		hue += 1.0f;

	if (6 * hue < 1.f)  return n1 + 6 * (n2 - n1)*hue;
	if (2 * hue < 1.f)  return n2;
	if (3 * hue < 2.f)  return n1 + 6 * (n2 - n1)* (2.0f/3.0f - hue);

	return n1;
}


//! Convert a color in HSL colorspace to an equivalent color in RGB colorspace.
/*!
    This is derived from sample code in:

    Foley et al. Computer Graphics: Principles and Practice.
        Second edition in C. 592-596. July 1997.
*/
void HSLToRGB(float *R, float *G, float *B, float H, float S, float L)
{
	if (S == 0)
	{
		// achromatic case
		*R = *G = *B = L;
	}
	else
	{
		// chromatic case
		float m1, m2;
		m2 = (L <= 0.5f) ? L * (1 + S) : L + S - L*S;
		m1 = 2*L - m2;

		*R = HueToRGB(m1, m2, H + 1.0f / 3.0f);
		*G = HueToRGB(m1, m2, H);
		*B = HueToRGB(m1, m2, H - 1.0f / 3.0f);
	}
}

void XYZToHSL(float *H, float *S, float *L, float X, float Y, float Z)
{
	float R, G, B;
	XYZToLinearSRGB(&R, &G, &B, X, Y, Z);
	RGBToHSL(H, S, L, R, G, B);
}

void HSLToXYZ(float *X, float *Y, float *Z, float H, float S, float L)
{
	float R, G, B;
	HSLToRGB(&R, &G, &B, H, S, L);
	LinearSRGBToXYZ(X, Y, Z, R, G, B);
}

void XYZToHSV(float *H, float *S, float *V, float X, float Y, float Z)
{
	float R, G, B;
	XYZToLinearSRGB(&R, &G, &B, X, Y, Z);
	RGBToHSV(H, S, V, R, G, B);
}

void HSVToXYZ(float *X, float *Y, float *Z, float H, float S, float V)
{
	float R, G, B;
	HSVToRGB(&R, &G, &B, H, S, V);
	LinearSRGBToXYZ(X, Y, Z, R, G, B);
}



void convertColorSpace(EColorSpace dst, float *a, float *b, float *c,
                       EColorSpace src, float A, float B, float C)
{
	// always convert between the color spaces by way of XYZ to reduce the combinations
	float X, Y, Z;
	switch (src)
	{
		case LinearSRGB_CS: LinearSRGBToXYZ(&X, &Y, &Z, A, B, C); break;
		case LinearAdobeRGB_CS: LinearAdobeRGBToXYZ(&X, &Y, &Z, A, B, C); break;
		case CIELab_CS: unnormalizeLab(&A, &B, &C); LabToXYZ(&X, &Y, &Z, A, B, C); break;
		case CIELuv_CS: LuvToXYZ(&X, &Y, &Z, A, B, C); break;
		case CIExyY_CS: xyYToXZ(&X, &Z, A, B, C); Y = C; break;
		case HLS_CS: HSLToXYZ(&X, &Y, &Z, A, B, C); break;
		case HSV_CS: HSVToXYZ(&X, &Y, &Z, A, B, C); break;
		default:  X = A; Y = B; Z = C;              // XYZ
	}

	// now convert from XYZ to the destination color space
	switch (dst)
	{
		case LinearSRGB_CS: XYZToLinearSRGB(a, b, c, X, Y, Z); break;
		case LinearAdobeRGB_CS: XYZToLinearAdobeRGB(a, b, c, X, Y, Z); break;
		case CIELab_CS: XYZToLab(a, b, c, X, Y, Z); normalizeLab(a, b, c); break;
		case CIELuv_CS: XYZToLuv(a, b, c, X, Y, Z); break;
		case CIExyY_CS: XYZToxy(a, b, X, Y, Z); *c = Y; break;
		case HLS_CS: XYZToHSL(a, b, c, X, Y, Z); break;
		case HSV_CS: XYZToHSV(a, b, c, X, Y, Z); break;
		default:  *a = X; *b = Y; *c = Z;          // XYZ
	}
}

const vector<string> & colorSpaceNames()
{
	static const vector<string> names =
		{
			"Linear sRGB",
			"Linear Adobe RGB",
			"CIE XYZ",
			"CIE L*a*b*",
			"CIE L*u*v*",
			"CIE xyY",
			"HSL",
			"HSV"
		};
	return names;
}