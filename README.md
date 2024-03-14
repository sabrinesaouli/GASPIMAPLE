DIRECTORY OVERVIEW:
================================================================================
mtl/            Mini Template Library
utils/          Generic helper code (I/O, Parsing, CPU-time, etc)
core/           A core version of the solver
simp/           An extended solver with simplification capabilities
README
LICENSE

To build: (release version: without assertions, statically linked, etc)
================================================================================

export MROOT=<maple-dir>              (or setenv in cshell)
cd simp
make rs

Usage:
================================================================================
To run maple with gaspi initialization:

`gaspi_maple.sh <NUM_GENERATIONS> <POPULATION_SIZE> <MUTATION_RATE> <CROSSOVER_RATE> <CNF_FILE>`
