#include "internal/structdecoder.h"

#include "bitfield/bitfield.h"
#include "bitutils.h"
#include "checksum.h"
#include "jsonutils.h"

#include <QDateTime>

namespace qbinarizer {

StructDecoder::StructDecoder(QObject *parent) : QObject{parent} {}

QVariantList StructDecoder::decode(const QString &datafieldListStr,
                                   const QByteArray &data) {
  const QVariantList datafieldList = parseJson(datafieldListStr);
  QVariantList valueList = decode(datafieldList, data);

  return valueList;
}

QVariantList StructDecoder::decode(const QVariantList &datafieldList,
                                   const QByteArray &data) {
  clear();

  if (datafieldList.isEmpty()) {
    return {};
  }

  m_datafieldList = datafieldList;
  m_data = data;

  m_buf.setData(m_data);
  m_buf.open(QIODevice::ReadOnly);
  m_ds.setDevice(&m_buf);

  decode();

  return m_resList;
}

void StructDecoder::clear() {
  m_name = QString();
  m_datafieldList = QVariantList();
  m_pos = 0;

  m_buf.close();
  m_ds.setDevice(nullptr);
  m_data = QByteArray();
  m_resList = QVariantList();
  m_decodedFields = QVariantMap();
}

void StructDecoder::decode() { m_resList = decodeList(m_datafieldList); }

inline QVariantMap toMap(const QVariantMap &field) {
  const QString &name = field["name"].toString();

  QVariantMap res;

  if (field.contains("value")) {
    res[name] = field["value"];
  }

  return res;
}

QVariantList StructDecoder::decodeList(const QVariantList &fieldList) {
  QVariantList resList;

  for (int i = 0; i < fieldList.size(); i++) {
    const QVariant &field = fieldList.at(i);
    if (field.type() == QVariant::List) {
      const QVariantList subList = field.toList();
      QVariantList subResList = decodeList(subList);

      resList.append(subResList);
    } else if (field.type() == QVariant::Map) {
      const QVariantMap fieldMap = field.toMap();

      QVariantMap res = decodeMap(fieldMap);
      if (res.isEmpty()) {
        continue;
      }

      resList.push_back(res);
    }
  }

  return resList;
}

QVariantMap StructDecoder::decodeMap(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();
  const QString type = fieldDescription["type"].toString();

  // const qint64 oldPos = m_buf.pos();
  qint64 newPos = -1;
  if (fieldDescription.contains("pos") &&
      fieldDescription["pos"].canConvert<qint64>()) {
    newPos = fieldDescription["pos"].toLongLong();
    m_buf.seek(newPos);
  }

  QVariantMap decodedField;
  decodedField["name"] = fieldName;
  decodedField["from"] = m_buf.pos();
  decodedField["type"] = type;
  m_decodedFields[fieldName] = decodedField;

  if (fieldDescription.contains("count")) {
    int count = 0;

    const QVariant &countValue = fieldDescription["count"];
    if (countValue.type() == QVariant::String) {
      const QString countField = countValue.toString();
      if (!m_decodedFields.contains(countField)) {
        return {};
      }

      count = getDecodedValue(countField).toInt();
    } else {
      count = countValue.toInt();
    }

    if (count > 1) {
      QVariantMap subFieldDescription = fieldDescription;
      subFieldDescription["count"] = 1;
      subFieldDescription.remove("pos");

      QVariantMap subField;
      subField[fieldName] = subFieldDescription;

      QVariantList subResList;
      for (int i = 0; i < count; i++) {
        const QVariantMap subRes = decodeMap(subField);
        if (subRes.isEmpty()) {
          continue;
        }

        const QVariant &value = subRes.first();
        subResList.push_back(value);
      }

      QVariantMap res;
      res["name"] = fieldName;
      res["value"] = subResList;

      //      if (newPos >= 0) {
      //        m_buf.seek(oldPos);
      //      }

      return toMap(res);
    }
  }

  QVariantMap valueRes;

  if (type == "array") {
    QVariantMap fieldDescriptionNew = fieldDescription;
    fieldDescriptionNew["type"] = fieldDescription["subtype"];
    fieldDescriptionNew.remove("subtype");
    fieldDescriptionNew.remove("pos");

    QVariantMap fieldNew;
    fieldNew[fieldName] = fieldDescriptionNew;

    valueRes = decodeMap(fieldNew);
  } else if (type.startsWith("int") || type.startsWith("uint") ||
             (type == "float") || (type == "double")) {
    valueRes = decodeValue(field);
  } else if (type == "const") {
    valueRes = decodeConst(field);
  } else if (type.startsWith("crc")) {
    valueRes = decodeCrc(field);
  } else if (type == "struct") {
    valueRes = decodeStruct(field);
  } else if (type == "custom") {
    valueRes = decodeCustom(field);
  } else if (type == "unixtime") {
    valueRes = decodeUnixtime(field);
  } else if (type == "raw") {
    valueRes = decodeRaw(field);
  } else if (type == "skip") {
    valueRes = decodeSkip(field);
  } else if (type == "bitfield") {
    valueRes = decodeBitfield(field);
  }

  //  if (newPos >= 0) {
  //    m_buf.seek(oldPos);
  //  }

  return valueRes;
}

QVariantMap StructDecoder::decodeValue(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString endian = fieldDescription["endian"].toString().toLower();
  const bool bigEndian = (endian == "big");
  if (bigEndian) {
    m_ds.setByteOrder(QDataStream::BigEndian);
  } else {
    m_ds.setByteOrder(QDataStream::LittleEndian);
  }

  QVariant value;

  // Value
  const QString type = fieldDescription["type"].toString();
  if (type == "int8" || type == "char") {
    readValue<qint8>(m_ds, value);
  } else if (type == "uint8") {
    readValue<qint8>(m_ds, value);
  } else if (type == "int16") {
    readValue<qint16>(m_ds, value);
  } else if (type == "uint16") {
    readValue<quint16>(m_ds, value);
  } else if (type == "int24") {
    qint32 val = read24<qint32>(m_ds);
    if (bigEndian) {
      val = reverse24<qint32>(val);
    }
    val = fixSign24(val);
    value = val;
  } else if (type == "uint24") {
    quint32 val = read24<quint32>(m_ds);
    if (bigEndian) {
      val = reverse24<quint32>(val);
    }
    value = val;
  } else if (type == "int32") {
    readValue<qint32>(m_ds, value);
  } else if (type == "uint32") {
    readValue<quint32>(m_ds, value);
  } else if (type == "int64") {
    readValue<qint64>(m_ds, value);
  } else if (type == "uint64") {
    readValue<quint64>(m_ds, value);
  } else if (type == "float") {
    m_ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    readValue<float>(m_ds, value);
  } else if (type == "double") {
    m_ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    readValue<double>(m_ds, value);
  }
  setDecodedValue(fieldName, value);

  QVariantMap res;
  res["name"] = fieldName;
  res["value"] = value;

  return toMap(res);
}

QVariantMap StructDecoder::decodeBitfield(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  if (!fieldDescription.contains("size")) {
    return {};
  }

  quint32 size = fieldDescription["size"].toUInt();
  size = (size <= 0) ? 1 : size;

  QByteArray data(size, static_cast<char>(0));
  m_ds.readRawData(data.data(), data.size());

  if (fieldDescription.contains("reversed") &&
      fieldDescription["reversed"].toBool()) {
    std::reverse(data.begin(), data.end());
    std::transform(data.begin(), data.end(), data.begin(),
                   [](const auto value) -> bool {
                     return reverseChar((uint8_t)(value & 0xff));
                   });
  }

  if (!fieldDescription.contains("spec") ||
      (fieldDescription["spec"].type() != QVariant::Map)) {
    return {};
  }

  const QVariantMap spec = fieldDescription["spec"].toMap();
  if (spec.isEmpty()) {
    return {};
  }

  QVariantMap resMap;
  const QStringList specNameList = spec.keys();
  for (const auto &specName : specNameList) {
    const QVariantMap specField = spec[specName].toMap();

    QVariantMap specFieldNew;
    specFieldNew[specName] = specField;
    const QVariant specRes = decodeBitfieldElement(specFieldNew, data);

    resMap[specName] = specRes;
  }

  QVariantMap res;
  res["name"] = fieldName;
  res["value"] = resMap;
  setDecodedValue(fieldName, resMap);

  return toMap(res);
}

QVariant StructDecoder::decodeBitfieldElement(const QVariantMap &field,
                                              const QByteArray &data) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  int size = fieldDescription["size"].toInt();
  size = (size == 0) ? 1 : size;

  const int pos = fieldDescription["pos"].toInt();
  const int last = pos + size - 1;
  if (last > (data.size() * CHAR_WIDTH)) {
    return {};
  }

  quint64 valueU = get_bitfield(reinterpret_cast<const uint8_t *>(data.data()),
                                size, pos, size) &
                   bitmask(size);

  if (fieldDescription.contains("signed") &&
      fieldDescription["signed"].toBool()) {
    if ((valueU & ((quint64)1 << (size - 1))) > 0) {
      qint64 valueS = (qint64)(-1) & ~bitmask(size) | (qint64)valueU;

      return valueS;
    }
  }

  return valueU;
}

QVariantMap StructDecoder::decodeCustom(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  if (!fieldDescription.contains("depend") ||
      (fieldDescription["depend"].type() != QVariant::String)) {
    return {};
  }

  const QString dependFieldName = fieldDescription["depend"].toString();
  if (!m_decodedFields.contains(dependFieldName)) {
    return {};
  }

  const QVariantMap dependField = m_decodedFields[dependFieldName].toMap();
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
  const QVariantMap spec = fieldDescription["spec"].toMap();
  if (!spec.contains(chooseFieldName)) {
    return {};
  }

  QVariantMap decodedField;
  decodedField[chooseFieldName] = spec[chooseFieldName];

  QVariantMap res;
  res[fieldName] = decodeMap(decodedField);

  return res;
}

QVariantMap StructDecoder::decodeStruct(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();
  const QVariantMap spec = fieldDescription["spec"].toMap();

  QVariantMap res;
  res[fieldName] = decodeMap(spec);

  return res;
}

QVariantMap StructDecoder::decodeRaw(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  int size = fieldDescription["size"].toUInt();
  size = (size == 0) ? 1 : size;

  QByteArray data(size, static_cast<char>(0));
  m_ds.readRawData(data.data(), data.size());

  QVariantMap res;
  res["name"] = fieldName;
  res["value"] = data.toHex();
  setDecodedValue(fieldName, data.toHex());

  return toMap(res);
}

QVariantMap StructDecoder::decodeSkip(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const int size = fieldDescription["size"].toUInt();
  if (size <= 0) {
    return {};
  }

  m_ds.skipRawData(size);

  QVariantMap res;
  res["name"] = fieldName;
  res["value"] = {};
  setDecodedValue(fieldName, {});

  return toMap(res);
}

QVariantMap StructDecoder::decodeCrc(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString type = fieldDescription["type"].toString();
  const QString mode = type.mid(3);

  int to = m_buf.pos() - 1;
  if (fieldDescription.contains("include") &&
      fieldDescription["include"].toBool()) {
    const int size = mode.toInt() / CHAR_WIDTH;

    to += size;
  }

  if (fieldDescription.contains("to") &&
      (fieldDescription["to"].type() == QVariant::String)) {
    const QString toFieldName = fieldDescription["to"].toString();

    to = getDecodedValue(toFieldName).toInt();
  }

  int from = fieldDescription["from"].toInt();
  if (fieldDescription.contains("parent")) {
    const QString parentName = fieldDescription["parent"].toString();
    if (m_decodedFields.contains(parentName)) {
      const QVariantMap parentField = m_decodedFields[parentName].toMap();

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

  const int size = to - from;
  const unsigned char *fromC =
      reinterpret_cast<const unsigned char *>(&data.constData()[from]);

  quint64 crc = 0;
  bool check = false;
  if (mode == "8") {
    crc = crc_8(fromC, size);
    quint8 crc8Read = 0;
    m_ds >> crc8Read;

    check = (crc == crc8Read);
  } else if (mode == "16") {
    crc = crc_16(fromC, size);
    quint8 crc16Read = 0;
    m_ds >> crc16Read;

    check = (crc == crc16Read);
  } else if (mode == "32") {
    crc = crc_32(fromC, size);
    quint32 crc32Read = 0;
    m_ds >> crc32Read;

    check = (crc == crc32Read);
  } else if (mode == "64") {
    crc = crc_64_we(fromC, size);
    quint64 crc64Read = 0;
    m_ds >> crc64Read;

    check = (crc == crc64Read);
  }
  setDecodedValue(fieldName, crc);

  if (field.contains("include") && field["include"].toBool()) {
    return {}; //(crc == 0)
  }

  return {};
}

QVariantMap StructDecoder::decodeUnixtime(const QVariantMap &field) {
  const QString &fieldName = field.firstKey();
  const QVariantMap fieldDescription = field[fieldName].toMap();

  const QString endian = fieldDescription["endian"].toString().toLower();
  const bool bigEndian = (endian == "big");
  if (bigEndian) {
    m_ds.setByteOrder(QDataStream::BigEndian);
  } else {
    m_ds.setByteOrder(QDataStream::LittleEndian);
  }

  qint64 unixtime = 0;
  m_ds >> unixtime;
  const auto dateTime = QDateTime::fromMSecsSinceEpoch(unixtime);
  const QString dateTimeStr = dateTime.toString(Qt::ISODateWithMs);

  setDecodedValue(fieldName, dateTimeStr);

  QVariantMap res;
  res["name"] = fieldName;
  res["value"] = dateTimeStr;

  return toMap(res);
}

QVariantMap StructDecoder::decodeConst(const QVariantMap &field) {
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

  QByteArray readData(size, static_cast<char>(0));
  m_ds.readRawData(readData.data(), readData.size());

  if (constData != readData) {
    QVariantMap res;
    res["name"] = fieldName;
    res["value"] = false;

    return toMap(res);
  }

  return {};
}

void StructDecoder::updateEncodedTo(const QString &name) {
  if (!m_decodedFields.contains(name)) {
    return;
  }

  QVariantMap decodedField = m_decodedFields[name].toMap();
  decodedField["to"] = m_buf.pos();
  m_decodedFields[name] = decodedField;
}

QVariant StructDecoder::getDecodedValue(const QString &name) const {
  if (!m_decodedFields.contains(name)) {
    return {};
  }

  const QVariantMap decodedField = m_decodedFields[name].toMap();
  QVariant decodedValue = decodedField["value"];

  return decodedValue;
}

QVariant StructDecoder::generateDecodedValue(const QString &name,
                                             const QVariant &value) {
  if (!m_decodedFields.contains(name) || (value.type() == QVariant::List)) {
    return value;
  }

  QVariant decodedValue = getDecodedValue(name);
  if (decodedValue.type() == QVariant::List) {
    QVariantList decodedValueList = decodedValue.toList();
    decodedValueList.push_back(value);

    return decodedValueList;
  }

  QVariantList decodedValueList = {m_decodedFields, value};

  return decodedValueList;
}

void StructDecoder::setDecodedValue(const QString &name,
                                    const QVariant &value) {
  if (!m_decodedFields.contains(name)) {
    return;
  }

  QVariantMap decodedField = m_decodedFields[name].toMap();

  QVariant decodedValue = generateDecodedValue(name, value);
  decodedField["value"] = value;

  m_decodedFields[name] = decodedField;
}

QVariantList StructDecoder::extractValues(const QVariant &value) {
  QVariantList valueList;

  if (value.type() == QVariant::Map) {
    const QVariantMap subValueMap = value.toMap();
    const QStringList keyList = subValueMap.keys();
    for (const auto &key : keyList) {
      const QVariant &subValue = subValueMap[key];
      if (subValue.type() != QVariant::Map &&
          subValue.type() != QVariant::List) {
        QVariantMap resValue;
        resValue[key] = subValue;
        valueList.append(resValue);
      } else {
        const QVariantList subValueList = extractValues(subValue);

        valueList.append(subValueList);
      }
    }
  } else if (value.type() == QVariant::List) {
    const QVariantList subValueList = value.toList();
    for (const auto &subValue : subValueList) {
      const QVariantList subValueList = extractValues(subValue);

      valueList.append(subValueList);
    }
  } else {
    valueList.push_back(value);
  }

  return valueList;
}

} // namespace qbinarizer
