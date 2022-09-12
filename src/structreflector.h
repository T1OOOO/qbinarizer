#ifndef STRUCTREFLECTOR_H
#define STRUCTREFLECTOR_H

#include "jsonutils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QObject>

class StructReflector {
public:
  StructReflector();

  template <typename T> static QVariantList createFieldSpec() {
    return createFieldSpecStr(T::staticMetaObject);
  }

  template <typename T> static QVariantList extractValues() {
    return createFieldSpecStr(T::staticMetaObject);
  }

  template <typename T>
  static QVariantList fillValues(const QVariantMap &values) {
    return createFieldSpecStr(T::staticMetaObject);
  }

  static QList<QMetaProperty> getPropertyList(const QMetaObject *object);

  static QVariantList createFieldSpecList(const QMetaObject *object);

  static QString createFieldSpecStr(const QMetaObject *object);

  struct StructInfo {
    QVariantMap valueMap;
    int size;

    StructInfo() : size(0) {}
  };

  static StructInfo getValuesInfo(const QMetaObject &metaObject,
                                  const void *gadget, const int offset = 0);

  static int setValuesInfo(const QMetaObject &metaObject,
                           const QVariantMap &valueMap, void *gadget,
                           const int offset = 0);

  template <typename T>
  static int setValuesString(T *gadget, const QVariantMap &valueMap) {
    const QMetaObject &metaObject = T::staticMetaObject;
    const int size = setValuesInfo(metaObject, valueMap, gadget, 0);

    return size;
  }

  template <typename T> static QString getValuesString(const T *gadget) {
    const QMetaObject &metaObject = T::staticMetaObject;
    const StructInfo info = getValuesInfo(metaObject, gadget);
    QString str = QJsonDocument(QJsonObject::fromVariantMap(info.valueMap))
                      .toJson(QJsonDocument::Compact);

    return str;
  }
};

#endif // STRUCTREFLECTOR_H
