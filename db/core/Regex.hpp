#pragma once

// Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562
#if __GNUC__ >= 12
#    if defined __SANITIZE_ADDRESS__ && defined __OPTIMIZE__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#    endif
#endif

#include <regex>

#if __GNUC__ >= 12
#    pragma GCC diagnostic pop
#endif
