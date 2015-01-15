#ifndef SPHERICAL_COORDINATES_HPP
#define SPHERICAL_COORDINATES_HPP

/*
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
struct sphericalCoordinates {
	int mPhi;
	int mTheta;

	bool operator==(const sphericalCoordinates &other) const
	{
		return (mPhi == other.mPhi && mTheta == other.mTheta);
	}
};

/*
 * Code based on:
 * http://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 */
namespace std {
	template<>
	struct hash<sphericalCoordinates>
	{
		std::size_t operator()(const sphericalCoordinates& k) const
		{
			// Compute individual hash values for mPhi and mTheta
			// and combine them using XOR and bit shifting:
			return ((hash<int>()(k.mPhi) ^ (hash<int>()(k.mTheta) << 1)));
		}
	};
}

double degreesToRadians(double aDegrees)
{
	return (aDegrees/180 * M_PI);
}

#endif /* SPHERICAL_COORDINATES_HPP */
