#ifndef GUI_H
#define GUI_H

#include <QMainWindow>
#include <scan.h>
#include <structs.h>

QT_BEGIN_NAMESPACE
namespace Ui { class GUI; }
QT_END_NAMESPACE

class GUI : public QMainWindow
{
    Q_OBJECT

public:
    GUI(QWidget *parent = nullptr);
    ~GUI();
    int scanX;
    int scanY;

    int ccdX;
    int ccdY;

    int sddChannels;

private slots:
    void on_actionExit_triggered();

    void on_cmdStartScan_clicked();

    void showDeviceStatus(QString, QString);
    void showPreview(std::string, std::string);

    void showROI(std::string, std::string);

    void on_cmdDeleteEnergy_clicked();

    void on_cmdAddEnergy_clicked();

    void on_ScanFinished();

    void on_cmdSaveScanNote_clicked();

    void on_cmdStopScan_clicked();

    void on_cmdPauseScan_clicked();

    void on_spbCCDWidth_valueChanged(int arg1);

    void on_spbCCDHeight_valueChanged(int arg1);

    void on_tabWidget_currentChanged(int index);

private:
    Ui::GUI *ui;
    scan* Scan;
};
#endif // GUI_H
