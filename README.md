# qasterix
Library for encode and decode binary structs

This project was inspired by idea that protocols could be parsed without compilation

Sample code:
```C++
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
```