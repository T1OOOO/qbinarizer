#ifndef MYSTRUCT_H
#define MYSTRUCT_H

#include <QObject>

struct MyStructChild {
  Q_GADGET

  Q_PROPERTY(quint8 b MEMBER b)

public:
  quint8 b;

  MyStructChild() : b(0) {}
};
Q_DECLARE_METATYPE(MyStructChild)

struct MyStructParent {
  Q_GADGET

  Q_PROPERTY(quint16 a MEMBER a)
  Q_PROPERTY(MyStructChild child MEMBER child)

public:
  quint16 a;
  MyStructChild child;

  MyStructParent() : a(0){};
};
Q_DECLARE_METATYPE(MyStructParent)

inline bool operator!=(const MyStructChild &child1,
                       const MyStructChild &child2) {
  return child1.b != child2.b;
}

#endif // MYSTRUCT_H
