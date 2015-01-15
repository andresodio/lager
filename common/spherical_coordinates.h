#ifndef SPHERICAL_COORDINATES_HPP
#define SPHERICAL_COORDINATES_HPP

/*
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
struct SphericalCoordinates {
	int phi;
	int theta;

	bool operator==(const SphericalCoordinates &other) const
	{
		return (phi == other.phi && theta == other.theta);
	}
};

/*
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
namespace std {
	template<>
	struct hash<SphericalCoordinates>
	{
		std::size_t operator()(const SphericalCoordinates& k) const
		{
			// Compute individual hash values for mPhi and mTheta
			// and combine them using XOR and bit shifting:
			return ((hash<int>()(k.phi) ^ (hash<int>()(k.theta) << 1)));
		}
	};
}

double DegreesToRadians(double degrees)
{
	return (degrees/180 * M_PI);
}

#endif /* SPHERICAL_COORDINATES_HPP */
