#include "internal/structencoder.h"

#include "bitutils.h"
#include "checksum.h"
#include "jsonutils.h"
#include <bitfield/bitfield.h>

#include <QDateTime>

namespace qbinarizer {

StructEncoder::StructEncoder(QObject *parent) : QObject{parent} {}

std::tuple<QByteArray, QVariantList>
StructEncoder::encode(const QString &datafieldListStr,
                      const QString &valueListStr) {
  const QVariantList datafieldList = parseJson(datafieldListStr);
  const QVariantList valueList = parseJson(valueListStr);

  return encode(datafieldList, valueList);
}

std::tuple<QByteArray, QVariantList>
StructEncoder::encode(const QVariantList &datafieldList,
                      const QVariantList &valueList) {
  clear();

  m_datafieldList = datafieldList;
  m_valueList = valueList;

  m_buf.open(QIODevice::WriteOnly);
  m_ds.setDevice(&m_buf);

  encode();

  QByteArray data = m_buf.buffer().mid(0, m_buf.pos());

  return std::make_tuple(data, m_encodeList);
}

void StructEncoder::clear() {
  m_encodedFields = QVariantMap();
  m_datafieldList = QVariantList();
  m_valueList = QVariantList();

  m_buf.close();
  m_ds.setDevice(nullptr);
}

void StructEncoder::encode() {
  m_encodeList = encodeList(m_datafieldList, m_valueList);
}

inline QVariantMap toMap(const QVariantMap &field) {
  const QString &name = field["name"].toString();

  QVariantMap res;

  if (field.contains("value")) {
    res[name] = field["value"];
  }

  return res;
}

QVariantList StructEncoder::encodeList(const QVariantList &datafieldList,
                                       const QVariantList &valueList) {
  QVariantList encodedListRes;

  for (int i = 0; i < datafieldList.size(); i++) {
    const QVariant &datafield = datafieldList.at(i);

    if (datafield.type() == QVariant::List) {
      const QVariantList subDatafieldList = datafield.toList();

      QVariantList subValueList;
      if (i < valueList.size()) {
        subValueList = m_valueList.at(i).toList();
      }
      const QVariantList encodedList =
          encodeList(subDatafieldList, subValueList);

      encodedListRes.append(encodedList);
    } else if (datafield.type() == QVariant::Map) {
      const QVariantMap field = datafield.toMap();
      const QString &fieldName = field.firstKey();

      auto it = std::find_if(valueList.cbegin(), valueList.cend(),
                             [fieldName](const QVariant &value) -> bool {
                               const QVariantMap valueMap = value.toMap();
                               const QString &valueName = valueMap.firstKey();

                               return fieldName == valueName;
                             });

      QVariantMap value;
      if (it != valueList.cend()) {
        value = it->toMap();
      }

      QVariantMap encoedMap = encodeMap(field, value);
      encodedListRes.push_back(encoedMap);
    }
  }

  return encodedListRes;
}

QVariantMap StructEncoder::encodeMap(const QVariantMap &field,
                                     const QVariantMap &fieldValue) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();
  const QString type = fieldDescription["type"].toString();

  QVariant valueData = fieldValue[fieldName];
  if (valueData.isNull()) {
    valueData = fieldDescription["value"];
  }

  qint64 newPos = -1;
  if (fieldDescription.contains("pos") &&
      fieldDescription["pos"].canConvert<qint64>()) {
    newPos = fieldDescription["pos"].toLongLong();
    m_buf.seek(newPos);
  }

  QVariantMap encodedField;
  encodedField["name"] = fieldName;
  encodedField["from"] = m_buf.pos();
  encodedField["value"] = generateEncodedValue(fieldName, valueData);
  encodedField["type"] = type;
  m_encodedFields[fieldName] = encodedField;

  if (fieldDescription.contains("count")) {
    int count = 0;

    const QVariant &countValue = fieldDescription["count"];
    if (countValue.type() == QVariant::String) {
      const QString countField = countValue.toString();
      if (m_encodedFields.contains(countField)) {
        count = getEncodedValue(countField).toInt();
      }
    } else {
      count = countValue.toUInt();
    }

    if (count > 1) {
      const QVariantList valueList = valueData.toList();
      QVariantMap subFieldDescription = fieldDescription;
      subFieldDescription["count"] = 1;

      QVariantMap subField;
      subField[fieldName] = subFieldDescription;

      for (int i = 0; i < count; i++) {
        QVariantMap subValue;

        if (i < valueList.size()) {
          subValue[fieldName] = valueList.at(i);
        }

        encodeMap(subField, subValue);
      }

      return {};
    }
  }

  QVariantMap valueEncoded;

  if (type == "array") {
    QVariantMap fieldDescriptionNew = fieldDescription;
    fieldDescriptionNew["type"] = fieldDescription["subtype"];
    fieldDescriptionNew.remove("subtype");

    QVariantMap fieldNew;
    fieldNew[fieldName] = fieldDescriptionNew;

    valueEncoded = encodeMap(fieldNew, fieldValue);
  } else if (type.startsWith("int") || type.startsWith("uint") ||
             (type == "float") || (type == "double")) {
    valueEncoded = encodeValue(field, valueData);
  } else if (type == "const") {
    valueEncoded = encodeConst(field, valueData);
  } else if (type.startsWith("crc")) {
    valueEncoded = encodeСrc(field);
  } else if (type == "struct") {
    valueEncoded = encodeStruct(field, valueData);
  } else if (type == "custom") {
    valueEncoded = encodeCustom(field, valueData);
  } else if (type == "unixtime") {
    valueEncoded = encodeUnixtime(field, valueData);
  } else if (type == "raw") {
    valueEncoded = encodeRaw(field, valueData);
  } else if (type == "skip") {
    valueEncoded = encodeSkip(fieldDescription);
  } else if (type == "bitfield") {
    valueEncoded = encodeBitfield(field, valueData);
  }

  return valueEncoded;
}

QVariantMap StructEncoder::encodeValue(const QVariantMap &field,
                                       const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString endian = fieldDescription["endian"].toString().toLower();
  const bool bigEndian = (endian == "big");
  if (bigEndian) {
    m_ds.setByteOrder(QDataStream::BigEndian);
    // res["endian"] = "big";
  } else {
    m_ds.setByteOrder(QDataStream::LittleEndian);
    // res["endian"] = "little";
  }

  const QString type = fieldDescription["type"].toString();
  if (type == "int8" || type == "char") {
    writeValue<qint8>(m_ds, valueData);
  } else if (type == "uint8") {
    writeValue<qint8>(m_ds, valueData);
  } else if (type == "int16") {
    writeValue<qint16>(m_ds, valueData);
  } else if (type == "uint16") {
    writeValue<quint16>(m_ds, valueData);
  } else if (type == "int24") {
    const qint32 val = valueData.toLongLong();

    write24<qint32>(m_ds, val, bigEndian);
  } else if (type == "uint24") {
    const quint32 val = read24<quint32>(m_ds);

    write24<quint32>(m_ds, val, bigEndian);
  } else if (type == "int32") {
    writeValue<qint32>(m_ds, valueData);
  } else if (type == "uint32") {
    writeValue<quint32>(m_ds, valueData);
  } else if (type == "int64") {
    writeValue<qint64>(m_ds, valueData);
  } else if (type == "uint64") {
    writeValue<quint64>(m_ds, valueData);
  } else if (type == "float") {
    m_ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    writeValue<float>(m_ds, valueData);
  } else if (type == "double") {
    m_ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    writeValue<double>(m_ds, valueData);
  }
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = valueData.isNull() ? 0 : valueData;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeBitfield(const QVariantMap &field,
                                          const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  quint32 size = fieldDescription["size"].toUInt();
  size = (size <= 0) ? 1 : size;

  QByteArray data(size, static_cast<char>(0));

  if (!fieldDescription.contains("spec") ||
      (fieldDescription["spec"].type() != QVariant::Map)) {
    return {};
  }

  const QVariantMap spec = fieldDescription["spec"].toMap();
  if (spec.isEmpty()) {
    return {};
  }

  const QVariantMap valueMap = valueData.toMap();
  const QStringList specNameList = spec.keys();
  for (int i = 0; i < specNameList.size(); i++) {
    const QString &specName = specNameList.at(i);
    const QVariantMap specField = spec[specName].toMap();

    QVariantMap specFieldNew;
    specFieldNew[specName] = specField;

    QVariant bitfieldValue;
    if (valueMap.contains(specName)) {
      bitfieldValue = valueMap[specName];
    }

    encodeBitfieldElement(specFieldNew, bitfieldValue, data);
  }

  if (fieldDescription.contains("reversed") &&
      fieldDescription["reversed"].toBool()) {
    std::reverse(data.begin(), data.end());
    std::transform(data.begin(), data.end(), data.begin(),
                   [](const auto value) -> bool {
                     return reverseChar((uint8_t)(value & 0xff));
                   });
  }

  m_ds.writeRawData(data.constBegin(), data.size());
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = valueData;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeBitfieldElement(const QVariantMap &field,
                                                 const QVariant &value,
                                                 QByteArray &data) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  QVariant valueData = value;
  if (value.isNull()) {
    valueData = fieldDescription["value"];
  }

  int size = fieldDescription["size"].toUInt();
  size = (size == 0) ? 1 : size;

  const int pos = fieldDescription["pos"].toUInt();
  const int last = pos + size - 1;
  if (last > (data.size() * CHAR_WIDTH)) {
    return {};
  }

  quint64 valueU = 0;
  if (fieldDescription.contains("signed") &&
      fieldDescription["signed"].toBool()) {
    const qint64 valueS = valueData.toLongLong();
    valueU = valueS & bitmask(size);
  } else {
    valueU = valueData.toULongLong();
  }

  set_bitfield(valueU, pos, size, reinterpret_cast<uint8_t *>(data.data()),
               data.size());

  QVariantMap encodedField;
  encodedField["name"] = fieldName;
  encodedField["value"] = valueData;
  encodedField["from"] = pos;
  encodedField["to"] = last;
  m_encodedFields[fieldName] = encodedField;

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = valueData;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeСrc(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString type = fieldDescription["type"].toString();
  const QString mode = type.mid(3);
  const int crcSize = mode.toInt() / CHAR_WIDTH;
  int to = m_buf.pos() - 1;
  if (to < 0) {
    return {};
  }

  if (fieldDescription.contains("include") &&
      fieldDescription["include"].toBool()) {
    to += crcSize;

    QByteArray data(crcSize, static_cast<char>(0));
    m_ds.writeRawData(data.constData(), data.size());
  }

  if (fieldDescription.contains("to") &&
      (fieldDescription["to"].type() == QVariant::String)) {
    const QString toField = fieldDescription["to"].toString();

    to = getEncodedValue(toField).toInt();
  }

  int from = fieldDescription["from"].toInt();
  if (fieldDescription.contains("parent")) {
    const QString parentName = fieldDescription["parent"].toString();
    if (m_encodedFields.contains(parentName)) {
      const QVariantMap parentField = m_encodedFields[parentName].toMap();

      from = parentField["from"].toInt();
    }
  }

  if (from > to) {
    return {};
  }

  const QByteArray &data = m_buf.buffer();
  if (to >= data.size()) {
    return {};
  }

  const int dataSize = to - from + 1;
  const unsigned char *fromC =
      reinterpret_cast<const unsigned char *>(&data.constData()[from]);

  if (fieldDescription.contains("include") &&
      fieldDescription["include"].toBool()) {
    const int pos = m_buf.pos();

    m_buf.seek(pos - crcSize);
  }

  quint64 crc = 0;
  if (mode == "8") {
    const quint8 crc8 = crc_8(fromC, dataSize);
    m_ds << crc8;
    crc = crc8;
  } else if (mode == "16") {
    const quint16 crc16 = crc_16(fromC, dataSize);
    m_ds << crc16;
    crc = crc16;
  } else if (mode == "32") {
    const quint32 crc32 = crc_32(fromC, dataSize);
    m_ds << crc32;
    crc = crc32;
  } else if (mode == "64") {
    const quint64 crc64 = crc_64_we(fromC, dataSize);
    m_ds << crc64;
    crc = crc64;
  }

  QVariantMap encodedField = m_encodedFields[fieldName].toMap();
  encodedField["to"] = m_buf.pos();
  encodedField["value"] = crc;
  m_encodedFields[fieldName] = encodedField;

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = crc;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeRaw(const QVariantMap &field,
                                     const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const int size = fieldDescription["size"].toUInt();
  if (size == 0) {
    return {};
  }

  const QByteArray data = QByteArray::fromHex(valueData.toString().toLatin1());
  m_ds.writeRawData(data.data(), data.size());
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = data.toHex();

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeCustom(const QVariantMap &field,
                                        const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  if (!fieldDescription.contains("depend") ||
      (fieldDescription["depend"].type() != QVariant::String)) {
    return {};
  }

  const QString dependFieldName = fieldDescription["depend"].toString();
  if (!m_encodedFields.contains(dependFieldName)) {
    return {};
  }

  const QVariantMap dependField = m_encodedFields[dependFieldName].toMap();
  const QVariant &dependValue = dependField["value"];
  if (dependValue.isNull()) {
    return {};
  }

  if (!fieldDescription.contains("choose") ||
      (fieldDescription["choose"].type() != QVariant::Map)) {
    return {};
  }
  const QVariantMap choose = fieldDescription["choose"].toMap();
  if (choose.isEmpty()) {
    return {};
  }

  const auto it = std::find_if(
      choose.constBegin(), choose.constEnd(),
      [&dependValue](const auto &it) -> bool { return dependValue == it; });
  if (it == choose.constEnd()) {
    return {};
  }

  const QString &chooseFieldName = it.key();
  QVariantMap spec = fieldDescription["spec"].toMap();
  if (!spec.contains(chooseFieldName)) {
    return {};
  }

  QVariantMap specValue = spec[chooseFieldName].toMap();
  specValue["parent"] = fieldName;

  m_encodedFields[chooseFieldName] = specValue;

  QVariantMap encodedField;
  encodedField[chooseFieldName] = specValue;
  encodeMap(encodedField, valueData.toMap());
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = valueData;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeStruct(const QVariantMap &field,
                                        const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QVariantMap spec = fieldDescription["spec"].toMap();
  const QString &specName = spec.firstKey();
  QVariantMap specValue = spec[specName].toMap();
  specValue["parent"] = fieldName;

  m_encodedFields[specName] = specValue;

  QVariantMap encodedField;
  encodedField[specName] = specValue;
  encodeMap(encodedField, valueData.toMap());
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = valueData;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeSkip(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const int size = fieldDescription["size"].toUInt();
  if (size <= 0) {
    return {};
  }

  m_ds.skipRawData(size);
  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = {};

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeUnixtime(const QVariantMap &field,
                                          const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString dateTimeStr = valueData.toString();
  const auto dateTime = QDateTime::fromString(dateTimeStr, Qt::ISODateWithMs);

  const qint64 unixtime = dateTime.toMSecsSinceEpoch();

  const QString endian = fieldDescription["endian"].toString().toLower();
  const bool bigEndian = (endian == "big");
  if (bigEndian) {
    m_ds.setByteOrder(QDataStream::BigEndian);
  } else {
    m_ds.setByteOrder(QDataStream::LittleEndian);
  }
  m_ds << unixtime;

  updateEncodedTo(fieldName);

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = dateTimeStr;

  return toMap(valueRes);
}

QVariantMap StructEncoder::encodeConst(const QVariantMap &field,
                                       const QVariant &valueData) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  if (!fieldDescription.contains("value") ||
      (fieldDescription["value"].type() != QVariant::String)) {
    return {};
  }

  if (!fieldDescription.contains("size") ||
      (fieldDescription["size"].toUInt() == 0)) {
    return {};
  }

  const QString constDataStr = fieldDescription["value"].toString();
  if (constDataStr.isEmpty()) {
    return {};
  }

  QByteArray constData = QByteArray::fromHex(constDataStr.toLatin1());
  if (constData.isEmpty()) {
    return {};
  }

  const quint32 size = fieldDescription["size"].toUInt();
  if (size > constData.size()) {
    return {};
  }
  constData = constData.mid(0, size);
  m_ds.writeRawData(constData.data(), constData.size());

  QVariantMap valueRes;
  valueRes["name"] = fieldName;
  valueRes["value"] = constDataStr;

  return toMap(valueRes);
}

void StructEncoder::updateEncodedTo(const QString &name) {
  if (!m_encodedFields.contains(name)) {
    return;
  }

  QVariantMap encodedField = m_encodedFields[name].toMap();
  encodedField["to"] = m_buf.pos();
  m_encodedFields[name] = encodedField;
}

QVariant StructEncoder::getEncodedValue(const QString &name) const {
  if (!m_encodedFields.contains(name)) {
    return {};
  }

  const QVariantMap encodedField = m_encodedFields[name].toMap();
  QVariant value = encodedField["value"];

  return value;
}

QVariant StructEncoder::generateEncodedValue(const QString &name,
                                             const QVariant &value) {
  if (!m_encodedFields.contains(name) || (value.type() == QVariant::List)) {
    return value;
  }

  QVariant encodedValue = getEncodedValue(name);
  if (encodedValue.type() == QVariant::List) {
    QVariantList encodedValueList = encodedValue.toList();
    encodedValueList.push_back(value);

    return encodedValueList;
  }

  QVariantList encodedValueList = {encodedValue, value};

  return encodedValueList;
}

} // namespace qbinarizer
