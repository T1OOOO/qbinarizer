#include "jsonutils.h"

#include <QJsonArray>
#include <QJsonDocument>

QVariantList parseJson(const QString &str) {
  {
    QJsonParseError error;

    QJsonArray arr = QJsonDocument::fromJson(str.toLatin1(), &error).array();
    if (error.error != QJsonParseError::NoError) {
      qDebug() << "parse error: " << error.errorString();

      return {};
    }

    return arr.toVariantList();
  }
}
