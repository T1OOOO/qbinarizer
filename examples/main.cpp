#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtDebug>

#include "mystruct.h"
#include "src/jsonutils.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <qbinarizer/StructDecoder>
#include <qbinarizer/StructEncoder>
#include <qbinarizer/StructReflector>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  qRegisterMetaType<MyStructParent>("MyStructParent");
  qRegisterMetaType<MyStructChild>("MyStructChild");

  //  const QVariantList fieldSpecList =
  //      StructReflector::createFieldSpecList(&MyStructParent::staticMetaObject);
  //  qDebug() << "fieldSpecList: "
  //           << QJsonDocument(QJsonArray::fromVariantList(fieldSpecList))
  //                  .toJson(QJsonDocument::Compact);

  MyStructParent parent;
  //  parent.a = 123;
  //  parent.child.b = 2;

  const QString valueStr = R"({"a": 123, "child": {"b": 2}})";

  QJsonParseError error;
  const QVariantMap valueMap =
      QJsonDocument::fromJson(valueStr.toLatin1(), &error)
          .object()
          .toVariantMap();
  if (error.error != QJsonParseError::NoError) {
    qDebug() << "error: " << error.errorString();

    return -1;
  }

  const int size = qbinarizer::StructReflector::setValuesString<MyStructParent>(
      &parent, valueMap);
  qDebug() << "size: " << size;

  const QString str =
      qbinarizer::StructReflector::getValuesString<MyStructParent>(&parent);
  qDebug() << "MyStructParent: " << str;

  // return propertyList;

  //  const QString descriptionStr =
  //      QStringLiteral("[{\"a\": {\"type\": \"int32\"}}, {\"d\":
  //      {\"type\": "
  //                     "\"crc32\"}}]");

  //  const QString valueStr = QStringLiteral("[{\"a\": 1}]");

  //  const QVariantList datafieldList = parseJson(descriptionStr);
  //  qDebug() << "fieldsEnc: " << datafieldList;

  //  const QVariantList valueList = parseJson(valueStr);

  //  qbinarizer::StructEncoder encoder;
  //  const QByteArray dataEnc = encoder.encode(datafieldList, valueList);
  //  qDebug() << "dataEnc: " << dataEnc;

  //  qbinarizer::StructDecoder decoder;
  //  const QVariantList resList = decoder.decode(datafieldList, dataEnc);
  //  qDebug() << "fieldsDec: " << QJsonArray::fromVariantList(resList);

  return 0;
}
