#pragma once
// STAR VOYAGER R2: Voyager 1/2 approximate sun-centered snapshots.
// Source: Horizons-derived geocentric apparent ICRF RA/Dec/range on
// 2026-11-04 08:26:49 UTC; propagated at published radial escape speeds.
// These are not Horizons heliocentric Cartesian state vectors. Error grows
// with elapsed time, but the visual navigation is meaningful near the epoch.
#include "astro.hpp"
namespace astro {
constexpr double VOYAGER_EPOCH_JD = 2461348.8519560187;
struct VoyagerSpec {const char* name; double ra,dec,earthAU,speed;};
constexpr VoyagerSpec VOYAGER_SPECS[2]={
 {"VOYAGER 1",259.0734742458,12.0590948111,172.906356521176,16900.},
 {"VOYAGER 2",302.9835652791,-59.5395249278,144.655937927078,15300.}
};
inline V voyagerPosition(int id,double jd) {
 const auto &p=VOYAGER_SPECS[id];
 double ra=p.ra*DEG,dec=p.dec*DEG,obl=23.43928*DEG;
 // ICRF equatorial to J2000 mean ecliptic (approximate).
 double x=cos(dec)*cos(ra),y=cos(dec)*sin(ra),z=sin(dec);
 V ecliptic{x,cos(obl)*y+sin(obl)*z,-sin(obl)*y+cos(obl)*z};
 V start=planet(2,VOYAGER_EPOCH_JD)+ecliptic*(p.earthAU*AU);
 V radial=unit(start)*p.speed;
 return start+radial*((jd-VOYAGER_EPOCH_JD)*86400.);
}
} // namespace astro
