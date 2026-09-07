# Type Masks `src/_headers/value.h:47`

```c
#define NIL_TYPE    (1<<0)
#define BOOL_TYPE   (1<<1)
#define INT_TYPE    (1<<2)
#define FLOAT_TYPE  (1<<3)
#define STRING_TYPE (1<<4)
#define ARRAY_TYPE  (1<<5)
#define TABLE_TYPE  (1<<6)
#define ERROR_TYPE  (1<<7)
#define RESULT_TYPE (1<<8)
#define RANDOM_TYPE (1<<9)
#define FILE_TYPE   (1<<10)
#define STRUCT_TYPE (1<<11)
#define VECTOR_TYPE (1<<12)
#define COMPLEX_TYPE(1<<13)
#define MATRIX_TYPE (1<<14)
#define TUPLE_TYPE  (1<<15)
#define BUFFER_TYPE (1<<16)
#define RANGE_TYPE  (1<<17)
#define ITERATOR_TYPE(1<<18)
#define OPTION_TYPE (1<<19)
// ... + NEVER=1<<30 ANY=1<<31
#define NUMERIC_TYPE (INT|FLOAT)
#define HASHABLE_TYPE (NIL|INT|FLOAT|BOOL|STRING)
```

Union `A|B` → `UNI(ARGS(...),NAMES(...),2)` `native_registration.c:54` order independent.
