#include "internal/exprmaster.h"

ExprMaster::ExprMaster(QObject *parent) : QObject{parent}, m_expr(nullptr) {}

ExprMaster::~ExprMaster() { clear(); }

void ExprMaster::setVars(const QVariantList &varList) {
  m_varList = varList;

  compile();
}

void ExprMaster::updateVars(const QVariantList &varList) {
  for (int i = 0; i < varList.size(); i++) {
    const QVariantMap varMap = varList.at(i).toMap();
    if (varMap.isEmpty()) {
      continue;
    }

    const QString &varName = varMap.firstKey();
    const QVariant &var = varMap[varName];
    if (!var.canConvert<double>()) {
      continue;
    }

    const std::string varNameStd = varName.toStdString();
    if (!m_varMap.contains(varNameStd)) {
      continue;
    }

    const double value = var.toDouble();
    m_varMap[varNameStd] = value;
  }
}

double ExprMaster::eval() {
  if (m_expr == nullptr) {
    return 0.0;
  }

  double res = te_eval(m_expr);

  return res;
}

void ExprMaster::setExpr(const QString &str) {
  m_exprStr = str;

  compile();
}

void ExprMaster::clear() {
  if (m_expr != nullptr) {
    te_free(m_expr);
    m_expr = nullptr;
  }

  m_exprVarVec.clear();
  m_varMap.clear();
  m_varList.clear();
  m_exprStr.clear();
}

bool ExprMaster::compile() {
  clear();

  for (int i = 0; i < m_varList.size(); i++) {
    const QVariantMap varMap = m_varList.at(i).toMap();
    if (varMap.isEmpty()) {
      continue;
    }

    const QString &varName = varMap.firstKey();
    const QVariant &var = varMap[varName];
    if (!var.canConvert<double>()) {
      continue;
    }

    const double value = var.toDouble();
    const std::string varNameStd = varName.toStdString();
    m_varMap[varNameStd] = value;
  }

  QMapIterator<std::string, double> it(m_varMap);
  while (it.hasNext()) {
    it.next();

    m_exprVarVec.push_back({it.key().data(),
                            reinterpret_cast<const void *>(&it.value()), 0,
                            nullptr});
  }

  int err = 0;
  m_expr = te_compile(m_exprStr.toLatin1().data(), m_exprVarVec.data(),
                      m_exprVarVec.size(), &err);
  if (err != 0) {
    clear();
  }

  return (err == 0);
}

bool ExprMaster::isInit() const {
  bool init = (m_expr != nullptr);

  return init;
}
