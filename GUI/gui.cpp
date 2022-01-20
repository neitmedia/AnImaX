#include "gui.h"
#include "ui_gui.h"
#include "math.h"
#include <QMessageBox>

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

    // disable start scan button, note input field and note save button
    ui->cmdStartScan->setDisabled(true);
    ui->txtScanNote->setDisabled(true);
    ui->cmdSaveScanNote->setDisabled(true);

    // get settings from UI and put them in a variable of type "settingsdata"
    settingsdata settings;

    settings.scanHeight = ui->spbScanHeight->value();
    settings.scanWidth = ui->spbScanWidth->value();
    settings.x_step_size = ui->dsbXStepSize->value();
    settings.y_step_size = ui->dsbYStepSize->value();
    settings.scantitle = ui->txtScanTitle->text().toStdString();

    settings.save_path = ui->txtFilePath->text().toStdString();
    settings.save_file = ui->txtFileName->text().toStdString();
    settings.file_compression = ui->chbEnableCompression->isChecked();
    settings.file_compression_level = ui->cmbCompressionLevel->currentIndex();

    // write scan width and height into global variables
    scanX = settings.scanWidth;
    scanY = settings.scanHeight;

    settings.energycount = ui->lstEnergies->count();
    settings.energies = (float*) malloc(ui->lstEnergies->count()*sizeof(float));
    for(int i = 0; i < ui->lstEnergies->count(); ++i)
    {
        QListWidgetItem* item = ui->lstEnergies->item(i);
        settings.energies[i] = item->text().toFloat();
    }
    settings.roidefinitions = ui->txtROIdefinitions->toPlainText().toStdString();

    if (ui->rdbNEXAFS->isChecked()) {
        settings.scantype = "NEXAFS";
    } else if (ui->rdbXRF->isChecked()) {
        settings.scantype = "XRF";
        settings.energycount = 1;
    }

    // network settings
    settings.datasinkIP = ui->txtDataSinkIP->text().toStdString();
    settings.datasinkPort = ui->spbDataSinkPort->value();

    settings.ccdIP = ui->txtCCDIP->text().toStdString();
    settings.ccdPort = ui->spbCDDPort->value();

    settings.sddIP = ui->txtSDDIP->text().toStdString();
    settings.sddPort = ui->spbSDDPort->value();

    settings.guiPort = ui->spbGUIPort->value();

    // ccd settings
    settings.ccd_binning_x = ui->spbCCDBinningX->value();
    settings.ccd_binning_y = ui->spbCCDBinningY->value();
    settings.ccd_height = ui->spbCCDHeight->value();
    settings.ccd_width = ui->spbCCDWidth->value();
    settings.ccd_pixelcount = ui->txtPixelCount->text().toInt();
    settings.ccd_frametransfer_mode = ui->spbCCDFTMode->value();
    settings.ccd_number_of_accumulations = ui->spbNumberOfAccumulations->value();
    settings.ccd_number_of_scans = ui->spbNumberOfScans->value();
    settings.ccd_set_kinetic_cycle_time = ui->dsbKineticCycleTime->value();
    settings.ccd_read_mode = ui->spbReadMode->value();
    settings.ccd_acquisition_mode = ui->spbAcquisitionMode->value();
    settings.ccd_shutter_mode = ui->spbShutterMode->value();
    settings.ccd_shutter_output_signal = ui->spbShutterOutputSignal->value();
    settings.ccd_shutter_open_time = ui->dsbShutterOpenTime->value();
    settings.ccd_shutter_close_time = ui->dsbShutterCloseTime->value();
    settings.ccd_triggermode = ui->spbTriggerMode->value();
    settings.ccd_exposure_time = ui->dsbExposureTime->value();
    settings.ccd_accumulation_time = ui->dsbAccumulationTime->value();
    settings.ccd_kinetic_time = ui->dsbKineticTime->value();
    settings.ccd_min_temp = ui->spbMinTemp->value();
    settings.ccd_max_temp = ui->spbMaxTemp->value();
    settings.ccd_target_temp = ui->spbTargetTemp->value();
    settings.ccd_pre_amp_gain = ui->spbPreAmpGain->value();
    settings.ccd_em_gain_mode = ui->spbEMGainMode->value();
    settings.ccd_em_gain = ui->spbEMGain->value();
    // write ccd width and height into global variables
    ccdX = settings.ccd_width;
    ccdY = settings.ccd_height;

    // sdd settings
    settings.sdd_sebitcount = ui->spbSebitcount->value();
    settings.sdd_filter = ui->cmbFilter->currentIndex();
    settings.sdd_energyrange = ui->cmbEnergyrange->currentIndex();
    settings.sdd_tempmode = ui->cmbTempmode->currentIndex();
    settings.sdd_zeropeakperiod = ui->spbZeroPeakPeriod->value();
    settings.sdd_checktemperature = ui->cmbCheckTemperature->currentIndex();

    if (ui->cmbAcquisionMode->currentIndex() == 0) {
        settings.sdd_acquisitionmode = 0;
    }

    if (ui->cmbAcquisionMode->currentIndex() == 1) {
        settings.sdd_acquisitionmode = 4;
    }

    settings.sdd_sdd1 = ui->chbSDD1->isChecked();
    settings.sdd_sdd2 = ui->chbSDD2->isChecked();
    settings.sdd_sdd3 = ui->chbSDD3->isChecked();
    settings.sdd_sdd4 = ui->chbSSD4->isChecked();

    // sample settings
    settings.sample_name = ui->txtSampleName->text().toStdString();
    settings.sample_type = ui->txtSampleType->text().toStdString();
    settings.sample_note = ui->txtSampleNote->toPlainText().toStdString();
    settings.sample_width = ui->dsbSampleWidth->value();
    settings.sample_height = ui->dsbSampleHeight->value();
    settings.sample_rotation_angle = ui->dsbSampleRotationAngle->value();

    // source settings
    settings.source_name = ui->txtSourceName->text().toStdString();
    settings.source_probe = ui->cmbSourceProbe->currentText().toStdString();
    settings.source_type = ui->cmbSourceType->currentText().toStdString();

    // additional settings
    settings.notes = ui->txtScanNote->toPlainText().toStdString();
    settings.userdata = ui->txtUserInfo->toPlainText().toStdString();

    // start "scan" thread
    Scan = new scan(settings);
    // connect thread-signal "sendDeviceStatusToGUI" to GUI-slot "showDeviceStatus" because we want to show current device status in GUI
    QObject::connect(Scan,SIGNAL(sendDeviceStatusToGUI(QString, QString)),this,SLOT(showDeviceStatus(QString, QString)));
    QObject::connect(Scan,SIGNAL(sendPreviewDataToGUI(std::string, std::string)),this,SLOT(showPreview(std::string, std::string)));
    QObject::connect(Scan,SIGNAL(sendROIDataToGUI(std::string, std::string)),this,SLOT(showROI(std::string, std::string)));
    QObject::connect(Scan,SIGNAL(sendScanFinished()),this,SLOT(on_ScanFinished()));

    Scan->start();
}

void GUI::showDeviceStatus(QString device, QString status) {
    if (device == "ccd") {
        if ((status == "connection ready") || (status == "detector ready")) {
            ui->chbCCD->setChecked(true);
        } else {
            ui->chbCCD->setChecked(false);
        }
    }
    if (device == "sdd") {
        if ((status == "connection ready") || (status == "detector ready")) {
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

        QPixmap scaledpixmp;

        if (scanX>=scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->ROIPPreview->width());
        }

        if (scanX<scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->ROIPPreview->height());
        }

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
        QPixmap scaledpixmp;

        if (scanX>=scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->stxmPreview->width());
        }

        if (scanX<scanY) {
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->stxmPreview->height());
        }

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

        ui->ccdPreview->setPixmap(pixmp);
    }

}


void GUI::on_cmdDeleteEnergy_clicked()
{
    // delete selected energy entries
    QList<QListWidgetItem*> items = ui->lstEnergies->selectedItems();

    foreach(QListWidgetItem * item, items)
    {
        delete ui->lstEnergies->takeItem(ui->lstEnergies->row(item));
    }
}


void GUI::on_cmdAddEnergy_clicked()
{
    ui->lstEnergies->addItem(QString::number(ui->spbNewEnergy->value()));
}

void GUI::on_ScanFinished()
{
    ui->txtScanNote->setDisabled(false);
    ui->cmdSaveScanNote->setDisabled(false);
    // give info message
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText("Scan finished!");
    msgBox.exec();
}

void GUI::on_cmdSaveScanNote_clicked()
{
    // get scan note from GUI and write it into thread variable
    Scan->scannote = ui->txtScanNote->toPlainText().toStdString();
}

void GUI::on_cmdStopScan_clicked()
{
    Scan->stopscan = true;
}


void GUI::on_cmdPauseScan_clicked()
{
    if (ui->cmdPauseScan->text() == "Pause") {
        Scan->pausescan = true;
        ui->cmdPauseScan->setText("Resume");
    } else {
        Scan->resumescan = true;
        ui->cmdPauseScan->setText("Pause");
    }
}


void GUI::on_spbCCDWidth_valueChanged(int arg1)
{
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}


void GUI::on_spbCCDHeight_valueChanged(int arg1)
{
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}


void GUI::on_tabWidget_currentChanged(int index)
{
    ui->spbNumberOfScans->setValue(ui->spbScanWidth->value()*ui->spbScanHeight->value());
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}

