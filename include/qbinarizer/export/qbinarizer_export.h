
#ifndef QBINARIZER_EXPORT_H
#define QBINARIZER_EXPORT_H

#ifdef QBINARIZER_STATIC_DEFINE
#  define QBINARIZER_EXPORT
#  define QBINARIZER_NO_EXPORT
#else
#  ifndef QBINARIZER_EXPORT
#    ifdef qbinarizer_EXPORTS
        /* We are building this library */
#      define QBINARIZER_EXPORT 
#    else
        /* We are using this library */
#      define QBINARIZER_EXPORT 
#    endif
#  endif

#  ifndef QBINARIZER_NO_EXPORT
#    define QBINARIZER_NO_EXPORT 
#  endif
#endif

#ifndef QBINARIZER_DEPRECATED
#  define QBINARIZER_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef QBINARIZER_DEPRECATED_EXPORT
#  define QBINARIZER_DEPRECATED_EXPORT QBINARIZER_EXPORT QBINARIZER_DEPRECATED
#endif

#ifndef QBINARIZER_DEPRECATED_NO_EXPORT
#  define QBINARIZER_DEPRECATED_NO_EXPORT QBINARIZER_NO_EXPORT QBINARIZER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef QBINARIZER_NO_DEPRECATED
#    define QBINARIZER_NO_DEPRECATED
#  endif
#endif

#endif /* QBINARIZER_EXPORT_H */
