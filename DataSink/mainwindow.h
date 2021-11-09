#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <zmqthread.h>
#include <sddthread.h>
#include <controlthread.h>
#include <QVector>
#include <QDateTime>
#include <hdf5nexus.h>

typedef QVector<QVector<uint32_t>> imagepixeldata;
typedef QVector<uint32_t> spectrumdata;
typedef QVector<uint32_t> imagepreviewdata;
typedef QMap<std::string, QVector<uint32_t>> roidata;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void sendROI(std::string, std::string, imagepreviewdata);
    void addSpecData(int, imagepreviewdata);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void getImageData(int, std::string);
    void getCCDSettings(int, int);
    void showIncomingSpectrum(int, spectrumdata);
    void showBeamlineData(float, float, float);
    void writeScanIndexData(int, int, int);
    void writeLineBreakData(roidata, int, int, int);
    void getScanSettings(settingsdata);
    void getMetadata(metadata);
    void ccdReady();
    void sddReady();

private slots:
    void updateSTXMPreview(int, int, int pixnum);

    void addCCDDataChunk(int ccdX, int ccdY, int scanX, int scanY, uint32_t pxnum, uint32_t stxmpxval, std::string);

    void addSDDDataChunk(int32_t pxnum, spectrumdata);

    void addBeamlineDataChunk(float, float, float);

    void showEvent( QShowEvent* event );

private:
    Ui::MainWindow *ui;
    int currentimage = 10;
    QFile myfile;
    QFile myfluofile;
    zmqThread* th;
    zmqThread* ccd;
    sddThread* sdd;
    sddThread* sddth;
    hdf5nexus* nexusfile;
    controlThread* controlth;
    uint16_t counterx = 0;
    uint16_t countery = 1;
    uint16_t scancounterx = 0;
    uint16_t scancountery = 1;
    uint16_t xmax = 0;
    imagepreviewdata stxmpreviewvec;

    int savestarttime;
    int saveendtime;

    uint32_t stxmimage[200000];

    QString hdf5filename;

    int ccdX;
    int ccdY;

    int scanX;
    int scanY;

};
#endif // MAINWINDOW_H
