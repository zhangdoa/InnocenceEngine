
#ifndef InnoLowLevelSystem_EXPORT_H
#define InnoLowLevelSystem_EXPORT_H

#ifdef InnoLowLevelSystem_BUILT_AS_STATIC
#  define InnoLowLevelSystem_EXPORT
#  define INNOLOWLEVELSYSTEM_NO_EXPORT
#else
#  ifndef InnoLowLevelSystem_EXPORT
#    ifdef InnoLowLevelSystem_EXPORTS
        /* We are building this library */
#      define InnoLowLevelSystem_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define InnoLowLevelSystem_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef INNOLOWLEVELSYSTEM_NO_EXPORT
#    define INNOLOWLEVELSYSTEM_NO_EXPORT 
#  endif
#endif

#ifndef INNOLOWLEVELSYSTEM_DEPRECATED
#  define INNOLOWLEVELSYSTEM_DEPRECATED __declspec(deprecated)
#endif

#ifndef INNOLOWLEVELSYSTEM_DEPRECATED_EXPORT
#  define INNOLOWLEVELSYSTEM_DEPRECATED_EXPORT InnoLowLevelSystem_EXPORT INNOLOWLEVELSYSTEM_DEPRECATED
#endif

#ifndef INNOLOWLEVELSYSTEM_DEPRECATED_NO_EXPORT
#  define INNOLOWLEVELSYSTEM_DEPRECATED_NO_EXPORT INNOLOWLEVELSYSTEM_NO_EXPORT INNOLOWLEVELSYSTEM_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef INNOLOWLEVELSYSTEM_NO_DEPRECATED
#    define INNOLOWLEVELSYSTEM_NO_DEPRECATED
#  endif
#endif

#endif /* InnoLowLevelSystem_EXPORT_H */
