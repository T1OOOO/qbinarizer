#ifndef BITUTILS_H
#define BITUTILS_H

#include <QBuffer>
#include <QDataStream>
#include <QVariantMap>

template <typename T>
void write24(QDataStream &ds, const T value, const bool reversed) {
  if (reversed) {
    for (int i = 2; i >= 0; i--) {
      const quint8 res = (value >> (i * CHAR_WIDTH)) & 0xff;

      ds << res;
    }
  } else {
    for (int i = 0; i < 3; i++) {
      const quint8 res = (value >> (i * CHAR_WIDTH)) & 0xff;

      ds << res;
    }
  }
}

template <typename T> T read24(QDataStream &ds) {
  T res = 0;

  for (int i = 0; i < 3; i++) {
    quint8 ch = 0;
    ds >> ch;
    res = res | (ch << i * 8);
  }

  return res;
}

template <typename T> T reverse24(const T val) {
  T res = 0;

  for (int i = 0; i < 3; i++) {
    const int ir = 2 - i;
    const T ch = (val >> (i * 8)) & 0xff;

    res = res | (ch << (ir * 8));
  }

  return res;
}

inline qint32 fixSign24(const qint32 val) {
  const bool sign = (val & (1 << 23)) > 0;
  if (!sign) {
    return val;
  }

  qint32 res = (0xff << 24) | (val & 0xffffff);

  return res;
}

template <typename T> QByteArray bytes24(const T val) {
  QByteArray res(3, static_cast<char>(0));

  for (int i = 0; i < 3; i++) {
    const T ch = (val >> (i * 8)) & 0xff;

    res[i] = ch;
  }

  return res;
}

template <typename T> void readValue(QDataStream &ds, QVariant &value) {
  const int size = (int)sizeof(T);
  // field["size"] = size;

  auto *buf = qobject_cast<QBuffer *>(ds.device());
  if (buf != nullptr) {
    const int start = buf->pos();
    // field["start"] = start;
    // field["end"] = start + size;

    const QByteArray &data = buf->buffer();
    const QByteArray subData = data.mid(start, size);

    // field["data"] = subData;
  }

  T res = 0;
  ds >> res;

  // field["endian"] = ds.byteOrder() == QDataStream::BigEndian ? "big" :
  // "little";
  value = res;
}

template <typename T> void writeValue(QDataStream &ds, const QVariant &value) {
  const T val = value.value<T>();

  ds << val;
}

inline unsigned char reverseChar(unsigned char b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

#endif // BITUTILS_H
