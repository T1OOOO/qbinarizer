#ifndef STRUCTDECODER_H
#define STRUCTDECODER_H

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QObject>
#include <QVariantMap>

namespace qbinarizer {

class StructDecoder : public QObject {
  Q_OBJECT
public:
  explicit StructDecoder(QObject *parent = nullptr);

  QVariantList decode(const QString &datafieldListStr, const QByteArray &data);

  QVariantList decode(const QVariantList &datafieldList,
                      const QByteArray &data);

  /**
   * @brief clear Clear internal state of object
   */
  void clear();

protected:
  void decode();

  QVariant parse(const QVariant &datafield);

  QVariantMap decodeMap(const QVariantMap &field);

  QVariantList decodeList(const QVariantList &fieldList);

  QVariantMap decodeBitfield(const QVariantMap &field);

  QVariant decodeBitfieldElement(const QVariantMap &field,
                                 const QByteArray &data);

  QVariantMap decodeCustom(const QVariantMap &field);

  QVariantMap decodeRaw(const QVariantMap &field);

  QVariantMap decodeValue(const QVariantMap &field);

  QVariantMap decodeCrc(const QVariantMap &field);

  QVariantMap decodeUnixtime(const QVariantMap &field);

  QVariantMap decodeConst(const QVariantMap &field);

  void updateEncodedTo(const QString &name);

  QVariant getDecodedValue(const QString &name) const;

  QVariant generateDecodedValue(const QString &name, const QVariant &value);

  void setDecodedValue(const QString &name, const QVariant &value);

private:
  QString m_name;
  QVariantList m_datafieldList;
  int m_pos;

  QBuffer m_buf;
  QDataStream m_ds;
  QByteArray m_data;
  QVariantList m_resList;
  QVariantMap m_decodedFields;
};

} // namespace qbinarizer

#endif // STRUCTDECODER_H
