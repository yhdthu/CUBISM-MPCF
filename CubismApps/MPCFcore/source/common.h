/*
 *  TwoPhase.h
 *  
 *
 *  Created by Diego Rossinelli on 5/14/12.
 *  Copyright 2012 ETH Zurich. All rights reserved.
 *
 */

#pragma once
#ifdef _QPXEMU_
#include "QPXEMU.h"
#endif

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

#ifdef _FLOAT_PRECISION_
typedef float Real;
#else
typedef double Real;
#endif

#ifndef _PREC_LEVEL_
static const int preclevel = 0;
#else
static const int preclevel = _PREC_LEVEL_;
#endif

#include "SOA2D.h"

/*working dataset types */
typedef SOA2D<-3, _BLOCKSIZE_+3, -3, _BLOCKSIZE_+3> InputSOA; 
typedef RingSOA2D<-3, _BLOCKSIZE_+3, -3, _BLOCKSIZE_+3, 6> RingInputSOA; 
typedef SOA2D<0,_BLOCKSIZE_+1, 0,_BLOCKSIZE_> TempSOA; 
typedef RingSOA2D<0, _BLOCKSIZE_+1, 0,_BLOCKSIZE_, 2> RingTempSOA; 
typedef RingSOA2D<0,_BLOCKSIZE_+1, 0, _BLOCKSIZE_, 3> RingTempSOA3;


//C++ related functions
template<typename X> inline X mysqrt(X x){ abort(); return sqrt(x);}
template<>  inline float mysqrt<float>(float x){ return sqrtf(x);}
template<>  inline double mysqrt<double>(double x){ return sqrt(x);}

template<typename X> inline X myabs(X x){ abort(); return sqrt(x);}
template<>  inline float myabs<float>(float x){ return fabsf(x);}
template<>  inline double myabs<double>(double x){ return fabs(x);}

//che cos'e' questo odore? - non era un sospiro.
inline Real heaviside(const Real phi, const Real inv_h) //11 FLOP
{
	const Real x = min((Real)1, max((Real)-1, phi * inv_h));
	const Real val_xneg = (((Real)-0.5)*x - 1)*x + ((Real)0.5);
	const Real val_xpos = (((Real)+0.5)*x - 1)*x + ((Real)0.5);
	const Real hs = x < 0 ? val_xneg : val_xpos;
	
	return hs;
}

inline Real reconstruct(const Real y0, const Real y1, const Real phi, const Real inv_h)//15 FLOP
{
	const Real hs = heaviside(phi, inv_h);
	
	return y0*hs + y1*(1-hs);
}

inline Real getgamma(const Real phi, const Real smoothlength, const Real gamma0, const Real gamma1) 
{ 
	return reconstruct(gamma0, gamma1, phi, 1/smoothlength); 
} 

inline Real getPC(const Real phi, const Real smoothlength, const Real pc1, const Real pc2) 
{ 
	return reconstruct(pc1, pc2, phi, 1/smoothlength); 
}

#if defined(_QPX_) || defined(_QPXEMU_)

//QPX related functions
#ifdef _QPX_
#define _DIEGO_TRANSPOSE4(a, b, c, d)\
{\
const vector4double v01L = vec_perm(a, b, vec_gpci(00415));\
const vector4double v01H = vec_perm(a, b, vec_gpci(02637));\
const vector4double v23L = vec_perm(c, d, vec_gpci(00415));\
const vector4double v23H = vec_perm(c, d, vec_gpci(02637));\
\
a = vec_perm(v01L, v23L, vec_gpci(00145));\
b = vec_perm(v01L, v23L, vec_gpci(02367));\
c = vec_perm(v01H, v23H, vec_gpci(00145));\
d = vec_perm(v01H, v23H, vec_gpci(02367));\
}
#endif

template< int preclevel >
vector4double inline myreciprocal(vector4double a) 
{ 
	return vec_swdiv((vector4double)(1), a); 
}

template<>
vector4double inline myreciprocal<0>(vector4double a) 
{
	const vector4double Ra0 = vec_res(a);
	return vec_mul(Ra0, vec_nmsub(Ra0, a, vec_splats(2)));
	//return vec_nmsub(vec_mul(Ra0, a), Ra0, vec_add(Ra0, Ra0));
}

template<>
vector4double inline myreciprocal<-1>(vector4double a) { return vec_res(a); }

template< int preclevel>
int myreciprocal_flops()
{
	if (preclevel == 0) return 4;
	if (preclevel == -1) return 1;
	return 1; //we don't know if it is just 1
}

template< int preclevel >
vector4double inline mydivision(vector4double a, vector4double b) 
{ 
	return vec_swdiv(a, b); 
}

template<>
vector4double inline mydivision<0>(vector4double a, vector4double b) 
{ 
	return vec_mul(a, myreciprocal<0>(b)); 
}

template<>
vector4double inline mydivision<-1>(vector4double a, vector4double b) 
{ 
	return vec_mul(a, vec_res(b)); 
}

template< int preclevel>
int mydivision_flops()
{
	if (preclevel <= 0) return myreciprocal_flops<preclevel>() + 1;
	return 1; //we don't know if it is just 1
}

template<int preclevel>
inline vector4double mysqrt(const vector4double a)
{
	const vector4double invz =  vec_rsqrtes(a);
	const vector4double z = vec_res(invz);
	const vector4double tmp = vec_msub(z, z, a);
	const vector4double candidate = vec_nmsub(tmp, vec_mul(invz, vec_splats(0.5f)), z);
	return vec_sel(candidate, vec_splats(0.f), vec_neg(a));
}

template< int preclevel>
int mysqrt_flops()
{
	return 9;
}

inline vector4double mymin(const vector4double a, const vector4double b)
{
#ifdef _QPXEMU_
	return _mm_min_ps(a,b);
#else
	return vec_sel(a, b, vec_cmpgt(a, b));
#endif
}

inline vector4double mymax(const vector4double a, const vector4double b)
{
#ifdef _QPXEMU_
	return _mm_max_ps(a,b);
#else
	return vec_sel(a, b, vec_cmplt(a, b));
#endif
}

inline int myminmax_flops()
{
	return 2;
}
#else

template< int preclevel>
int myreciprocal_flops()
{
	if (preclevel == 0) return 4;
	if (preclevel == -1) return 1;
	return 1; //we don't know if it is just 1
}

template< int preclevel>
int mysqrt_flops()
{
	return 9;
}

template< int preclevel>
int mydivision_flops()
{
	if (preclevel <= 0) return myreciprocal_flops<preclevel>() + 1;
	return 1; //we don't know if it is just 1
}

inline int myminmax_flops()
{
	return 2;
}
#endif


//SSE-related functions
#ifdef _SSE_
#include <xmmintrin.h>

#ifdef __INTEL_COMPILER
inline __m128 operator+(__m128 a, __m128 b){ return _mm_add_ps(a, b); }
inline __m128 operator&(__m128 a, __m128 b){ return _mm_and_ps(a, b); }
inline __m128 operator|(__m128 a, __m128 b){ return _mm_or_ps(a, b); }
inline __m128 operator*(__m128 a, __m128 b){ return _mm_mul_ps(a, b); }
inline __m128 operator-(__m128 a,  __m128 b){ return _mm_sub_ps(a, b); }
inline __m128 operator/(__m128 a, __m128 b){ return _mm_div_ps(a, b); }
inline __m128d operator+(__m128d a, __m128d b){ return _mm_add_pd(a, b); }
inline __m128d operator*(__m128d a, __m128d b){ return _mm_mul_pd(a, b); }
inline __m128d operator-(__m128d a, __m128d b){ return _mm_sub_pd(a, b); }
inline __m128d operator/(__m128d a, __m128d b){ return _mm_div_pd(a, b); }
inline __m128d operator&(__m128d a, __m128d b){ return _mm_and_pd(a, b); }
inline __m128d operator|(__m128d a, __m128d b){ return _mm_or_pd(a, b); }
#endif

inline __m128 better_rcp(const __m128 a)
{
	const __m128 Ra0 = _mm_rcp_ps(a);
	return _mm_sub_ps(_mm_add_ps(Ra0, Ra0), _mm_mul_ps(_mm_mul_ps(Ra0, a), Ra0));
}

inline __m128 worse_sqrt(const __m128 a)
{	
	const __m128 invz =  _mm_rsqrt_ps(a);
	const __m128 z = _mm_rcp_ps(invz);
	const __m128 tmp =  z-(z*z-a)*invz*_mm_set_ps1(0.5f);
	return  _mm_and_ps(tmp, _mm_cmpgt_ps(a, _mm_setzero_ps()));
}

inline __m128 worse_sqrt_unsafe(const __m128 a)
{
	const __m128 invz =  _mm_rsqrt_ps(a);
	const __m128 z = _mm_rcp_ps(invz);
	return z-(z*z-a)*invz*_mm_set_ps1(0.5f);
}

inline __m128 myrcp(const __m128 a) 
{
#ifdef _PREC_DIV_
	return _mm_set1_ps(1.f)/a;
#else
	return better_rcp(a);
#endif
}

inline __m128 mysqrt(const __m128 a) 
{
#ifdef _PREC_DIV_
	return _mm_sqrt_ps(a);
#else
	const __m128 invz =  _mm_rsqrt_ps(a);
	const __m128 z = _mm_rcp_ps(invz);
	return z-(z*z-a)*invz*_mm_set_ps1(0.5f);
#endif
}

inline __m128 myrsqrt(const __m128 v) 
{
#ifdef _PREC_DIV_
	return _mm_div_ps(_mm_set1_ps(1.), _mm_sqrt_ps(v));
#else
	const __m128 approx = _mm_rsqrt_ps( v );
	const __m128 muls = _mm_mul_ps(_mm_mul_ps(v, approx), approx);
	return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), approx), _mm_sub_ps(_mm_set1_ps(3.f), muls) );
#endif
}

inline __m128 heaviside(const __m128 phi, const __m128 inv_h, const __m128 one, const __m128 phalf, const __m128 mhalf) 
{
	const __m128 x = _mm_min_ps(one, _mm_max_ps(_mm_setzero_ps() - one, phi*inv_h));
	
	const __m128 val_xneg = (mhalf*x - one)*x + phalf;
	const __m128 val_xpos = (phalf*x - one)*x + phalf;
	
	const __m128 flag = _mm_cmplt_ps(x, _mm_setzero_ps());
	
	return _mm_or_ps(_mm_and_ps(flag, val_xneg),_mm_andnot_ps(flag, val_xpos));
}

inline __m128d heaviside(const __m128d phi, const __m128d inv_h, const __m128d one, const __m128d phalf, const __m128d mhalf) 
{
	const __m128d x = _mm_min_pd(one, _mm_max_pd(_mm_setzero_pd() - one, phi*inv_h));
	
	const __m128d val_xneg = (mhalf*x - one)*x + phalf;
	const __m128d val_xpos = (phalf*x - one)*x + phalf;
	
	const __m128d flag = _mm_cmplt_pd(x, _mm_setzero_pd());
	
	return _mm_or_pd(_mm_and_pd(flag, val_xneg),_mm_andnot_pd(flag, val_xpos));
}

inline __m128 reconstruct(const __m128 y0, const __m128 y1, const __m128 phi, const __m128 inv_h, const __m128 one, const __m128 phalf, const __m128 mhalf) //15 FLOP
{
	const __m128 hs = heaviside(phi, inv_h, one, phalf, mhalf);
	
	return y0 * hs + y1 * (one - hs);
}

inline __m128d reconstruct(const __m128d y0, const __m128d y1, const __m128d phi, const __m128d inv_h, const __m128d one, const __m128d phalf, const __m128d mhalf) //15 FLOP
{
	const __m128d hs = heaviside(phi, inv_h, one, phalf, mhalf);
	
	return y0 * hs + y1 * (one - hs);
}

inline __m128 getgamma(const __m128 phi, const __m128 inv_smoothlength, 
					   const __m128 gamma1, const __m128 gamma2,
					   const __m128 F_1, const __m128 F_1_2, const __m128 M_1_2) 
{
	return reconstruct(gamma1, gamma2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

inline __m128d getgamma(const __m128d phi, const __m128d inv_smoothlength, 
						const __m128d gamma1, const __m128d gamma2,
						const __m128d F_1, const __m128d F_1_2, const __m128d M_1_2) 
{
	return reconstruct(gamma1, gamma2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

inline __m128 getPC(const __m128 phi, const __m128 inv_smoothlength, 
					const __m128 pc1, const __m128 pc2,
					const __m128 F_1, const __m128 F_1_2, const __m128 M_1_2) 
{
	return reconstruct(pc1, pc2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

inline __m128d getPC(const __m128d phi, const __m128d inv_smoothlength, 
					 const __m128d pc1, const __m128d pc2,
					 const __m128d F_1, const __m128d F_1_2, const __m128d M_1_2) 
{
	return reconstruct(pc1, pc2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

#ifndef _mm_set_pd1
#define _mm_set_pd1(a) (_mm_set_pd((a),(a)))
#endif

#define _3ORPS_(a,b,c) _mm_or_ps(a, _mm_or_ps(b,c))
#define _3ORPD_(a,b,c) _mm_or_pd(a, _mm_or_pd(b,c))

#define _4ORPS_(a,b,c,d) _mm_or_ps(a, _mm_or_ps(b, _mm_or_ps(c,d)))
#define _4ORPD_(a,b,c,d) _mm_or_pd(a, _mm_or_pd(b, _mm_or_pd(c,d)))
#endif //SSE

//AVX-related functions
#ifdef _AVX_
#include <immintrin.h>

#if defined(__INTEL_COMPILER)
inline __m256 operator+(__m256 a, __m256 b){ return _mm256_add_ps(a, b); }
inline __m256 operator&(__m256 a, __m256 b){ return _mm256_and_ps(a, b); }
inline __m256 operator|(__m256 a, __m256 b){ return _mm256_or_ps(a, b); }
inline __m256 operator*(__m256 a, __m256 b){ return _mm256_mul_ps(a, b); }
inline __m256 operator-(__m256 a, __m256 b){ return _mm256_sub_ps(a, b); }
inline __m256 operator/(__m256 a, __m256 b){ return _mm256_div_ps(a, b); }
inline __m256d operator+(__m256d a, __m256d b){ return _mm256_add_pd(a, b); }
inline __m256d operator*(__m256d a, __m256d b){ return _mm256_mul_pd(a, b); }
inline __m256d operator-(__m256d a, __m256d b){ return _mm256_sub_pd(a, b); }
inline __m256d operator/(__m256d a, __m256d b){ return _mm256_div_pd(a, b); }
inline __m256d operator&(__m256d a, __m256d b){ return _mm256_and_pd(a, b); }
inline __m256d operator|(__m256d a, __m256d b){ return _mm256_or_pd(a, b); }
#endif

inline __m256 heaviside(const __m256 phi, const __m256 inv_h, const __m256 one, const __m256 phalf, const __m256 mhalf)
{
	const __m256 x = _mm256_min_ps(one, _mm256_max_ps(_mm256_setzero_ps() - one, phi*inv_h));
	
	return (_mm256_blendv_ps(phalf, mhalf,  _mm256_cmp_ps(x, _mm256_setzero_ps(), _CMP_LT_OS))*x - one)*x + phalf;	
}

inline __m256 reconstruct(const __m256 y0, const __m256 y1, const __m256 phi, const __m256 inv_h, const __m256 one, const __m256 phalf, const __m256 mhalf) //15 FLOP
{
	const __m256 hs = heaviside(phi, inv_h, one, phalf, mhalf);
	
	return y0 * hs + y1 * (one - hs);
}

inline __m256 getgamma(const __m256 phi, const __m256 inv_smoothlength, 
					   const __m256 gamma1, const __m256 gamma2,
					   const __m256 F_1, const __m256 F_1_2, const __m256 M_1_2)
{
	return reconstruct(gamma1, gamma2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

inline __m256 getPC(const __m256 phi, const __m256 inv_smoothlength, 
					const __m256 pc1, const __m256 pc2,
					const __m256 F_1, const __m256 F_1_2, const __m256 M_1_2)
{
	return reconstruct(pc1, pc2, phi, inv_smoothlength, F_1, F_1_2, M_1_2);
}

inline __m256 better_rcp(const __m256 a)
{
	const __m256 Ra0 = _mm256_rcp_ps(a);
	return _mm256_sub_ps(_mm256_add_ps(Ra0, Ra0), _mm256_mul_ps(_mm256_mul_ps(Ra0, a), Ra0));
}

inline __m256 worse_sqrt(const __m256 a)
{
	const __m256 invz =  _mm256_rsqrt_ps(a);
	const __m256 z = _mm256_rcp_ps(invz);
	const __m256 tmp = z-(z*z-a)*invz*_mm256_set1_ps(0.5f);
	
	return  _mm256_and_ps(tmp, _mm256_cmp_ps(a, _mm256_setzero_ps(), _CMP_GT_OS));
}

inline __m256 myrsqrt(const __m256 v)
{
#ifdef _PREC_DIV_
	return _mm256_div_ps(_mm256_set1_ps(1.), _mm_sqrt_ps(v));
#else
	const __m256 approx = _mm256_rsqrt_ps( v );
	const __m256 muls = _mm256_mul_ps(_mm256_mul_ps(v, approx), approx);
	return _mm256_mul_ps(_mm256_mul_ps(_mm256_set1_ps(0.5f), approx), _mm256_sub_ps(_mm256_set1_ps(3.f), muls) );
#endif
}

#endif

inline void printKernelName(string kernelname)
{
	cout << endl;
	for (int i=0; i<100; i++)
		cout << "=";
	cout << endl << endl;
	cout << "KERNEL - " << kernelname.c_str() << endl << endl;
}

inline void printEndKernelTest()
{
	cout << endl;
	for (int i=0; i<100; i++)
		cout << "=";
	cout << endl << endl;
}

inline void printAccuracyTitle()
{
	string acc = " ACCURACY ";
	int l = (80-acc.size())/2;
	cout << "\t";
	for (int i=0; i<l; i++)
		cout << "-";
	cout << acc.c_str();
	cout << "";
	for (int i=0; i<l; i++)
		cout << "-";
	cout << endl;
}

inline void printPerformanceTitle()
{
	string perf = " PERFORMANCE ";
	int l = (80-perf.size())/2;
	cout << "\t";
	for (int i=0; i<l; i++)
		cout << "-";
	cout << perf.c_str();
	cout << "";
	for (int i=0; i<80-l-(int)perf.size(); i++)
		cout << "-";
	cout << endl;
}

inline void printEndLine()
{
	cout << "\t";
	for (int i=0; i<80; i++)
		cout << "-";
	cout << endl;
}

inline void awkAcc(string kernelname,
				   double linf0, double linf1, double linf2, double linf3, double linf4, double linf5,
				   double l10, double l11, double l12, double l13, double l14, double l15)
{
	cout << "awkAccLinf\t" <<  kernelname.c_str();
	cout << "\t" << setprecision(4) << linf0;
	cout << "\t" << setprecision(4) << linf1;
	cout << "\t" << setprecision(4) << linf2;
	cout << "\t" << setprecision(4) << linf3;
	cout << "\t" << setprecision(4) << linf4;
	cout << "\t" << setprecision(4) << linf5;
	cout << endl;
	
	cout << "awkAccL1\t" <<  kernelname.c_str();
	cout << "\t" << setprecision(4) << l10;
	cout << "\t" << setprecision(4) << l11;
	cout << "\t" << setprecision(4) << l12;
	cout << "\t" << setprecision(4) << l13;
	cout << "\t" << setprecision(4) << l14;
	cout << "\t" << setprecision(4) << l15;
	cout << endl;
}
