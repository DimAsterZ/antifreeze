#ifndef ANTIFREEZE_GLOBAL_H
#define ANTIFREEZE_GLOBAL_H

//#include <QtCore/qglobal.h>

//#if defined(ANTIFREEZE_LIBRARY)
//#  define ANTIFREEZESHARED_EXPORT Q_DECL_EXPORT
//#else
//#  define ANTIFREEZESHARED_EXPORT Q_DECL_IMPORT
//#endif

//#endif // ANTIFREEZE_GLOBAL_H


#ifdef _WIN32
    #ifdef ANTIFREEZE_LIBRARY
		#define ANTIFREEZESHARED_EXPORT __declspec(dllexport)
    #else
		#define ANTIFREEZESHARED_EXPORT __declspec(dllimport)
    #endif
#else
		#define ANTIFREEZESHARED_EXPORT __attribute__((visibility("default")))
#endif

#endif // ANTIFREEZE_GLOBAL_H
