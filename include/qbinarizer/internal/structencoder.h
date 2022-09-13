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

  std::tuple<QByteArray, QVariantList>
  encode(const QString &datafieldListStr,
         const QString &valueListStr = QString());

  std::tuple<QByteArray, QVariantList>
  encode(const QVariantList &datafieldList,
         const QVariantList &valueList = QVariantList());

  void clear();

protected:
  void encode();

  QVariantList encodeList(const QVariantList &datafieldList,
                          const QVariantList &valueList);

  QVariantMap encodeMap(const QVariantMap &datafield, const QVariantMap &value);

  QVariantMap encodeValue(const QVariantMap &field, const QVariant &valueData);

  QVariantMap encodeBitfield(const QVariantMap &field,
                             const QVariant &valueData);

  QVariantMap encodeBitfieldElement(const QVariantMap &field,
                                    const QVariant &value, QByteArray &data);

  QVariantMap encodeСrc(const QVariantMap &field);

  QVariantMap encodeRaw(const QVariantMap &field, const QVariant &valueData);

  QVariantMap encodeSkip(const QVariantMap &field);

  QVariantMap encodeCustom(const QVariantMap &field, const QVariant &valueData);

  QVariantMap encodeStruct(const QVariantMap &field, const QVariant &valueData);

  QVariantMap encodeUnixtime(const QVariantMap &field,
                             const QVariant &valueData);

  QVariantMap encodeConst(const QVariantMap &field, const QVariant &valueData);

  void updateEncodedTo(const QString &name);

  QVariant getEncodedValue(const QString &name) const;

  QVariant generateEncodedValue(const QString &name, const QVariant &value);

private:
  QVariantMap m_encodedFields;
  QVariantList m_datafieldList;
  QVariantList m_valueList;
  QVariantList m_encodeList;

  QBuffer m_buf;
  QDataStream m_ds;
};

} // namespace qbinarizer

#endif // STRUCTDECODER_H
