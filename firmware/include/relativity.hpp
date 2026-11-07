#pragma once
// Relativistic stellar-light Doppler and aberration for STAR VOYAGER.
// Physics: observed frequency ratio D = gamma(1 + beta*cos_theta),
// where cos_theta=+1 means a source directly ahead of the travel direction.
// Blackbody approximation: observed thermal spectrum appears at D*T.
// This is a visible-band approximation, NOT a stellar spectral catalogue.
#include "astro.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace relativity {
// Arduino-ESP32 2.0.x uses C++11 by default; std::clamp requires C++17.
// Keep this helper local instead of changing the compiler standard.
template <class T>
inline T clampValue(T v, T lo, T hi) { return std::max(lo, std::min(hi, v)); }

inline double factor(double beta, double mu) {
    beta = clampValue(beta,0.0,0.999999999999);
    mu = clampValue(mu,-1.0,1.0);
    return (1.0+beta*mu)/std::sqrt((1.0-beta)*(1.0+beta));
}
inline astro::V aberrate(astro::V source, astro::V motion, double beta){
    // Exact special-relativistic aberration of a direction TO a light source.
    if(beta<1.e-9)return source;
    double gamma=1.0/std::sqrt(std::max(1.e-16,(1.0-beta)*(1.0+beta)));
    double mu=astro::dot(source,motion);
    double den=gamma*(1.+beta*mu);
    return (source + motion*((gamma-1.)*mu+gamma*beta))/den;
}
inline double planckRelative(double wavelengthNm,double doppler) {
    // B_lambda(D*T) / B_lambda(T) at fixed detector wavelength;
    // D is a frequency shift and an observer-frame brightness shift.
    // Approximate every unresolved star as T0=6500K.
    constexpr double HC_OVER_K=14387768.775; // nm*K
    constexpr double T0=6500.;
    auto occupation=[](double x){
        if(x>80.)return std::exp(-x); // avoids exp overflow
        return 1.0/std::expm1(x);
    };
    double x=HC_OVER_K/(wavelengthNm*T0);
    double d=clampValue(doppler,0.001,1000.);
    return occupation(x/d)/occupation(x);
}
struct Sample {uint16_t rgb565;uint8_t visible;};
inline Sample sample(double doppler){
    double r=planckRelative(650.,doppler),g=planckRelative(545.,doppler),b=planckRelative(450.,doppler);
    double peak=std::max({r,g,b,1.e-30});
    double light=clampValue(std::pow(peak,.40),0.0,2.0);
    // Preserve spectral hue independently of exposure. Excess high-speed
    // brightness is clamped to fit 8-bit RGB output (screen HDR simulation).
    double bright=clampValue(light,0.0,1.0);
    int R=(int)std::lround(255.*bright*r/peak);
    int G=(int)std::lround(255.*bright*g/peak);
    int B=(int)std::lround(255.*bright*b/peak);
    uint16_t rgb=((R>>3)<<11)|((G>>2)<<5)|(B>>3);
    return {rgb,(uint8_t)std::lround(std::min(255.,std::max(0.,light*255.)))};
}
// Quantize directional cosine (1D table, recalculated only when beta changes).
// At render time each star uses only dot product + lookup, not Planck/exps.
struct Palette {
    static constexpr int N=128;
    Sample values[N+1]{};
    double lastBeta=-1.;
    void update(double beta){
        if(std::abs(beta-lastBeta)<.002)return;
        lastBeta=beta;
        for(int i=0;i<=N;i++)values[i]=sample(factor(beta,2.*i/N-1.));
    }
    const Sample& lookup(double mu)const{
        int i=(int)((clampValue(mu,-1.0,1.0)+1.)*.5*N+.5);
        return values[clampValue(i,0,N)];
    }
};
} // namespace relativity
