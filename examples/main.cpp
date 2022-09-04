#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtDebug>

#include "src/jsonutils.h"
#include <qbinarizer/StructDecoder>
#include <qbinarizer/StructEncoder>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  const QString descriptionStr =
      QStringLiteral("[{\"a\": {\"type\": \"int32\"}}, {\"d\": {\"type\": "
                     "\"crc32\"}}]");

  const QString valueStr = QStringLiteral("[{\"a\": 1}]");

  const QVariantList datafieldList = parseJson(descriptionStr);
  qDebug() << "fieldsEnc: " << datafieldList;

  const QVariantList valueList = parseJson(valueStr);

  qbinarizer::StructEncoder encoder;
  const QByteArray dataEnc = encoder.encode(datafieldList, valueList);
  qDebug() << "dataEnc: " << dataEnc;

  qbinarizer::StructDecoder decoder;
  const QVariantList resList = decoder.decode(datafieldList, dataEnc);
  qDebug() << "fieldsDec: " << QJsonArray::fromVariantList(resList);

  return 0;
}
