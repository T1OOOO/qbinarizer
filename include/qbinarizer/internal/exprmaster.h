#ifndef EXPRMASTER_H
#define EXPRMASTER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "qbinarizer/export/qbinarizer_export.h"

struct te_expr;
struct te_variable;

namespace qbinarizer {

class QBINARIZER_EXPORT ExprMaster : public QObject {
  Q_OBJECT
public:
  explicit ExprMaster(QObject *parent = nullptr);

  ~ExprMaster() override;

  bool isInit() const;

  void setVars(const QVariantList &varList);

  void updateVars(const QVariantList &varList);

  double eval();

  void setExpr(const QString &str);

  void clear();

protected:
  bool compile();

private:
  QString m_exprStr;
  QVariantList m_varList;
  QMap<std::string, double> m_varMap;
  std::vector<te_variable> m_exprVarVec;
  te_expr *m_expr;
};

} // namespace qbinarizer

#endif // EXPRMASTER_H
