#ifndef CRUX_LANG_MATRIX_H
#define CRUX_LANG_MATRIX_H

#include "object.h"
#include "vm.h"

/* ── Construction ──────────────────────────────────────────────────────────── */

/* new_matrix_function(rows, cols) -> Result<Matrix>
 * new_matrix_identity_function(n)  -> Result<Matrix>   (n×n identity)
 * new_matrix_from_array_function(rows, cols, array) -> Result<Matrix>
 */
CruxValue new_matrix_function(CruxVM *vm, const CruxValue *args);
CruxValue new_matrix_identity_function(CruxVM *vm, const CruxValue *args);
CruxValue new_matrix_from_array_function(CruxVM *vm, const CruxValue *args);

/* ── Element access ────────────────────────────────────────────────────────── */

/* get(row, col) -> Result<float>   set(row, col, val) -> Result<nil> */
CruxValue matrix_get_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_set_method(CruxVM *vm, const CruxValue *args);

/* ── Infallible properties ─────────────────────────────────────────────────── */

CruxValue matrix_rows_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_cols_method(CruxVM *vm, const CruxValue *args);

/* ── Arithmetic ────────────────────────────────────────────────────────────── */

CruxValue matrix_add_value(CruxVM *vm, const ObjectMatrix *a, const ObjectMatrix *b);
CruxValue matrix_subtract_value(CruxVM *vm, const ObjectMatrix *a, const ObjectMatrix *b);
CruxValue matrix_multiply_value(CruxVM *vm, const ObjectMatrix *a, const ObjectMatrix *b);
CruxValue matrix_scale_value(CruxVM *vm, const ObjectMatrix *mat, double scalar);
CruxValue matrix_scalar_add_value(CruxVM *vm, const ObjectMatrix *mat, double scalar);
CruxValue matrix_scalar_subtract_value(CruxVM *vm, const ObjectMatrix *mat, double scalar);
CruxValue scalar_matrix_subtract_value(CruxVM *vm, double scalar, const ObjectMatrix *mat);
CruxValue matrix_scalar_divide_value(CruxVM *vm, const ObjectMatrix *mat, double scalar);

CruxValue matrix_add_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_subtract_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_multiply_method(CruxVM *vm, const CruxValue *args);   /* mat×mat or mat×scalar */
CruxValue matrix_scale_method(CruxVM *vm, const CruxValue *args);      /* mat × scalar */

/* ── Linear-algebra operations ─────────────────────────────────────────────── */

CruxValue matrix_transpose_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_determinant_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_inverse_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_trace_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_rank_method(CruxVM *vm, const CruxValue *args);

/* ── Row operations (return new matrix) ────────────────────────────────────── */

CruxValue matrix_row_method(CruxVM *vm, const CruxValue *args);   /* row(i) -> Array */
CruxValue matrix_col_method(CruxVM *vm, const CruxValue *args);   /* col(j) -> Array */

/* ── Utilities ─────────────────────────────────────────────────────────────── */

CruxValue matrix_equals_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_copy_method(CruxVM *vm, const CruxValue *args);
CruxValue matrix_to_array_method(CruxVM *vm, const CruxValue *args); /* -> Array of Arrays */

/* ── Vector interop ────────────────────────────────────────────────────────── */

CruxValue matrix_multiply_vector_method(CruxVM *vm, const CruxValue *args); /* M × v -> Vector */

#endif // CRUX_LANG_MATRIX_H
