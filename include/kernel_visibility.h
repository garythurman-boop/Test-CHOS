/* Flat kernels have no dynamic loader to relocate absolute GOT pointers.
 * Include before every C declaration, including external assembly/linker symbols. */
#ifndef __ASSEMBLER__
#pragma GCC visibility push(hidden)
#endif
