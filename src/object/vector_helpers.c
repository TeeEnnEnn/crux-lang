#include <math.h>

#include "object/object.h"
#include "object/vector_helpers.h"

double compute_magnitude(const double *restrict components, const uint32_t dimensions)
{
	double sum = 0.0;
	for (uint32_t i = 0; i < dimensions; i++) {
		sum += components[i] * components[i];
	}
	return sqrt(sum);
}

double compute_dot_product(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions)
{
	double result = 0.0;
	for (uint32_t i = 0; i < dimensions; i++) {
		result += comp1[i] * comp2[i];
	}
	return result;
}

void compute_vector_add(double *restrict result, const double *restrict comp1, const double *restrict comp2,
						const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = comp1[i] + comp2[i];
	}
}

void compute_vector_subtract(double *restrict result, const double *restrict comp1, const double *restrict comp2,
							 const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = comp1[i] - comp2[i];
	}
}

void compute_scalar_multiply(double *restrict result, const double *restrict components, const double scalar,
							 const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = components[i] * scalar;
	}
}

void compute_scalar_divide(double *restrict result, const double *restrict components, const double scalar,
						   const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = components[i] / scalar;
	}
}

void compute_normalize(double *restrict result, const double *restrict components, const double magnitude,
					   const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = components[i] / magnitude;
	}
}

double compute_distance(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions)
{
	double sum = 0.0;
	for (uint32_t i = 0; i < dimensions; i++) {
		const double diff = comp1[i] - comp2[i];
		sum += diff * diff;
	}
	return sqrt(sum);
}

void compute_lerp(double *restrict result, const double *restrict comp1, const double *restrict comp2, const double t,
				  const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = comp1[i] + t * (comp2[i] - comp1[i]);
	}
}

void compute_reflect(double *restrict result, const double *restrict incident, const double *restrict normal,
					 const double normal_mag, const uint32_t dimensions)
{
	double dot = 0.0;
	for (uint32_t i = 0; i < dimensions; i++) {
		dot += incident[i] * (normal[i] / normal_mag);
	}

	for (uint32_t i = 0; i < dimensions; i++) {
		result[i] = incident[i] - 2.0 * dot * (normal[i] / normal_mag);
	}
}

bool compute_equals(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions)
{
	for (uint32_t i = 0; i < dimensions; i++) {
		if (fabs(comp1[i] - comp2[i]) >= EPSILON) {
			return false;
		}
	}
	return true;
}
