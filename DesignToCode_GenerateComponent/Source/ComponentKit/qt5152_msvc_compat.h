#pragma once
#include <QtCore/qglobal.h>

// Qt 5.15.2 references stdext iterators removed in MSVC 14.51.
// Use the same pointer form as Qt's non-MSVC fallback, without editing the SDK.
#if defined(_MSC_VER) && _MSC_VER >= 1951 && QT_VERSION == QT_VERSION_CHECK(5, 15, 2)
#undef QT_MAKE_UNCHECKED_ARRAY_ITERATOR
#undef QT_MAKE_CHECKED_ARRAY_ITERATOR
#define QT_MAKE_UNCHECKED_ARRAY_ITERATOR(x) (x)
#define QT_MAKE_CHECKED_ARRAY_ITERATOR(x, N) (x)
#endif
