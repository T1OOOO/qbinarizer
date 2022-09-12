#ifndef STRUCTENCODER_H
#define STRUCTENCODER_H

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QObject>
#include <QVariantList>

#include "qbinarizer/export/qbinarizer_export.h"

namespace qbinarizer {

class QBINARIZER_EXPORT StructEncoder : public QObject {
  Q_OBJECT
public:
  explicit StructEncoder(QObject *parent = nullptr);

  QByteArray encode(const QString &datafieldListStr,
                    const QString &valueListStr = QString());

  QByteArray encode(const QVariantList &datafieldList,
                    const QVariantList &valueList = QVariantList());

  void clear();

protected:
  void encode();

  void encodeList(const QVariantList &datafieldList,
                  const QVariantList &valueList);

  void encodeMap(const QVariantMap &datafield, const QVariantMap &value);

  void encodeValue(const QVariantMap &field, const QVariant &valueData);

  void encodeBitfield(const QVariantMap &field, const QVariant &valueData);

  void encodeBitfieldElement(const QVariantMap &field, const QVariant &value,
                             QByteArray &data);

  void encodeСrc(const QVariantMap &field);

  void encodeRaw(const QVariantMap &field, const QVariant &valueData);

  void encodeCustom(const QVariantMap &field, const QVariant &valueData);

  void encodeStruct(const QVariantMap &field, const QVariant &valueData);

  void encodeSkip(const QVariantMap &field);

  void encodeUnixtime(const QVariantMap &field, const QVariant &valueData);

  void encodeConst(const QVariantMap &field, const QVariant &valueData);

  void updateEncodedTo(const QString &name);

  QVariant getEncodedValue(const QString &name) const;

  QVariant generateEncodedValue(const QString &name, const QVariant &value);

private:
  QVariantMap m_encodedFields;
  QVariantList m_datafieldList;
  QVariantList m_valueList;

  QBuffer m_buf;
  QDataStream m_ds;
};

} // namespace qbinarizer

#endif // STRUCTDECODER_H
