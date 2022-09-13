#include "internal/structreflector.h"
#include "jsonutils.h"

namespace qbinarizer {

StructReflector::StructReflector() {}

QVariantList StructReflector::createFieldSpecList(const QMetaObject *object) {
  const QString fieldSpecStr = createFieldSpecStr(object);
  const QString fieldSpecListStr = QString("[%1]").arg(fieldSpecStr);
  QVariantList fieldList = parseJson(fieldSpecListStr);

  return fieldList;
}

QList<QMetaProperty>
StructReflector::getPropertyList(const QMetaObject *object) {
  QList<QMetaProperty> propertyList;

  for (int i = 0; i < object->propertyCount(); i++) {
    const QMetaProperty property = object->property(i);

    propertyList.push_back(property);
  }

  return propertyList;
}

QString StructReflector::createFieldSpecStr(const QMetaObject *object) {
  const QList<QMetaProperty> propertyList = getPropertyList(object);

  QStringList fieldSpecList;
  for (const auto &property : propertyList) {
    const QString name(property.name());
    const QVariant::Type type = property.type();
    const QMetaType::Type metaType = static_cast<QMetaType::Type>(type);

    QString valueType;
    switch (metaType) {
    case QMetaType::Bool:
      valueType = QStringLiteral("uint8");
      break;
    case QMetaType::Int:
      valueType = QStringLiteral("int32");
      break;
    case QMetaType::UInt:
      valueType = QStringLiteral("uint32");
      break;
    case QMetaType::LongLong:
      valueType = QStringLiteral("int64");
      break;
    case QMetaType::ULongLong:
      valueType = QStringLiteral("uint64");
      break;
    case QMetaType::Double:
      valueType = QStringLiteral("double");
      break;
    case QMetaType::Long:
      valueType = QStringLiteral("int64");
      break;
    case QMetaType::Short:
      valueType = QStringLiteral("int16");
      break;
    case QMetaType::Char:
      valueType = QStringLiteral("int8");
      break;
    case QMetaType::ULong:
      valueType = QStringLiteral("uint64");
      break;
    case QMetaType::UShort:
      valueType = QStringLiteral("uint16");
      break;
    case QMetaType::UChar:
      valueType = QStringLiteral("uint8");
      break;
    case QMetaType::Float:
      valueType = QStringLiteral("float");
      break;
    case QMetaType::SChar:
      valueType = QStringLiteral("uint8");
      break;
    case QMetaType::QDateTime:
      valueType = QStringLiteral("unixtime");
      break;
    case QMetaType::QByteArray:
      valueType = QStringLiteral("raw");
      break;
    default:
      valueType = QStringLiteral("struct");
      break;
    }

    QString fieldSpecStr;
    if (valueType == "struct") {
      const QMetaObject *subMeta = property.enclosingMetaObject();
      if (subMeta == nullptr) {
        continue;
      }

      static const QString fieldSpecTemplate =
          R"({"%1": {"type": "%2", "spec": [%3]}})";
      const QString subFieldStr = createFieldSpecStr(subMeta);
      fieldSpecStr =
          fieldSpecTemplate.arg(name).arg(valueType).arg(subFieldStr);
    } else if (!valueType.isEmpty()) {
      static const QString fieldSpecTemplate = R"({"%1": {"type": "%2"}})";
      fieldSpecStr = fieldSpecTemplate.arg(name).arg(valueType);
    } else {
      continue;
    }

    fieldSpecList.push_back(fieldSpecStr);
  }

  QString fieldSpecAll = fieldSpecList.join(",");

  return fieldSpecAll;
}

int StructReflector::setValuesInfo(const QMetaObject &metaObject,
                                   const QVariantMap &valueMap, void *gadget,
                                   const int offset) {
  int size = offset;
  QList<QMetaProperty> propertyList = getPropertyList(&metaObject);
  for (const auto &property : propertyList) {
    const QString name(property.name());
    const QVariant::Type type = property.type();
    const QMetaType::Type metaType = static_cast<QMetaType::Type>(type);

    QVariant value;
    if (valueMap.contains(name)) {
      value = valueMap[name];
    }

    QVariant valueNew;
    switch (metaType) {
    case QMetaType::Bool:
      size += 1;
      break;
    case QMetaType::Char:
      size += 1;
      break;
    case QMetaType::UChar:
      size += 1;
      break;
    case QMetaType::SChar:
      size += 1;
      break;
    case QMetaType::Short:
      size += 2;
      break;
    case QMetaType::UShort:
      size += 2;
      break;
    case QMetaType::Int:
      size += 4;
      break;
    case QMetaType::UInt:
      size += 4;
      break;
    case QMetaType::Long:
      size += 4;
      break;
    case QMetaType::ULong:
      size += 4;
      break;
    case QMetaType::LongLong:
      size += 8;
      break;
    case QMetaType::ULongLong:
      size += 8;
      break;
    case QMetaType::Float:
      size += 4;
      break;
    case QMetaType::Double:
      size += 8;
      break;
    default: {
      const int type = QMetaType::type(property.typeName());
      const QMetaObject *subMeta = QMetaType::metaObjectForType(type);
      if (subMeta == nullptr) {
        continue;
      }

      const int offset = size;
      char *gadgetData = reinterpret_cast<char *>(gadget);
      char *propertyData = &gadgetData[offset];
      const int subSize =
          setValuesInfo(*subMeta, value.toMap(), propertyData, size);
      size += subSize;
    } break;
    }

    if (!value.isNull()) {
      property.writeOnGadget(gadget, value);
    }
  }

  return size;
}

StructReflector::StructInfo
StructReflector::getValuesInfo(const QMetaObject &metaObject,
                               const void *gadget, const int offset) {
  QVariantMap valueMap;

  int size = offset;
  QList<QMetaProperty> propertyList = getPropertyList(&metaObject);
  for (const auto &property : propertyList) {
    const QString name(property.name());
    const QVariant::Type type = property.type();
    const QMetaType::Type metaType = static_cast<QMetaType::Type>(type);

    QVariant value = property.readOnGadget(gadget);

    QVariant valueNew;
    switch (metaType) {
    case QMetaType::Bool:
      size += 1;
      valueNew = (int)value.toBool();
      break;
    case QMetaType::Char:
      size += 1;
      valueNew = value.toChar();
      break;
    case QMetaType::UChar:
      size += 1;
      valueNew = value.toUInt();
      break;
    case QMetaType::SChar:
      size += 1;
      valueNew = value.toInt();
      break;
    case QMetaType::Short:
      size += 2;
      valueNew = value.toInt();
      break;
    case QMetaType::UShort:
      size += 2;
      valueNew = value.toUInt();
      break;
    case QMetaType::Int:
      size += 4;
      valueNew = value.toInt();
      break;
    case QMetaType::UInt:
      size += 4;
      valueNew = value.toUInt();
      break;
    case QMetaType::Long:
      size += 4;
      valueNew = value.toLongLong();
      break;
    case QMetaType::ULong:
      size += 4;
      valueNew = value.toULongLong();
      break;
    case QMetaType::LongLong:
      size += 8;
      valueNew = value.toLongLong();
      break;
    case QMetaType::ULongLong:
      size += 8;
      valueNew = value.toULongLong();
      break;
    case QMetaType::Float:
      size += 4;
      valueNew = value.toFloat();
      break;
    case QMetaType::Double:
      size += 8;
      valueNew = value.toDouble();
      break;
    default: {
      const int type = QMetaType::type(property.typeName());
      const QMetaObject *subMeta = QMetaType::metaObjectForType(type);
      if (subMeta == nullptr) {
        continue;
      }

      const int offset = size;
      const char *gadgetData = reinterpret_cast<const char *>(gadget);
      const char *propertyData = &gadgetData[offset];

      const StructInfo info = getValuesInfo(*subMeta, propertyData);
      size += info.size;
      valueNew = info.valueMap;
    } break;
    }

    valueMap[name] = valueNew;
  }

  StructInfo info;
  info.size = size;
  info.valueMap = valueMap;

  return info;
}

} // namespace qbinarizer
