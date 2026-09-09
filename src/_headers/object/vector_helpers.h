#include <stdbool.h>
#include <stdint.h>

#define EPSILON 1e-10

double compute_magnitude(const double *restrict components, const uint32_t dimensions);

double compute_dot_product(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions);

void compute_vector_add(double *restrict result, const double *restrict comp1, const double *restrict comp2,
						const uint32_t dimensions);

void compute_vector_subtract(double *restrict result, const double *restrict comp1, const double *restrict comp2,
							 const uint32_t dimensions);

void compute_scalar_multiply(double *restrict result, const double *restrict components, const double scalar,
							 const uint32_t dimensions);

void compute_scalar_divide(double *restrict result, const double *restrict components, const double scalar,
						   const uint32_t dimensions);

void compute_normalize(double *restrict result, const double *restrict components, const double magnitude,
					   const uint32_t dimensions);

double compute_distance(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions);

void compute_lerp(double *restrict result, const double *restrict comp1, const double *restrict comp2, const double t,
				  const uint32_t dimensions);

void compute_reflect(double *restrict result, const double *restrict incident, const double *restrict normal,
					 const double normal_mag, const uint32_t dimensions);

bool compute_equals(const double *restrict comp1, const double *restrict comp2, const uint32_t dimensions);
