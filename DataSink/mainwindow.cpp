#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QFileDevice>
#include <QImage>
#include <iostream>
#include <string>
#include <math.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <random>
#include <structs.h>
#include <QDateTime>
#include <QSettings>
#include <QListWidgetItem>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

#include "H5Cpp.h"
using namespace H5;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // register Metatypes to be able to use them with signals/slots
    qRegisterMetaType<imagepixeldata>( "imagepixeldata" );
    qRegisterMetaType<spectrumdata>( "spectrumdata" );
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<metadata>( "metadata" );
    qRegisterMetaType<std::string>();
    qRegisterMetaType<imagepreviewdata>( "imagepreviewdata" );
    qRegisterMetaType<roidata>( "roidata" );

    // create filename of first file
    hdf5filename = "measurement_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent( QShowEvent* event ) {
    QWidget::showEvent( event );
    // get IP settings from ini file
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");

    settings.beginGroup("Network");

    QString guiIP = settings.value("guiIP").toString();
    QString guiPort = settings.value("guiPort").toString();

    settings.endGroup();

    // add log item
    addLogItem("read network settings from file "+settings.fileName());
    addLogItem("GUI - "+guiIP+":"+guiPort);

    if ((guiIP == "") || (guiPort == "")) {
        // write standard IP settings to ini file
        guiIP = "127.0.0.1";
        guiPort = "5555";
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");
        settings.beginGroup("Network");
        settings.setValue("guiIP", guiIP);
        settings.setValue("guiPort", guiPort);
        settings.endGroup();
    }

    // set log filename
    QString logpath = QDir::currentPath();
    qint64 qiTimestamp=QDateTime::currentMSecsSinceEpoch();
    QDateTime dt;
    dt.setTime_t(qiTimestamp/1000);
    QString datestring = dt.toString("yyyy-MM-dd-hh-mm-ss");
    QString logfile = "measurement_"+datestring+".log";

    QString logfilepath = logpath+"/"+logfile;

    ui->txtLogFileName->setText(logfilepath);

    // launch control thread
    controlth = new controlThread(guiIP+':'+guiPort);
    const bool connected = connect(controlth, SIGNAL(sendSettingsToGUI(settingsdata)),this,SLOT(getScanSettings(settingsdata)));
    const bool connected2 = connect(controlth, SIGNAL(sendMetadataToGUI(metadata)),this,SLOT(getMetadata(metadata)));
    controlth->start();

    // add log item
    addLogItem("launched control thread");
}

void MainWindow::addLogItem(QString logtext) {
    qint64 qiTimestamp=QDateTime::currentMSecsSinceEpoch();
    QDateTime dt;
    dt.setTime_t(qiTimestamp/1000);
    QString datestring = dt.toString("yyyy-MM-dd hh:mm:ss");

    new QListWidgetItem(datestring+": "+logtext, ui->lstStatusLog);

    ui->lstStatusLog->scrollToBottom();

    // if "save to file"-checkbox is checked, save log entry to file
    if (ui->chbSaveLogToFile->isChecked()) {
        QString filename = ui->txtLogFileName->text();
        QFile file(filename);
        if (file.open(QIODevice::ReadWrite | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << datestring+": "+logtext << Qt::endl;
        }
    }
}

void MainWindow::getMetadata(metadata metadata) {
    currentmetadata = metadata;

    if (ui->chbSaveData->isChecked()) {
        nexusfile->writeMetadata(metadata);
    }

    // add log item
    addLogItem("received metadata");
}

void MainWindow::getScanSettings(settingsdata settings) {
    scansettings = settings;

    ccdX = scansettings.ccdWidth;
    ccdY = scansettings.ccdHeight;

    scanX = scansettings.scanWidth;
    scanY = scansettings.scanHeight;

    // allocate memory for STXM transmission preview image
    stxmimage = (uint32_t*) malloc(scanX*scanY*sizeof(uint32_t));

    // make sure stxmimage contains only zeros
    for (int i=0;i<=scanX*scanY;i++) {
        stxmimage[i] = 0;
    }

    std::cout<<"ccd ip:port "<<scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)<<std::endl;
    std::cout<<"sdd ip:port "<<scansettings.sddIP+':'+std::to_string(scansettings.sddPort)<<std::endl;

    // create HDF5/NeXus file
    hdf5filename = "measurement_testmessung_1_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
    nexusfile = new hdf5nexus();
    nexusfile->createDataFile(hdf5filename, scansettings);
    ui->lblFilename->setText("current file: "+hdf5filename);
    addLogItem("created file "+hdf5filename);

    // prepare ROI preview labels
    QJsonParseError jsonError;
    QString roijson = QString::fromStdString(scansettings.roidefinitions);
    QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);
    QJsonObject jsonObj = document.object();
    foreach(QString key, jsonObj.keys()) {
        ui->cmbROISelect->addItem(key);
    }

    // start ccd thread
    ccd = new zmqThread(QString::fromStdString(scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)), scanX, scanY);
    connect(ccd, SIGNAL(ccdReady()),this,SLOT(ccdReady()));

    const bool connected = connect(ccd, SIGNAL(sendImageData(int, std::string)),this,SLOT(getImageData(int, std::string)));
    if (connected) {
        std::cout<<"getImageData connected"<<std::endl;
    } else {
        std::cout<<"getImageData not connected"<<std::endl;
    }

    const bool connected1 = connect(ccd, SIGNAL(sendCCDSettings(int, int)),this,SLOT(getCCDSettings(int, int)));
    if (connected1) {
        std::cout<<"getCCDSettings connected"<<std::endl;
    } else {
        std::cout<<"getCCDSettings not connected"<<std::endl;
    }

    sdd = new sddThread(QString::fromStdString(scansettings.sddIP+':'+std::to_string(scansettings.sddPort)), QString::fromStdString(scansettings.roidefinitions), scanX, scanY);
    connect(sdd, SIGNAL(sddReady()),this,SLOT(sddReady()));

    const bool connected2 = connect(sdd, SIGNAL(sendScanIndexDataToGUI(int, int, int)),this,SLOT(writeScanIndexData(int, int, int)));
    if (connected2) {
        std::cout<<"writeScanIndexData connected"<<std::endl;
    } else {
        std::cout<<"writeScanIndexData not connected"<<std::endl;
    }

    const bool connected3 = connect(sdd, SIGNAL(sendLineBreakDataToGUI(roidata, int, int, int)),this,SLOT(writeLineBreakData(roidata, int, int, int)));
    if (connected3) {
        std::cout<<"writeLineBreakData connected"<<std::endl;
    } else {
        std::cout<<"writeLineBreakData not connected"<<std::endl;
    }

    const bool connected4 = connect(sdd, SIGNAL(sendSpectrumDataToGUI(int, spectrumdata)),this,SLOT(showIncomingSpectrum(int, spectrumdata)));
    if (connected4) {
        std::cout<<"showIncomingSpectrum connected"<<std::endl;
    } else {
        std::cout<<"showIncomingSpectrum not connected"<<std::endl;
    }

    // start threads
    ccd->start();
    sdd->start();

    // tell control thread that it should listen for metadata
    controlth->waitForMetadata = true;

    // add log item
    addLogItem("received scan settings");
}

void MainWindow::ccdReady() {
    std::cout<<"GUI knows that CCD is ready"<<std::endl;
    controlth->ccdReady = true;
    // add log item
    addLogItem("ccd is ready");
}

void MainWindow::sddReady() {
    std::cout<<"GUI knows that SDD is ready"<<std::endl;
    controlth->sddReady = true;
    // add log item
    addLogItem("sdd is ready");
}

void MainWindow::updateSTXMPreview(int scanX, int scanY, int pixnum) {
       u_int32_t max = 0;
       u_int32_t min = 0xFFFFFFFF;

       int count = 0;
       for (int j = 0; j < scanY; ++j) {
         for (int i = 0; i < scanX; ++i) {
             if (count <= pixnum) {
                 uint32_t dummy = stxmimage[i + (j * scanX)];
                 if (dummy > max) {
                     max = dummy;
                 }
                 if (dummy < min) {
                     min = dummy;
                 }
             }
             count++;
         }
       }

       double factor =(double)65535/(max-min);

       QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);
       count = 0;
       for (int j = 0; j < scanY; ++j) {
         quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());
         for (int i = 0; i < scanX; ++i) {
           dst[i] = floor((stxmimage[i + j * scanX]-min)*factor);
         count++;
         }
       }

       QPixmap scaledpixmp;
       if (scanX>=scanY) {
         scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->imagepreview_2->width());
       }

       if (scanX<scanY) {
         scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->imagepreview_2->height());
       }

       ui->imagepreview_2->setPixmap(scaledpixmp);
}

void MainWindow::getImageData(int cntx, std::string datax) {
     uint16_t max = 0;
     uint16_t min = 65535;

     uint16_t savedata[1][ccdX][ccdY];
     uint32_t ccdpixelcount = ccdX*ccdY;
     uint32_t ccddatabytecount = ccdpixelcount * 2;
     uint16_t imagex[ccdpixelcount];

     uint32_t sum = 0;
     int counterx = 0;
     int x = 0;
     int y = 0;
     for (uint32_t i = 0; i < ccddatabytecount; i=i+2) {
             uint16_t value = ((uint16_t) (uint8_t)datax[i+1] << 8) | (uint8_t)datax[i];

             if (value > max) {
                 max = value;
             }

             if (value < min) {
                 min = value;
             }

             imagex[counterx] = value;

             if (((counterx-1)%ccdX == 0) && (counterx > 0)) {
                 x = 0;
                 y++;
             } else {
                 if (counterx > 0) {
                    x++;
                 }
             }

             savedata[0][x][y] = value;

             sum = sum + value;

             counterx++;
     }

        double factor = 65535/(max-min);

        if (ui->chbPreview->isChecked()) {
            if (((cntx+1)%scanX) == 0) {

                 ui->lblStatus->setText("image #"+QString::number(cntx)+" received");

                 QImage image = QImage(ccdX, ccdY, QImage::Format_Grayscale16);
                 for (int j = 0; j < ccdY; ++j)
                 {
                    quint16 *dst =  reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());
                    for (int i = 0; i < ccdX; ++i)
                    {
                         unsigned short pixelval =  static_cast<unsigned short>(imagex[i + j * ccdX]);
                         dst[i] = (pixelval-min)*factor;
                    }
                 }

                 QPixmap pixmp = QPixmap::fromImage(image);

                 ui->imagepreview->setPixmap(pixmp);
             }
        }

        stxmimage[cntx] = sum;
        stxmpreviewvec.append(sum);

        if (ui->chbSaveData->isChecked()) {
            // write data to file
            hsize_t size[3];
            size[1] = ccdX;
            size[2] = ccdY;

            hsize_t offset[3];
            offset[1] = 0;
            offset[2] = 0;

            long pixelvalue[1];
            pixelvalue[0] = 0;


            // WRITE DETECTOR DATA TO FILE
            size[0] = cntx+1;
            offset[0] = cntx;

            hsize_t dimsext[3] = {1, (hsize_t)ccdX, (hsize_t)ccdY}; // extend dimensions

            DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/ccd/data"));

            dataset->extend(size);

            // Select a hyperslab in extended portion of the dataset.
            DataSpace *filespace = new DataSpace(dataset->getSpace());

            filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

            // Define memory space.
            DataSpace *memspace = new DataSpace(3, dimsext, NULL);

            // Write data to the extended portion of the dataset.
            dataset->write(savedata, PredType::STD_U16LE, *memspace, *filespace);

            // PLOTTABLE SUM IMAGE

            hsize_t dimsextsumimage[2] = {1, 1}; // extend dimensions

            hsize_t sizesumimage[2];
            hsize_t offsetsumimage[2];

            if (scancounterx < scanX) {
                    scancounterx++;
                } else {
                    scancounterx = 1;
                    xmax = scanX;
                    scancountery++;
                }

            sizesumimage[0] = scancountery;
            if (xmax > 0) {
                sizesumimage[1] = xmax;
            } else {
                sizesumimage[1] = scancounterx;
            }
            offsetsumimage[0] = scancountery-1;
            offsetsumimage[1] = scancounterx-1;

            DataSet *datasetsumimage = new DataSet(nexusfile->file->openDataSet("/measurement/data/data"));

            datasetsumimage->extend(sizesumimage);

            // Select a hyperslab in extended portion of the dataset.
            DataSpace *filespacesumimage = new DataSpace(datasetsumimage->getSpace());

            filespacesumimage->selectHyperslab(H5S_SELECT_SET, dimsextsumimage, offsetsumimage);

            // Define memory space.
            DataSpace *memspacesumimage = new DataSpace(2, dimsextsumimage, NULL);

            uint64_t blubb[1];
            blubb[0] = stxmimage[cntx];

            // Write data to the extended portion of the dataset.
            datasetsumimage->write(blubb, PredType::STD_I64LE, *memspacesumimage, *filespacesumimage);

            dataset->close();
            delete dataset;
            filespace->close();
            delete filespace;
            memspace->close();
            delete memspace;
            datasetsumimage->close();
            delete datasetsumimage;
            filespacesumimage->close();
            delete filespacesumimage;
            memspacesumimage->close();
            delete memspacesumimage;

        }

        if (((cntx+1)%scanX) == 0) {
            if (ui->chbPreview->isChecked()) {
                updateSTXMPreview(scanX, scanY, cntx);
            }
            controlth->setCurrentSTXMPreview(stxmpreviewvec);
            controlth->setCurrentCCDImage(datax);
    }

    if (cntx == (scanX*scanY)-1) {
        ccdReceived = true;
        // add log item
        addLogItem("fully received ccd data");
        checkIfScanIsFinished();
    }
}

void MainWindow::showIncomingSpectrum(int cnt, spectrumdata blubb) {
    if (ui->chbSaveData->isChecked()) {
        nexusfile->writeSDDData(cnt, blubb);
    }
    emit addSpecData(cnt, blubb);
}

void MainWindow::writeScanIndexData(int dataindex, int nopx, int stopx) {
    if (ui->chbSaveData->isChecked()) {
        nexusfile->writeScanIndexData(dataindex, nopx, stopx);
    }
}

void MainWindow::writeLineBreakData(roidata ROImap, int dataindex, int nopx, int stopx) {
    controlth->setCurrentROIs(ROImap);

    if (ui->chbSaveData->isChecked()) {
        nexusfile->writeLineBreakDataAndROIs(ROImap, dataindex, nopx, stopx, scanX, scanY);
    }

    if (nopx == scanX*scanY) {
        sddReceived = true;
        checkIfScanIsFinished();
        addLogItem("fully received sdd data");
    }

    // if preview checkbox is checked, show ROI preview
    if (ui->chbPreview->isChecked()) {
        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        unsigned long count = 0;

        unsigned long pixelcount = scanX*scanY;
        unsigned long bytecount = pixelcount*sizeof(uint32_t);

        std::string ROIelement = ui->cmbROISelect->currentText().toStdString();

        uint32_t previewimage[pixelcount];

        QVector<uint32_t> roidata = ROImap[ROIelement];

        for (unsigned long i = 0; i < pixelcount; i++) {
            if ((i+3) < (unsigned long) roidata.length()) {
                uint32_t pixelvalue = roidata.at(i);

                if (pixelvalue > max) {
                    max = pixelvalue;
                }
                if (pixelvalue < min) {
                    min = pixelvalue;
                }

                previewimage[count] = pixelvalue;
            } else {
                previewimage[count] = 0;
            }
            count++;
        }

        double factor =(double)65535/(max-min);

        QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);
        count = 0;
        for (int j = 0; j < scanY; ++j) {
          quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());
          for (int i = 0; i < scanX; ++i) {
            dst[i] = floor((previewimage[i + j * scanX]-min)*factor);
            std::cout<<"roi preview debug: "<<dst[10]<<std::endl;
            count++;
          }
        }

        QPixmap scaledpixmp;

        if (scanX>=scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->imagepreview->width());
        }

        if (scanX<scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->imagepreview->height());
        }

        ui->lblROIPreview->setPixmap(scaledpixmp);

    }
}

void MainWindow::getCCDSettings(int width, int height) {
    std::cout<<"real ccd width: "<<width;
    std::cout<<"real ccd height: "<<height;
    nexusfile->writeCCDSettings(width, height);
}

void MainWindow::checkIfScanIsFinished() {
    if ((sddReceived) && (ccdReceived)) {
        nexusfile->closeDataFile();
        addLogItem("closed file "+hdf5filename);
        sddReceived = false;
        ccdReceived = false;

        std::cout<<"closed file '"<<hdf5filename.toStdString()<<"'"<<std::endl;

        int currentScanNumber = currentmetadata.acquisition_number;

        ui->lblStatus->setText("scan "+QString::number(currentScanNumber)+" finished");
        addLogItem("scan "+QString::number(currentScanNumber)+" finished");

        if ((scansettings.scantype == "NEXAFS") && (currentmetadata.acquisition_number < scansettings.energycount)) {

            // clear stxm transmission preview
            for (int i=0;i<=scanX*scanY;i++) {
                stxmimage[i] = 0;
            }

            // incremement acquisition number
            currentmetadata.acquisition_number++;
            hdf5filename = "measurement_testmessung_"+QString::number(currentmetadata.acquisition_number)+"_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";

            nexusfile = new hdf5nexus();
            nexusfile->createDataFile(hdf5filename, scansettings);

            addLogItem("created file "+hdf5filename);

            ui->lblFilename->setText("current file: "+hdf5filename);
            std::cout<<"created datafile "<<hdf5filename.toStdString()<<std::endl;

            std::cout<<"scan part is ready"<<std::endl;

            // clear variables
            counterx = 0;
            countery = 1;
            scancounterx = 0;
            scancountery = 1;
            xmax = 0;
            stxmpreviewvec.clear();

            // tell control thread to send finish signal
            controlth->partScanFinished = true;
            controlth->waitForMetadata = true;
        } else {
            // stop communication threads
            controlth->stop = true;
            sdd->stop = true;
            ccd->stop = true;
        }
    }
}

void MainWindow::on_actionClose_triggered()
{
    qApp->closeAllWindows();
}


void MainWindow::on_cmdSelectLogFile_clicked()
{
    QString logFileName = QFileDialog::getSaveFileName(this, tr("Save log file"), "/", tr("Log Files (*.log)"));
    ui->txtLogFileName->setText(logFileName);
}

