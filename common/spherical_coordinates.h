#ifndef LAGER_COMMON_SPHERICAL_COORDINATES_H
#define LAGER_COMMON_SPHERICAL_COORDINATES_H

#include <math.h>

/**
 * Structure that describes spherical coordinates, consisting of a theta
 * (polar) angle and a phi (azimuthal) angle.
 *
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
struct SphericalCoordinates {
	int theta;
  int phi;

	bool operator==(const SphericalCoordinates &other) const
	{
		return (theta == other.theta && phi == other.phi);
	}
};

/*
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
namespace std {
	template<>
	/**
	 * Structure that allows the use of the custom Spherical Coordinates class
	 * as an unordered map key.
	 */
	struct hash<SphericalCoordinates>
	{
	  /**
	   * Function call operator() overloading that computes a hash value for
	   * SphericalCoordinates structures.
	   */
		std::size_t operator()(const SphericalCoordinates& k) const
		{
			// Compute individual hash values for mTheta and mPhi
			// and combine them using XOR and bit shifting:
			return ((hash<int>()(k.theta) ^ (hash<int>()(k.phi) << 1)));
		}
	};
}

/**
 * Converts degrees to radians.
 */
double DegreesToRadians(double degrees)
{
	return (degrees/180 * M_PI);
}

#endif /* LAGER_COMMON_SPHERICAL_COORDINATES_H */
