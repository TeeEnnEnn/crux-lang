This contains the Crux source code

- `compiler`: This contains the source code for the crux compiler
- `internal_headers`: This contains the internal header files used by Crux. For the public embedding API check `/include/crux.h`
- `memory`: This contains the garbage collector related source code
- `native`: This contains native C functions that are compiled with and accessible from Crux
- `type_system`: This contains type system related source code
- `vm`: This contains the main Crux VM source. This is the heart of Crux. It contains the main dispatch loop. 

All of these sources are required to compile Crux. Nothing is optional.
