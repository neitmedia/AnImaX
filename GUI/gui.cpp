#include "gui.h"
#include "ui_gui.h"
#include "math.h"

GUI::GUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GUI)
{
    ui->setupUi(this);

    // register "settingsdata" and std::string as metatype because we want to use variables of "settingsdata" and std::string type as arguments
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<std::string>();
}

GUI::~GUI()
{
    delete ui;
}


void GUI::on_actionExit_triggered()
{
    qApp->quit();
}


void GUI::on_cmdStartScan_clicked()
{
    /* StartScan function functionality:
       1) publish settings (settings protobuf)
       2) wait for data sink, ccd, sdd ready signals
       3) get parameters from beamline etc.
       4) publish parameters (metadata protobuf)
       5) start scan
    */

    // get settings from UI and put them in a variable of type "settingsdata"
    settingsdata settings;
    settings.scanHeight = ui->spbScanHeight->value();
    settings.scanWidth = ui->spbScanWidth->value();
    settings.ccdHeight = ui->spbCCDHeight->value();
    settings.ccdWidth = ui->spbCCDWidth->value();
    settings.sddChannels = ui->spbSDDChannels->value();
    settings.roidefinitions = ui->txtROIdefinitions->toPlainText().toStdString();

    if (ui->rdbNEXAFS->isChecked()) {
        settings.scantype = "NEXAFS";
    } else if (ui->rdbXRF->isChecked()) {
        settings.scantype = "XRF";
    }

    // set global variables to values
    scanX = settings.scanWidth;
    scanY = settings.scanHeight;

    ccdX = settings.ccdWidth;
    ccdY = settings.ccdHeight;

    sddChannels = settings.sddChannels;

    // start "scan" thread
    Scan = new scan(settings);
    // connect thread-signal "sendDeviceStatusToGUI" to GUI-slot "showDeviceStatus" because we want to show current device status in GUI
    QObject::connect(Scan,SIGNAL(sendDeviceStatusToGUI(QString, QString)),this,SLOT(showDeviceStatus(QString, QString)));
    QObject::connect(Scan,SIGNAL(sendPreviewDataToGUI(std::string, std::string)),this,SLOT(showPreview(std::string, std::string)));
    QObject::connect(Scan,SIGNAL(sendROIDataToGUI(std::string, std::string)),this,SLOT(showROI(std::string, std::string)));

    Scan->start();
}

void GUI::showDeviceStatus(QString device, QString status) {
    if (device == "ccd") {
        if (status == "ready") {
            ui->chbCCD->setChecked(true);
        } else {
            ui->chbCCD->setChecked(false);
        }
    }
    if (device == "sdd") {
        if (status == "ready") {
            ui->chbSDD->setChecked(true);
        } else {
            ui->chbSDD->setChecked(false);
        }
    }
    if (device == "datasink") {
        if (status == "ready") {
            ui->chbDataSink->setChecked(true);
        } else {
            ui->chbDataSink->setChecked(false);
        }
    }
}

void GUI::showROI(std::string element, std::string previewdata) {
        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        unsigned long count = 0;

        unsigned long pixelcount = scanX*scanY;
        unsigned long bytecount = pixelcount*4;

        uint32_t previewimage[pixelcount];

        for (unsigned long i = 0; i < bytecount; i=i+4) {
            if ((i+3) < previewdata.length()) {
                uint32_t pixelvalue = ((uint32_t)(previewdata[i+3]<<24)) | (uint8_t)previewdata[i+2]<<16 | (uint8_t)previewdata[i+1]<<8 | (uint8_t)previewdata[i];

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
            count++;
          }
        }

        QPixmap pixmp = QPixmap::fromImage(image);
        QPixmap scaledpixmp = pixmp.scaledToHeight(ui->ROIPPreview->height());
        if (element == "P") {
            ui->ROIPPreview->setPixmap(scaledpixmp);
        } else if (element == "O") {
            ui->ROIOPreview->setPixmap(scaledpixmp);
        } else if (element == "C") {
            ui->ROICPreview->setPixmap(scaledpixmp);
        } else if (element == "Zero") {
            ui->ROIZeroPreview->setPixmap(scaledpixmp);
        } else if (element == "Fe") {
            ui->ROIFePreview->setPixmap(scaledpixmp);
        }
}

void GUI::showPreview(std::string previewtype, std::string previewdata) {

    if (previewtype == "stxm") {

        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        unsigned long count = 0;

        unsigned long pixelcount = scanX*scanY;
        unsigned long bytecount = pixelcount*4;

        uint32_t previewimage[pixelcount];

        for (unsigned long i = 0; i < bytecount; i=i+4) {
            if ((i+3) < previewdata.length()) {
                uint32_t pixelvalue = ((uint32_t)(previewdata[i+3]<<24)) | (uint8_t)previewdata[i+2]<<16 | (uint8_t)previewdata[i+1]<<8 | (uint8_t)previewdata[i];

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
            count++;
          }
        }

        QPixmap pixmp = QPixmap::fromImage(image);
        QPixmap scaledpixmp = pixmp.scaledToHeight(ui->stxmPreview->height());
        ui->stxmPreview->setPixmap(scaledpixmp);
    }

    if (previewtype == "ccd") {

        uint16_t max = 0;
        uint16_t min = 65535;

        uint32_t ccdpixelcount = ccdX*ccdY;
        uint32_t ccddatabytecount = ccdpixelcount * 2;
        uint32_t imagex[ccdpixelcount];

        uint32_t sum = 0;
        int counterx = 0;

        for (int i = 0; i < ccddatabytecount; i=i+2) {
                uint16_t value = ((uint16_t) (uint8_t)previewdata[i+1] << 8) | (uint8_t)previewdata[i];

                if (value > max) {
                    max = value;
                }

                if (value < min) {
                    min = value;
                }

                imagex[counterx] = value;

                sum = sum + value;

                counterx++;
        }
        double factor = 1;

        if (max != min) {
          factor = abs(65535/(max-min));
        }
                QImage image = QImage(ccdX, ccdY, QImage::Format_Grayscale16);
                for (int j = 0; j < ccdY; ++j)
                {
                   quint16 *dst =  reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());
                   for (int i = 0; i < ccdX; ++i)
                   {
                        unsigned short pixelval = static_cast<unsigned short>(imagex[i + j * ccdX]);
                        dst[i] = (pixelval-min)*factor;
                   }
                }

        QPixmap pixmp = QPixmap::fromImage(image);
        QPixmap scaledpixmp = pixmp.scaledToHeight(ui->ccdPreview->height());
        ui->ccdPreview->setPixmap(scaledpixmp);
    }

}

