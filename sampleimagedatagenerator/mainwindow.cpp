#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include <stdio.h>
#include <QImageReader>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_cmdGenImage_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, ("Save File"), "/home", ("Scan file (*.bin)"));

    int scanX = ui->spbScanX->value();
    int scanY = ui->spbScanY->value();

    int ccdX = ui->spbCCDX->value();
    int ccdY = ui->spbCCDY->value();

    QImage scaled = newImage.scaled(scanX,scanY,Qt::IgnoreAspectRatio);

    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    for (int j=0;j<scanY;j++) {
        for (int i=0;i<scanX;i++) {

            int yrand = QRandomGenerator::global()->bounded(ccdY);
            int xrand = QRandomGenerator::global()->bounded(ccdX);

            uint16_t pixelvalue = (uint16_t) ~((unsigned int)qGray(scaled.pixel(i,j)));
            for (int k=0;k<ccdY;k++) {
                for (int l=0;l<ccdX;l++) {
                    if ((k == yrand) && (l == xrand)) {
                        out << (uint16_t)pixelvalue;
                    } else {
                        out << (uint16_t)0;
                    }
                }
            }
        }
    }

   file.close();

   ui->statusbar->showMessage(QString("file ")+fileName+QString(" written"));
}


void MainWindow::on_cmdOpenImage_clicked()
{

    QString fileName = QFileDialog::getOpenFileName(this, ("Open File"), "/home", ("Images (*.png *.xpm *.jpg *.bmp)"));

    ui->statusbar->showMessage(QString("file ")+fileName+QString(" opened"));

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage dummyImage = reader.read();

    newImage = dummyImage.convertToFormat(QImage::Format_Grayscale8);

    int scanX = ui->spbScanX->value();
    int scanY = ui->spbScanY->value();

    QImage QImage8Bit = dummyImage.convertToFormat(QImage::Format_Grayscale8);
    QImage scaled = QImage8Bit.scaled(scanX,scanY,Qt::IgnoreAspectRatio);

    QPixmap pix = QPixmap::fromImage(scaled);
    ui->imagePreview->setPixmap(pix);

   /* QPainter painter(&printer);
            QPixmap pixmap = imageLabel->pixmap(Qt::ReturnByValue);
            QRect rect = painter.viewport();
            QSize size = pixmap.size();

            size.scale(rect.size(), Qt::KeepAspectRatio);
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(pixmap.rect());
            painter.drawPixmap(0, 0, pixmap);*/
}

void MainWindow::resizeImage() {
    int scanX = ui->spbScanX->value();
    int scanY = ui->spbScanY->value();

    QImage QImage8Bit = newImage.convertToFormat(QImage::Format_Grayscale8);
    QImage scaled = QImage8Bit.scaled(scanX,scanY,Qt::IgnoreAspectRatio);

    QPixmap pix = QPixmap::fromImage(scaled);
    ui->imagePreview->setPixmap(pix);
}


void MainWindow::on_spbScanX_valueChanged(int arg1)
{

    int scanX = ui->spbScanX->value();
    int scanY = ui->spbScanY->value();
    ui->imagePreview->setGeometry(190,10,scanX,scanY);
    resizeImage();
}


void MainWindow::on_spbScanY_valueChanged(int arg1)
{
    int scanX = ui->spbScanX->value();
    int scanY = ui->spbScanY->value();
    ui->imagePreview->setGeometry(190,10,scanX,scanY);
    resizeImage();
}


void MainWindow::on_actionBeenden_triggered()
{
    qApp->closeAllWindows();
}

