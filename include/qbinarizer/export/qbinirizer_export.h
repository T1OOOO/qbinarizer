
#ifndef QBINIRIZER_EXPORT_H
#define QBINIRIZER_EXPORT_H

#ifdef QBINIRIZER_STATIC_DEFINE
#  define QBINIRIZER_EXPORT
#  define QBINIRIZER_NO_EXPORT
#else
#  ifndef QBINIRIZER_EXPORT
#    ifdef qbinirizer_EXPORTS
        /* We are building this library */
#      define QBINIRIZER_EXPORT 
#    else
        /* We are using this library */
#      define QBINIRIZER_EXPORT 
#    endif
#  endif

#  ifndef QBINIRIZER_NO_EXPORT
#    define QBINIRIZER_NO_EXPORT 
#  endif
#endif

#ifndef QBINIRIZER_DEPRECATED
#  define QBINIRIZER_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef QBINIRIZER_DEPRECATED_EXPORT
#  define QBINIRIZER_DEPRECATED_EXPORT QBINIRIZER_EXPORT QBINIRIZER_DEPRECATED
#endif

#ifndef QBINIRIZER_DEPRECATED_NO_EXPORT
#  define QBINIRIZER_DEPRECATED_NO_EXPORT QBINIRIZER_NO_EXPORT QBINIRIZER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef QBINIRIZER_NO_DEPRECATED
#    define QBINIRIZER_NO_DEPRECATED
#  endif
#endif

#endif /* QBINIRIZER_EXPORT_H */
