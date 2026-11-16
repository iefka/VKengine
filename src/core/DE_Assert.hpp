#pragma once

#include<cassert>
#include<stdexcept>

#define DE_STRINGIFY_IMPL(x) #x
#define DE_STRINGIFY(x) DE_STRINGIFY_IMPL(x)

// Programmer error: a broken invariant, not external data.
// Debug  -> assert (breakpoint, stack, no unwinding).
// Release-> throw: assert is compiled out, a silent nullptr deref is not a diagnosis.
// Do NOT use inside destructors or noexcept functions: in Release that is std::terminate.
#ifdef NDEBUG
#  define DE_ASSERT(cond) \
	do { if (!(cond)) throw std::logic_error(#cond " @ " __FILE__ ":" DE_STRINGIFY(__LINE__)); } while (false)
#else
#  define DE_ASSERT(cond) assert(cond)
#endif