/*
 * 文件名: modelmanager.cpp
 * 作用:
 * 1. 拦截108个模型界面懒加载要求。
 * 2. 分配数学求解任务。
 * 3. 静态提供两级拼装架构合并的
 * 全部初始物理字典池。
 */

#include "modelmanager.h"
#include "modelselect.h"
#include "modelparameter.h"
#include "wt_modelwidget.h"
#include "modelsolver01.h"
#include "modelsolver02.h"
#include "modelsolver03.h"
#include <QVBoxLayout>

// 构造
ModelManager::ModelManager(
    QWidget* parent) :
    QObject(parent),
    m_mainWidget(nullptr),
    m_modelStack(nullptr),
    m_currentModelType(Model_1) {}

// 析构清理内存
ModelManager::~ModelManager() {
    for(auto* s : m_solversGroup1)
        if(s) delete s;
    for(auto* s : m_solversGroup2)
        if(s) delete s;
    for(auto* s : m_solversGroup3)
        if(s) delete s;
}

// 界面搭建
void ModelManager::initializeModels(
    QWidget* parentWidget)
{
    if (!parentWidget) return;
    createMainWidget();
    m_modelStack = new QStackedWidget(
        m_mainWidget);

    m_modelWidgets.resize(108);
    m_modelWidgets.fill(nullptr);
    m_solversGroup1.resize(36);
    m_solversGroup1.fill(nullptr);
    m_solversGroup2.resize(36);
    m_solversGroup2.fill(nullptr);
    m_solversGroup3.resize(36);
    m_solversGroup3.fill(nullptr);

    m_mainWidget->layout()->addWidget(
        m_modelStack);
    switchToModel(Model_1);

    if (parentWidget->layout()) {
        parentWidget->layout()->
            addWidget(m_mainWidget);
    } else {
        QVBoxLayout* lay =
            new QVBoxLayout(
                parentWidget);
        lay->addWidget(m_mainWidget);
        parentWidget->setLayout(lay);
    }
}

// 宿主
void ModelManager::createMainWidget() {
    m_mainWidget = new QWidget();
    QVBoxLayout* ml = new QVBoxLayout(
        m_mainWidget);
    ml->setContentsMargins(0,0,0,0);
    ml->setSpacing(0);
    m_mainWidget->setLayout(ml);
}

// 懒加载界面
WT_ModelWidget* ModelManager::
    ensureWidget(ModelType type)
{
    int i = (int)type;
    if (i < 0 || i >= 108)
        return nullptr;

    if (!m_modelWidgets[i]) {
        WT_ModelWidget* w =
            new WT_ModelWidget(
                type, m_modelStack);
        m_modelWidgets[i] = w;
        m_modelStack->addWidget(w);

        connect(w,
                &WT_ModelWidget::
                requestModelSelection,
                this,
                &ModelManager::
                onSelectModelClicked);

        connect(w,
                &WT_ModelWidget::
                calculationCompleted,
                this,
                &ModelManager::
                onWidgetCalculationCompleted);
    }
    return m_modelWidgets[i];
}

ModelSolver01* ModelManager::
    ensureSolverGroup1(int idx) {
    if (!m_solversGroup1[idx])
        m_solversGroup1[idx] =
            new ModelSolver01(
                (ModelSolver01::ModelType)
                idx);
    return m_solversGroup1[idx];
}

ModelSolver02* ModelManager::
    ensureSolverGroup2(int idx) {
    if (!m_solversGroup2[idx])
        m_solversGroup2[idx] =
            new ModelSolver02(
                (ModelSolver02::ModelType)
                idx);
    return m_solversGroup2[idx];
}

ModelSolver03* ModelManager::
    ensureSolverGroup3(int idx) {
    if (!m_solversGroup3[idx])
        m_solversGroup3[idx] =
            new ModelSolver03(
                (ModelSolver03::ModelType)
                idx);
    return m_solversGroup3[idx];
}

// 界面切换(含参数记忆接力)
void ModelManager::switchToModel(
    ModelType modelType)
{
    if (!m_modelStack) return;
    ModelType old = m_currentModelType;

    QMap<QString, QString> curT;
    if (m_modelWidgets[old])
        curT = m_modelWidgets[old]->
               getUiTexts();

    m_currentModelType = modelType;
    WT_ModelWidget* w =
        ensureWidget(modelType);

    if (w) {
        if (!curT.isEmpty())
            w->setUiTexts(curT);
        m_modelStack->
            setCurrentWidget(w);
    }
    emit modelSwitched(modelType, old);
}

// 计算路由
ModelCurveData ModelManager::
    calculateTheoreticalCurve(
        ModelType type,
        const QMap<QString, double>& p,
        const QVector<double>& t)
{
    int id = (int)type;
    if (id <= 35)
        return ensureSolverGroup1(id)
            ->calculateTheoreticalCurve(
                p, t);
    else if (id <= 71)
        return ensureSolverGroup2(id-36)
            ->calculateTheoreticalCurve(
                p, t);
    else
        return ensureSolverGroup3(id-72)
            ->calculateTheoreticalCurve(
                p, t);
}

// 获取标题
QString ModelManager::
    getModelTypeName(ModelType type) {
    int id = (int)type;
    if (id <= 35)
        return ModelSolver01::
            getModelName(
                (ModelSolver01::ModelType)id);
    else if (id <= 71)
        return ModelSolver02::
            getModelName(
                (ModelSolver02::ModelType)
                (id - 36));
    else
        return ModelSolver03::
            getModelName(
                (ModelSolver03::ModelType)
                (id - 72));
}

// 弹出窗口
void ModelManager::
    onSelectModelClicked() {
    ModelSelect dlg(m_mainWidget);
    dlg.setCurrentModelCode(
        QString("modelwidget%1").arg(
            (int)m_currentModelType + 1));

    if (dlg.exec() ==
        QDialog::Accepted) {
        QString numS =
            dlg.getSelectedModelCode();
        numS.remove("modelwidget");
        int mId = numS.toInt();
        if (mId >= 1 && mId <= 108)
            switchToModel(
                (ModelType)(mId - 1));
    }
}

// 中枢配置生成池
QMap<QString, double> ModelManager::
    getDefaultParameters(ModelType type)
{
    QMap<QString, double> p;
    ModelParameter* mp =
        ModelParameter::instance();

    p.insert("phi", mp->getPhi());
    p.insert("h", mp->getH());
    p.insert("rw", mp->getRw());
    p.insert("mu", mp->getMu());
    p.insert("B", mp->getB());
    p.insert("Ct", mp->getCt());
    p.insert("q", mp->getQ());
    p.insert("L", mp->getL());
    p.insert("nf", mp->getNf());
    p.insert("alpha", mp->getAlpha());
    p.insert("C_phi", mp->getCPhi());

    p.insert("kf", 10.0);
    p.insert("M12", 5.0);
    p.insert("Lf", 50.0);
    p.insert("rm", 5000.0);
    p.insert("gamaD", 0.006);

    int t = (int)type;
    bool isMix = (t >= 72 && t <= 107);
    int gI = t % 12;

    if (gI >= 4) {
        p.insert("re", isMix ?
                           500000.0 : 20000.0);
    }

    if (isMix) {
        int sub3 = t - 72;
        p.insert("omega_f1", 0.02);
        p.insert("omega_v1", 0.01);
        p.insert("lambda_m1", 1e-4);
        p.insert("lambda_v1", 1e-1);
        if (sub3 < 24) {
            p.insert("omega_f2", 0.008);
            p.insert("lambda_m2", 1e-7);
        }
    } else {
        p.insert("omega1", 0.4);
        p.insert("omega2", 0.08);
        p.insert("lambda1", 1e-3);
        p.insert("lambda2", 1e-4);
    }

    int sT = t % 4;
    if (sT != 1) {
        p.insert("C", 100.0);
        p.insert("S", 10.0);
    }

    p.insert("t", 1e10);
    p.insert("points", 100.0);

    return p;
}

// 精度穿透
void ModelManager::
    setHighPrecision(bool h) {
    for(WT_ModelWidget* w :
         m_modelWidgets)
        if(w) w->setHighPrecision(h);

    for(ModelSolver01* s :
         m_solversGroup1)
        if(s) s->setHighPrecision(h);

    for(ModelSolver02* s :
         m_solversGroup2)
        if(s) s->setHighPrecision(h);

    for(ModelSolver03* s :
         m_solversGroup3)
        if(s) s->setHighPrecision(h);
}

// 刷新全部
void ModelManager::
    updateAllModelsBasicParameters() {
    for(WT_ModelWidget* w :
         m_modelWidgets)
        if(w) QMetaObject::invokeMethod(
                w, "onResetParameters");
}

// 数据写入读出
void ModelManager::setObservedData(
    const QVector<double>& t,
    const QVector<double>& p,
    const QVector<double>& d)
{
    m_cachedObsTime = t;
    m_cachedObsPressure = p;
    m_cachedObsDeriv = d;
}

void ModelManager::getObservedData(
    QVector<double>& t,
    QVector<double>& p,
    QVector<double>& d) const
{
    t = m_cachedObsTime;
    p = m_cachedObsPressure;
    d = m_cachedObsDeriv;
}

void ModelManager::clearCache() {
    m_cachedObsTime.clear();
    m_cachedObsPressure.clear();
    m_cachedObsDeriv.clear();
}

bool ModelManager::hasObservedData()
    const {
    return !m_cachedObsTime.isEmpty();
}

// 完成回调透传
void ModelManager::
    onWidgetCalculationCompleted(
        const QString &t,
        const QMap<QString, double> &r)
{
    emit calculationCompleted(t, r);
}

// 时序列生成
QVector<double> ModelManager::
    generateLogTimeSteps(
        int c, double s, double e)
{
    return ModelSolver01::
        generateLogTimeSteps(c, s, e);
}
