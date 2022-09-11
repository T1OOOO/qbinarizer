#include "binarizertest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtDebug>
#include <QtMath>

struct CheckStruct {
  QString fieldStr;
  QString valueStr;
  QString dataHex;

  CheckStruct(QString fieldStr, QString valueStr, QString dataHex)
      : fieldStr(fieldStr), valueStr(valueStr), dataHex(dataHex) {}
};

bool IsEqualDouble(double dX, double dY) {
  static const double dEpsilon = 0.0001; // or some other small number
  return fabs(dX - dY) <= dEpsilon * fabs(dX);
}

bool compareVariants(const QVariant &v1, const QVariant &v2) {
  if (v1.type() != v1.type()) {
    return false;
  }

  if (v1.type() == QVariant::List) {
    const QVariantList v1List = v1.toList();
    const QVariantList v2List = v2.toList();

    if (v1List.size() != v2List.size()) {
      return false;
    }

    for (int i = 0; i < v1List.size(); i++) {
      const QVariant &v1Value = v1List.at(i);
      const QVariant &v2Value = v2List.at(i);
      if (!compareVariants(v1Value, v2Value)) {
        return false;
      }
    }
  } else if (v1.type() == QVariant::Map) {
    const QVariantMap v1Map = v1.toMap();
    const QVariantMap v2Map = v2.toMap();

    const QStringList v1Keys = v1Map.keys();
    const QStringList v2Keys = v2Map.keys();
    if (v1Keys.size() != v2Keys.size()) {
      return false;
    }

    for (const QString &v1Key : v1Keys) {
      if (!v2Map.contains(v1Key)) {
        return false;
      }

      const QVariant &v1Value = v1Map[v1Key];
      const QVariant &v2Value = v2Map[v1Key];
      if (!compareVariants(v1Value, v2Value)) {
        return false;
      }
    }
  } else if (v1.type() == QVariant::Double) {
    const double v1Value = v1.toDouble();
    const double v2Value = v2.toDouble();

    bool res = IsEqualDouble(v1Value, v2Value);

    return res;
  }

  return v1 == v2;
}

namespace {
const QVector<CheckStruct> checkList = {
    {R"([{"a": {"type": "int32"}}])", R"([{"a": 1}])", "01000000"},
    {R"([{"a": {"type": "int32"}}, {"b": {"type": "int32"}}])",
     R"([{"a": 1}, {"b": 2}])", "0100000002000000"},
    {R"([{"a": {"type": "int32", "count": 2}}])", R"([{"a": [1, 1]}])",
     "0100000001000000"},
    {R"([{"a": {"type": "array", "subtype": "int32", "count": 2}}])",
     R"([{"a": [1, 1]}])", "0100000001000000"},
    {R"([{"l": {"type": "int8"}}, {"a": {"type": "int32", "count":
        "l"}}])",
     R"([{"l": 2}, {"a": [1, 1]}])", "020100000001000000"},
    {R"([{"l": {"type": "int8"}}, {"a": {"type": "array", "subtype":
        "int32", "count": "l"}}])",
     R"([{"l": 2}, {"a": [1, 1]}])", "020100000001000000"},
    {R"([{"a": {"type": "int32"}}, {"crc": {"type": "crc16"}}])",
     R"([{"a": 1}])", "0100000001FC"},
    {R"([{"l": {"type": "int8"}}, {"a": {"type": "int32", "count": "l"}},
        {"crc": {"type": "crc16"}}])",
     R"([{"l": 2}, {"a": [1, 1]}])", "020100000001000000D950"},
    {R"([{"v": {"type": "int8"}}, {"a": {"type": "custom", "choose": {"b":
        1, "c": 2}, "depend": "v", "spec": {"b": {"type": "int8"}, "c":
        {"type": "int8"}}}}])",
     R"([{"v": 2}, {"a": {"c": 1}}])", "0201"},
    {R"([{"b": {"type": "bitfield", "size": 1, "spec": {"f": {"pos": 0,
        "size": 1}}}}])",
     R"([{"b": {"f": 1}}])", "80"},
    {R"([{"b": {"type": "bitfield", "size": 1, "spec": {"f1": {"pos": 0,
        "size": 1}, "f2": {"pos": 7, "size": 1}}}}])",
     R"([{"b": {"f1": 1, "f2": 1}}])", "81"},
    {R"([{"b": {"type": "bitfield", "size": 1, "spec": {"f1": {"pos": 0,
        "size": 2, "signed": true}}}}])",
     R"([{"b": {"f1": -1}}])", "C0"},
    {R"([{"b": {"type": "unixtime"}}])",
     R"([{"b": "2022-09-04T22:01:31.902"}])", "7e1ee10983010000"},
    {R"([{"a": {"type": "const", "size": 3, "value": "112233"}}, {"b": {"type": "int8"}}])",
     R"([{"b": 1}])", "11223301"},
};
}

QVariantList getList(const QString &str) {
  QVariantList arr =
      QJsonDocument::fromJson(str.toLatin1()).array().toVariantList();

  return arr;
}

TEST_F(BinarizerTest, EncodeDecodeTest) {
  for (const auto &check : checkList) {
    const QVariantList fieldList = getList(check.fieldStr);
    const QVariantList valueList = getList(check.valueStr);
    const QByteArray testData = QByteArray::fromHex(check.dataHex.toLatin1());

    const QByteArray encData = encoder.encode(fieldList, valueList);
    const QVariantList decList = decoder.decode(fieldList, encData);

    if (!compareVariants(valueList, decList) || (testData != encData)) {
      qDebug() << testData.toHex() << "!= " << encData.toHex();
      qDebug() << valueList << "!= " << decList;
    }

    EXPECT_EQ(testData, encData) << "Failed to encode decode message: ";
    EXPECT_TRUE(compareVariants(valueList, decList))
        << "Failed to encode decode message: ";
  }
}

// TEST_F(BinarizerTest, EncodeTest) {
//   for (const auto &check : checkList) {
//     const QVariantMap testObj = getObj(check.jsonStr);
//     const QByteArray testData =
//     QByteArray::fromHex(check.dataStr.toLatin1()); const QByteArray encData =
//     encoder.encode(check.cat, testObj);

//    if (testData != encData) {
//      qDebug() << testData << "!= " << encData;
//    }

//    EXPECT_EQ(testData, encData)
//        << "Failed to encode message: " << testData.toHex().toStdString()
//        << "; " << encData.toHex().toStdString();
//  }
//}

// TEST_F(BinarizerTest, DecodeTest) {
//   for (const auto &check : checkList) {
//     const QVariantMap testObj = getObj(check.jsonStr);
//     const QByteArray testData =
//     QByteArray::fromHex(check.dataStr.toLatin1()); const QVariantMap decObj =
//     decoder.decode(testData);

//    if (testObj != decObj) {
//      qDebug() << testObj << "!= " << decObj;
//    }

//    EXPECT_EQ(testObj, decObj) << "Failed to decode message: ";
//  }
//}

// TEST_F(BinarizerTest, DecodeEncodeTest) {
//   for (const auto &check : checkList) {
//     const QVariantMap testObj = getObj(check.jsonStr);
//     const QByteArray testData =
//     QByteArray::fromHex(check.dataStr.toLatin1()); const QVariantMap decObj =
//     decoder.decode(testData); const QByteArray encData =
//     encoder.encode(check.cat, decObj);

//    if (testData != encData) {
//      qDebug() << testData << "!= " << encData;
//    }

//    EXPECT_EQ(testData, encData)
//        << "Failed to decode encode message: " << check.jsonStr.toStdString();
//  }
//}
