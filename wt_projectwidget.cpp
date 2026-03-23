/*
 * 文件名: wt_projectwidget.cpp
 * 文件作用: 项目管理界面实现文件
 * 功能描述:
 * 1. 初始化界面样式和按钮事件连接。
 * 2. 实现"新建"、"打开"、"关闭"、"退出"的详细交互逻辑。
 * 3. 【修改点】：退出动作重构为触发主窗口的关闭指令，委托主窗口执行统一的确认弹窗。
 */

#include "wt_projectwidget.h"
#include "ui_wt_projectwidget.h"
#include "newprojectdialog.h"
#include "modelparameter.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QPalette>
#include <QFileInfo>
#include <QPushButton>

WT_ProjectWidget::WT_ProjectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WT_ProjectWidget),
    m_isProjectOpen(false),
    m_currentProjectFilePath("")
{
    ui->setupUi(this);
    init();
}

WT_ProjectWidget::~WT_ProjectWidget()
{
    delete ui;
}

void WT_ProjectWidget::init()
{
    this->setStyleSheet("background-color: transparent;");
    ui->widget_5->setStyleSheet("background-color: transparent;");

    ui->gridLayout_3->setHorizontalSpacing(30);
    ui->gridLayout_3->setVerticalSpacing(10);

    QFont bigFont;
    bigFont.setPointSize(16);
    bigFont.setBold(true);

    QColor backgroundColor(148, 226, 255);

    QString forceStyle = QString(
        "MonitoStateW { "
        "background-color: rgb(148, 226, 255); "
        "border-radius: 10px; "
        "padding: 10px; "
        "} "
        "MonitoStateW * { "
        "background-color: transparent; "
        "} "
        "MonitoStateW:hover { "
        "background-color: rgb(120, 200, 240); "
        "} "
        "QLabel { "
        "color: #333333; "
        "font-weight: bold; "
        "margin-top: 5px; "
        "background-color: transparent; "
        "}"
        );

    QString topPicStyle = "";
    QString topName = "  ";

    QString centerPicStyle1 = "border-image: url(:/new/prefix1/Resource/PRO1.png);";
    ui->MonitState1->setTextInfo(centerPicStyle1, topPicStyle, topName, "新建");
    ui->MonitState1->setFixedSize(128, 160);
    ui->MonitState1->setStyleSheet(forceStyle);
    ui->MonitState1->setAutoFillBackground(true);
    QPalette pal1 = ui->MonitState1->palette(); pal1.setColor(QPalette::Window, backgroundColor); ui->MonitState1->setPalette(pal1);
    ui->MonitState1->setFont(bigFont);
    connect(ui->MonitState1, SIGNAL(sigClicked()), this, SLOT(onNewProjectClicked()));

    QString centerPicStyle2 = "border-image: url(:/new/prefix1/Resource/PRO2.png);";
    ui->MonitState2->setTextInfo(centerPicStyle2, topPicStyle, topName, "打开");
    ui->MonitState2->setFixedSize(128, 160);
    ui->MonitState2->setStyleSheet(forceStyle);
    ui->MonitState2->setAutoFillBackground(true);
    QPalette pal2 = ui->MonitState2->palette(); pal2.setColor(QPalette::Window, backgroundColor); ui->MonitState2->setPalette(pal2);
    ui->MonitState2->setFont(bigFont);
    connect(ui->MonitState2, SIGNAL(sigClicked()), this, SLOT(onOpenProjectClicked()));

    QString centerPicStyle3 = "border-image: url(:/new/prefix1/Resource/PRO3.png);";
    ui->MonitState3->setTextInfo(centerPicStyle3, topPicStyle, topName, "关闭");
    ui->MonitState3->setFixedSize(128, 160);
    ui->MonitState3->setStyleSheet(forceStyle);
    ui->MonitState3->setAutoFillBackground(true);
    QPalette pal3 = ui->MonitState3->palette(); pal3.setColor(QPalette::Window, backgroundColor); ui->MonitState3->setPalette(pal3);
    ui->MonitState3->setFont(bigFont);
    connect(ui->MonitState3, SIGNAL(sigClicked()), this, SLOT(onCloseProjectClicked()));

    QString centerPicStyle4 = "border-image: url(:/new/prefix1/Resource/PRO4.png);";
    ui->MonitState4->setTextInfo(centerPicStyle4, topPicStyle, topName, "退出");
    ui->MonitState4->setFixedSize(128, 160);
    ui->MonitState4->setStyleSheet(forceStyle);
    ui->MonitState4->setAutoFillBackground(true);
    QPalette pal4 = ui->MonitState4->palette(); pal4.setColor(QPalette::Window, backgroundColor); ui->MonitState4->setPalette(pal4);
    ui->MonitState4->setFont(bigFont);
    connect(ui->MonitState4, SIGNAL(sigClicked()), this, SLOT(onExitClicked()));
}

void WT_ProjectWidget::setProjectState(bool isOpen, const QString& filePath)
{
    m_isProjectOpen = isOpen;
    m_currentProjectFilePath = filePath;
}

QString WT_ProjectWidget::getMessageBoxStyle() const
{
    return "QMessageBox { background-color: #ffffff; color: #000000; }"
           "QLabel { color: #000000; background-color: transparent; }"
           "QPushButton { "
           "   color: #000000; "
           "   background-color: #f0f0f0; "
           "   border: 1px solid #c0c0c0; "
           "   border-radius: 3px; "
           "   padding: 5px 15px; "
           "   min-width: 60px;"
           "}"
           "QPushButton:hover { background-color: #e0e0e0; }"
           "QPushButton:pressed { background-color: #d0d0d0; }";
}

void WT_ProjectWidget::onNewProjectClicked()
{
    if (m_isProjectOpen) {
        QString projName = QFileInfo(m_currentProjectFilePath).fileName();
        if(projName.isEmpty()) projName = "当前项目";

        QMessageBox msgBox;
        msgBox.setWindowTitle("操作受限");
        msgBox.setText(QString("项目 [%1] 正在运行中。\n为了数据安全，不能直接创建新项目。\n请先点击“关闭”按钮关闭当前项目。").arg(projName));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(getMessageBoxStyle());
        msgBox.exec();
        return;
    }

    NewProjectDialog* dialog = new NewProjectDialog(this);
    if (dialog->exec() == QDialog::Accepted) {
        ProjectData data = dialog->getProjectData();
        setProjectState(true, data.fullFilePath);
        emit projectOpened(true);
    }
    delete dialog;
}

void WT_ProjectWidget::onOpenProjectClicked()
{
    if (m_isProjectOpen) {
        QString projName = QFileInfo(m_currentProjectFilePath).fileName();
        if(projName.isEmpty()) projName = "当前项目";

        QMessageBox msgBox;
        msgBox.setWindowTitle("操作受限");
        msgBox.setText(QString("项目 [%1] 已经打开。\n不能同时打开多个项目。\n请先点击“关闭”按钮关闭当前项目。").arg(projName));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(getMessageBoxStyle());
        msgBox.exec();
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("打开项目"),
        "",
        tr("WellTest Project (*.pwt)")
        );

    if (filePath.isEmpty()) return;

    if (ModelParameter::instance()->loadProject(filePath)) {
        setProjectState(true, filePath);
        emit projectOpened(false);
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("错误");
        msgBox.setText("项目文件损坏或格式不正确，无法打开。");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(getMessageBoxStyle());
        msgBox.exec();
    }
}

void WT_ProjectWidget::onCloseProjectClicked()
{
    if (!m_isProjectOpen) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("提示");
        msgBox.setText("当前没有正在运行的项目，无法执行关闭操作。");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(getMessageBoxStyle());
        msgBox.exec();
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("关闭项目");
    msgBox.setText(QString("是否关闭当前项目 [%1]？").arg(QFileInfo(m_currentProjectFilePath).fileName()));
    msgBox.setInformativeText("关闭前建议保存数据。");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStyleSheet(getMessageBoxStyle());

    QPushButton *saveCloseBtn = msgBox.addButton("保存并关闭", QMessageBox::AcceptRole);
    QPushButton *directCloseBtn = msgBox.addButton("直接关闭", QMessageBox::DestructiveRole);
    msgBox.addButton("取消", QMessageBox::RejectRole);

    msgBox.setDefaultButton(saveCloseBtn);
    msgBox.exec();

    if (msgBox.clickedButton() == saveCloseBtn) {
        if (saveCurrentProject()) {
            closeProjectInternal();
        }
    } else if (msgBox.clickedButton() == directCloseBtn) {
        closeProjectInternal();
    }
}

/**
 * @brief 点击退出按钮事件：委托给主窗口的 closeEvent 统一处理
 */
void WT_ProjectWidget::onExitClicked()
{
    // 如果没有项目打开，直接关闭应用程序
    if (!m_isProjectOpen) {
        QApplication::quit();
        return;
    }

    // 委托给主界面的 closeEvent 统一处理安全退出拦截，防止两次弹窗冲突
    this->window()->close();
}

void WT_ProjectWidget::onLoadFileClicked()
{
    QString filter = tr("Excel Files (*.xlsx *.xls);;Text Files (*.txt);;All Files (*.*)");
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择要读取的数据文件"), QString(), filter);

    if (filePath.isEmpty()) return;

    QString fileType = "unknown";
    if (filePath.endsWith(".xlsx", Qt::CaseInsensitive) || filePath.endsWith(".xls", Qt::CaseInsensitive)) {
        fileType = "excel";
    } else if (filePath.endsWith(".txt", Qt::CaseInsensitive)) {
        fileType = "txt";
    }

    emit fileLoaded(filePath, fileType);

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("文件读取"));
    msgBox.setText(tr("文件已成功读取，正在准备显示数据..."));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(getMessageBoxStyle());
    msgBox.exec();
}

bool WT_ProjectWidget::saveCurrentProject()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ModelParameter::instance()->saveProject();
    QApplication::restoreOverrideCursor();
    return true;
}

void WT_ProjectWidget::closeProjectInternal()
{
    setProjectState(false, "");
    emit projectClosed();
}

