/*
 * 文件名: wt_projectwidget.h
 * 文件作用: 项目管理界面类头文件
 * 功能描述:
 * 1. 提供“新建”、“打开”、“关闭”、“退出”等核心项目管理入口交互。
 * 2. 负责通知 Mainwindow 系统状态的更替。
 */

#ifndef WT_PROJECTWIDGET_H
#define WT_PROJECTWIDGET_H

#include <QWidget>
#include <QString>

namespace Ui {
class WT_ProjectWidget;
}

class WT_ProjectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WT_ProjectWidget(QWidget *parent = nullptr);
    ~WT_ProjectWidget();

signals:
    void projectOpened(bool isNew);
    void projectClosed();
    void fileLoaded(const QString& filePath, const QString& fileType);

private slots:
    void onNewProjectClicked();
    void onOpenProjectClicked();
    void onCloseProjectClicked();
    void onExitClicked();
    void onLoadFileClicked();

private:
    Ui::WT_ProjectWidget *ui;

    bool m_isProjectOpen;
    QString m_currentProjectFilePath;

    void init();
    void setProjectState(bool isOpen, const QString& filePath);
    bool saveCurrentProject();
    void closeProjectInternal();

    QString getMessageBoxStyle() const;
};

#endif // WT_PROJECTWIDGET_H
