#include "geo.h"

#include <math.h>

namespace {
constexpr double DEG2RAD = M_PI / 180.0;
}

double haversineKm(double lat1, double lon1, double lat2, double lon2) {
  double dlat = (lat2-lat1)*DEG2RAD, dlon = (lon2-lon1)*DEG2RAD;
  double a = sin(dlat/2)*sin(dlat/2) + cos(lat1*DEG2RAD)*cos(lat2*DEG2RAD)*sin(dlon/2)*sin(dlon/2);
  return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0-a));
}
double bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double dlon=(lon2-lon1)*DEG2RAD;
  double y=sin(dlon)*cos(lat2*DEG2RAD);
  double x=cos(lat1*DEG2RAD)*sin(lat2*DEG2RAD)-sin(lat1*DEG2RAD)*cos(lat2*DEG2RAD)*cos(dlon);
  return fmod(atan2(y,x)/DEG2RAD+360.0,360.0);
}
