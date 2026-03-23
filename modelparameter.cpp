/*
 * 文件名: modelparameter.cpp
 * 文件作用与功能:
 * 1. 实例化全局第一级参数，在程序刚启动或新建项目时提供对齐 MATLAB 理论源码的兜底默认值。
 * 2. 详细实现了与项目文件 (.pwt) 及其关联产物 (_chart.json, _date.json) 的硬盘读写逻辑。
 * 3. 实现了 JSON 数据的序列化与反序列化，保证所有新旧版本的属性项能够平滑兼容并落盘。
 */

#include "modelparameter.h"
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDebug>

ModelParameter* ModelParameter::m_instance = nullptr;

/**
 * @brief 构造函数：按照 MATLAB 代码中的设定，对全局客观物理属性赋予符合油藏工程理论的初始占位值
 */
ModelParameter::ModelParameter(QObject* parent) : QObject(parent), m_hasLoaded(false)
{
    // 对齐 MATLAB (model_Double_1.m 等) 的默认全局物性设定
    m_L = 1000.0;   // 水平井总长默认为 1000 m
    m_nf = 9.0;     // 裂缝条数默认为 9 条
    m_q = 10.0;     // 默认产量 10 m³/d
    m_phi = 0.05;   // 默认致密孔隙度 0.05
    m_h = 10.0;     // 默认储层厚度 10 m
    m_rw = 0.1;     // 默认井筒半径 0.1 m
    m_Ct = 0.005;   // 默认综合压缩系数 5e-3 MPa^-1
    m_mu = 5.0;     // 默认粘度 5.0 mPa·s
    m_B = 1.2;      // 默认体积系数

    // 特殊扩展项 (针对变井储模型)
    m_alpha = 0.1;    // 变井储时间参数 alpha
    m_C_phi = 0.0001; // 变井储压力参数 C_phi
}

/**
 * @brief 懒汉式获取全局唯一实例的指针
 */
ModelParameter* ModelParameter::instance()
{
    if (!m_instance) m_instance = new ModelParameter();
    return m_instance;
}

/**
 * @brief 在新建项目确认时调用，更新内存中的所有第一级物性参数并构建基础 JSON 对象格式
 */
void ModelParameter::setParameters(double phi, double h, double mu, double B, double Ct,
                                   double q, double rw, double L, double nf, const QString& path)
{
    m_phi = phi; m_h = h; m_mu = mu; m_B = B; m_Ct = Ct; m_q = q; m_rw = rw;
    m_L = L; m_nf = nf;

    m_projectFilePath = path;

    // 解析出文件所在的上级目录
    QFileInfo fi(path);
    m_projectPath = fi.isFile() ? fi.absolutePath() : path;
    m_hasLoaded = true;

    // 如果该项目没有旧数据缓存，则初始化标准的存储树结构
    if (m_fullProjectData.isEmpty()) {
        QJsonObject reservoir;
        reservoir["porosity"] = m_phi;
        reservoir["thickness"] = m_h;
        reservoir["wellRadius"] = m_rw;
        reservoir["productionRate"] = m_q;
        reservoir["horizLength"] = m_L;
        reservoir["fracCount"] = m_nf;
        reservoir["alpha"] = m_alpha;
        reservoir["C_phi"] = m_C_phi;

        QJsonObject pvt;
        pvt["viscosity"] = m_mu;
        pvt["volumeFactor"] = m_B;
        pvt["compressibility"] = m_Ct;

        m_fullProjectData["reservoir"] = reservoir;
        m_fullProjectData["pvt"] = pvt;
    }
}

/**
 * @brief 写入变井储时间参数
 */
void ModelParameter::setAlpha(double v) { m_alpha = v; }

/**
 * @brief 写入变井储压力参数
 */
void ModelParameter::setCPhi(double v) { m_C_phi = v; }

/**
 * @brief 自动推导并返回绘图数据文件 (_chart.json) 的完整路径
 */
QString ModelParameter::getPlottingDataFilePath() const
{
    if (m_projectFilePath.isEmpty()) return QString();
    QFileInfo fi(m_projectFilePath);
    return fi.absolutePath() + "/" + fi.completeBaseName() + "_chart.json";
}

/**
 * @brief 自动推导并返回数据表格文件 (_date.json) 的完整路径
 */
QString ModelParameter::getTableDataFilePath() const
{
    if (m_projectFilePath.isEmpty()) return QString();
    QFileInfo fi(m_projectFilePath);
    return fi.absolutePath() + "/" + fi.completeBaseName() + "_date.json";
}

/**
 * @brief 读取硬盘上的 .pwt 文件并反序列化回程序的各个第一级参数变量中
 */
bool ModelParameter::loadProject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return false;

    m_fullProjectData = doc.object();

    // 尝试解析油藏相关的物理属性
    if (m_fullProjectData.contains("reservoir")) {
        QJsonObject res = m_fullProjectData["reservoir"].toObject();
        m_q = res["productionRate"].toDouble(10.0);
        m_phi = res["porosity"].toDouble(0.05);
        m_h = res["thickness"].toDouble(10.0);
        m_rw = res["wellRadius"].toDouble(0.1);
        m_L = res["horizLength"].toDouble(1000.0);
        m_nf = res["fracCount"].toDouble(9.0);
        m_alpha = res["alpha"].toDouble(0.1);
        m_C_phi = res["C_phi"].toDouble(0.0001);
    }

    // 尝试解析 PVT 相关的流体属性
    if (m_fullProjectData.contains("pvt")) {
        QJsonObject pvt = m_fullProjectData["pvt"].toObject();
        m_Ct = pvt["compressibility"].toDouble(0.005);
        m_mu = pvt["viscosity"].toDouble(5.0);
        m_B = pvt["volumeFactor"].toDouble(1.2);
    }

    m_projectFilePath = filePath;
    m_projectPath = QFileInfo(filePath).absolutePath();
    m_hasLoaded = true;

    // 静默加载并挂载可能存在的附属绘图分析文件
    QFile chartFile(getPlottingDataFilePath());
    if (chartFile.exists() && chartFile.open(QIODevice::ReadOnly)) {
        QJsonDocument d = QJsonDocument::fromJson(chartFile.readAll());
        if (!d.isNull() && d.isObject() && d.object().contains("plotting_data")) {
            m_fullProjectData["plotting_data"] = d.object()["plotting_data"];
        }
        chartFile.close();
    }

    // 静默加载并挂载可能存在的附属分析表格文件
    QFile dateFile(getTableDataFilePath());
    if (dateFile.exists() && dateFile.open(QIODevice::ReadOnly)) {
        QJsonDocument d = QJsonDocument::fromJson(dateFile.readAll());
        if (!d.isNull() && d.isObject() && d.object().contains("table_data")) {
            m_fullProjectData["table_data"] = d.object()["table_data"];
        }
        dateFile.close();
    } else {
        // 若没有找到，则清除字典中的无效节点
        m_fullProjectData.remove("table_data");
    }

    return true;
}

/**
 * @brief 将内存中发生更改的第一级参数组装成 JSON，并向硬盘持久化覆写
 */
bool ModelParameter::saveProject()
{
    if (!m_hasLoaded || m_projectFilePath.isEmpty()) return false;

    QJsonObject reservoir = m_fullProjectData["reservoir"].toObject();
    reservoir["porosity"] = m_phi;
    reservoir["thickness"] = m_h;
    reservoir["wellRadius"] = m_rw;
    reservoir["productionRate"] = m_q;
    reservoir["horizLength"] = m_L;
    reservoir["fracCount"] = m_nf;
    reservoir["alpha"] = m_alpha;
    reservoir["C_phi"] = m_C_phi;
    m_fullProjectData["reservoir"] = reservoir;

    QJsonObject pvt = m_fullProjectData["pvt"].toObject();
    pvt["viscosity"] = m_mu;
    pvt["volumeFactor"] = m_B;
    pvt["compressibility"] = m_Ct;
    m_fullProjectData["pvt"] = pvt;

    // 分离存储：主文件不包含过大的分析缓存数据
    QJsonObject dataToWrite = m_fullProjectData;
    dataToWrite.remove("plotting_data");
    dataToWrite.remove("table_data");

    QFile file(m_projectFilePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(dataToWrite).toJson());
    file.close();

    return true;
}

/**
 * @brief 关闭当前已加载的工程状态，并执行数据清理
 */
void ModelParameter::closeProject() { resetAllData(); }

/**
 * @brief 保存拟合模块传入的最佳逼近反演参数与误差历史
 */
void ModelParameter::saveFittingResult(const QJsonObject& fittingData)
{
    if (m_projectFilePath.isEmpty()) return;
    m_fullProjectData["fitting"] = fittingData;

    QFile file(m_projectFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject dataToWrite = m_fullProjectData;
        dataToWrite.remove("plotting_data");
        dataToWrite.remove("table_data");
        file.write(QJsonDocument(dataToWrite).toJson());
        file.close();
    }
}

/**
 * @brief 获取当前工程记录在案的自动拟合结果数据包
 */
QJsonObject ModelParameter::getFittingResult() const { return m_fullProjectData.value("fitting").toObject(); }

/**
 * @brief 将图版模块生成的曲线集合剥离并保存入专属 JSON 附属文件
 */
void ModelParameter::savePlottingData(const QJsonArray& plots)
{
    if (m_projectFilePath.isEmpty()) return;
    m_fullProjectData["plotting_data"] = plots;
    QJsonObject dataObj; dataObj["plotting_data"] = plots;
    QFile file(getPlottingDataFilePath());
    if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(dataObj).toJson()); file.close(); }
}

/**
 * @brief 提取当前绑定的附属分析图表数据
 */
QJsonArray ModelParameter::getPlottingData() const { return m_fullProjectData.value("plotting_data").toArray(); }

/**
 * @brief 将数据表模块的明细矩阵剥离并保存入专属 JSON 附属文件
 */
void ModelParameter::saveTableData(const QJsonArray& tableData)
{
    if (m_projectFilePath.isEmpty()) return;
    m_fullProjectData["table_data"] = tableData;
    QJsonObject dataObj; dataObj["table_data"] = tableData;
    QFile file(getTableDataFilePath());
    if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(dataObj).toJson()); file.close(); }
}

/**
 * @brief 提取当前绑定的附属明细表格数据
 */
QJsonArray ModelParameter::getTableData() const { return m_fullProjectData.value("table_data").toArray(); }

/**
 * @brief 强制所有变量退回默认物理设定，清除内存中的残留 JSON 树，用于关闭项目操作
 */
void ModelParameter::resetAllData()
{
    m_L = 1000.0; m_nf = 9.0; m_q = 10.0; m_phi = 0.05;
    m_h = 10.0; m_rw = 0.1; m_Ct = 0.005; m_mu = 5.0; m_B = 1.2;
    m_alpha = 0.1; m_C_phi = 0.0001;

    m_hasLoaded = false;
    m_projectPath.clear();
    m_projectFilePath.clear();
    m_fullProjectData = QJsonObject();
}
