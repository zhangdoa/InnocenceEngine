
#ifndef InnoHighLevelSystem_EXPORT_H
#define InnoHighLevelSystem_EXPORT_H

#ifdef InnoHighLevelSystem_BUILT_AS_STATIC
#  define InnoHighLevelSystem_EXPORT
#  define INNOHIGHLEVELSYSTEM_NO_EXPORT
#else
#  ifndef InnoHighLevelSystem_EXPORT
#    ifdef InnoHighLevelSystem_EXPORTS
        /* We are building this library */
#      define InnoHighLevelSystem_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define InnoHighLevelSystem_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef INNOHIGHLEVELSYSTEM_NO_EXPORT
#    define INNOHIGHLEVELSYSTEM_NO_EXPORT 
#  endif
#endif

#ifndef INNOHIGHLEVELSYSTEM_DEPRECATED
#  define INNOHIGHLEVELSYSTEM_DEPRECATED __declspec(deprecated)
#endif

#ifndef INNOHIGHLEVELSYSTEM_DEPRECATED_EXPORT
#  define INNOHIGHLEVELSYSTEM_DEPRECATED_EXPORT InnoHighLevelSystem_EXPORT INNOHIGHLEVELSYSTEM_DEPRECATED
#endif

#ifndef INNOHIGHLEVELSYSTEM_DEPRECATED_NO_EXPORT
#  define INNOHIGHLEVELSYSTEM_DEPRECATED_NO_EXPORT INNOHIGHLEVELSYSTEM_NO_EXPORT INNOHIGHLEVELSYSTEM_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef INNOHIGHLEVELSYSTEM_NO_DEPRECATED
#    define INNOHIGHLEVELSYSTEM_NO_DEPRECATED
#  endif
#endif

#endif /* InnoHighLevelSystem_EXPORT_H */
