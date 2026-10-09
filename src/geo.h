#pragma once

// Great-circle distance in km between two WGS84 points (degrees).
double haversineKm(double lat1, double lon1, double lat2, double lon2);
// Initial bearing in degrees [0, 360) from point 1 to point 2.
double bearingDeg(double lat1, double lon1, double lat2, double lon2);
