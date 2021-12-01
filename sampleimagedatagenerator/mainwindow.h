#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_cmdGenImage_clicked();

    void on_cmdOpenImage_clicked();

    void resizeImage();

    void on_spbScanX_valueChanged(int arg1);

    void on_spbScanY_valueChanged(int arg1);

    void on_actionBeenden_triggered();

private:
    Ui::MainWindow *ui;

    QImage newImage;
};
#endif // MAINWINDOW_H
