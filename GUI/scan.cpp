#include "scan.h"
#include <QDebug>
#include <stdio.h>
#include <QThread>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <QDate>
#include <QTime>

#include <zmq.hpp>

scan::scan(settingsdata s): settings(s)
{
    qRegisterMetaType<std::string>();
}

void scan::run()
{
        //  Prepare publisher
        zmq::context_t ctx(1);
        zmq::socket_t publisher(ctx, zmq::socket_type::pub);
        publisher.bind("tcp://*:"+std::to_string(settings.guiPort));

        // Prepare ccd subscriber => open "statusdata" envelopes from ccd PC
        zmq::socket_t ccd(ctx, zmq::socket_type::sub);
        ccd.connect("tcp://"+settings.ccdIP+":"+std::to_string(settings.ccdPort));
        std::cout<<"connected to ccd: "<<"tcp://"+settings.ccdIP+":"+std::to_string(settings.ccdPort)<<std::endl;
        ccd.set(zmq::sockopt::subscribe, "statusdata");

        // Prepare sdd subscriber => open "statusdata" envelopes from sdd PC
        zmq::socket_t sdd(ctx, zmq::socket_type::sub);
        sdd.connect("tcp://"+settings.sddIP+":"+std::to_string(settings.sddPort));
        std::cout<<"connected to sdd: "<<"tcp://"+settings.sddIP+":"+std::to_string(settings.sddPort)<<std::endl;
        sdd.set(zmq::sockopt::subscribe, "statusdata");

        // Prepare datasink subscriber => open "statusdata", "previewdata" and "roidata" envelopes from datasink PC
        zmq::socket_t datasink(ctx, zmq::socket_type::sub);
        datasink.connect("tcp://"+settings.datasinkIP+":"+std::to_string(settings.datasinkPort));
        std::cout<<"connected to sdd: "<<"tcp://"+settings.datasinkIP+":"+std::to_string(settings.datasinkPort)<<std::endl;
        datasink.set(zmq::sockopt::subscribe, "statusdata");
        datasink.set(zmq::sockopt::subscribe, "previewdata");
        datasink.set(zmq::sockopt::subscribe, "roidata");
        datasink.set(zmq::sockopt::subscribe, "scanstatus");

        // prepare "Measurement"-protobuf
        animax::Measurement Measurement;

        /* BEGIN SET PROTOBUF VALUES */
        // general scan settings
        Measurement.set_width(settings.scanWidth);
        Measurement.set_height(settings.scanHeight);
        Measurement.set_acquisition_time(10);
        Measurement.set_energy_count(settings.energycount);
        for (int i=0;i<settings.energycount;i++) {
            Measurement.add_energies(settings.energies[i]);
        }
        Measurement.set_roidefinitions(settings.roidefinitions);
        Measurement.set_scantype(settings.scantype);
        Measurement.set_save_path(settings.save_path);
        Measurement.set_save_file(settings.save_file);

        // network settings
        Measurement.set_ccdip(settings.ccdIP);
        Measurement.set_ccdport(settings.ccdPort);
        Measurement.set_sddip(settings.sddIP);
        Measurement.set_sddport(settings.sddPort);
        Measurement.set_datasinkip(settings.datasinkIP);
        Measurement.set_datasinkport(settings.datasinkPort);

        // ccd settings
        Measurement.set_ccdheight(settings.ccdHeight);
        Measurement.set_ccdwidth(settings.ccdWidth);

        // sdd settings
        Measurement.set_sebitcount(settings.sebitcount);
        Measurement.set_filter(settings.filter);
        Measurement.set_energyrange(settings.energyrange);
        Measurement.set_tempmode(settings.tempmode);
        Measurement.set_zeropeakperiod(settings.zeropeakperiod);
        Measurement.set_acquisitionmode(settings.acquisitionmode);
        Measurement.set_checktemperature(settings.checktemperature);
        Measurement.set_sdd1(settings.sdd1);
        Measurement.set_sdd2(settings.sdd2);
        Measurement.set_sdd3(settings.sdd3);
        Measurement.set_sdd4(settings.sdd4);

        // sample settings
        Measurement.set_sample_name(settings.sample_name);
        Measurement.set_sample_type(settings.sample_type);
        Measurement.set_sample_note(settings.sample_note);
        Measurement.set_sample_width(settings.sample_width);
        Measurement.set_sample_height(settings.sample_height);
        Measurement.set_sample_rotation_angle(settings.sample_rotation_angle);

        // additional settings
        Measurement.set_notes(settings.notes);
        Measurement.set_userdata(settings.userdata);
        /* END SET PROTOBUF VALUES */

        bool ccd_connection_ready = false;
        bool ccd_detector_ready = false;
        bool sdd_connection_ready = false;
        bool sdd_detector_ready = false;
        bool datasink_ready = false;
        bool metadata_sent = false;

        while (true) {

            // send settings and make sure all hosts and peripherals / devices are connected
            while ((!ccd_connection_ready) || (!sdd_connection_ready) || (!datasink_ready)) {
                // publish settings
                publisher.send(zmq::str_buffer("settings"), zmq::send_flags::sndmore);
                size_t settingssize = Measurement.ByteSizeLong();
                void *settingsbuffer = malloc(settingssize);
                Measurement.SerializeToArray(settingsbuffer, settingssize);
                zmq::message_t request(settingssize);
                memcpy((void *)request.data(), settingsbuffer, settingssize);
                publisher.send(request, zmq::send_flags::none);
                std::cout<<"sent!"<<std::endl;

                // check if ccd connection is ready
                if (!ccd_connection_ready) {
                    zmq::message_t ccdreply;
                    (void)ccd.recv(ccdreply, zmq::recv_flags::dontwait);
                    std::string ccdenv_str = "";
                    ccdenv_str = std::string(static_cast<char*>(ccdreply.data()), ccdreply.size());
                    if (ccdenv_str != "") {
                        zmq::message_t ccdmsg;
                        (void)ccd.recv(ccdmsg, zmq::recv_flags::none);
                        std::string ccdmsg_str = std::string(static_cast<char*>(ccdmsg.data()), ccdmsg.size());
                        if (ccdmsg_str == "connection ready") {
                            ccd_connection_ready = true;
                            emit sendDeviceStatusToGUI("ccd", QString::fromStdString(ccdmsg_str));
                        }
                    }
                }

                // check if sdd connection is ready
                if (!sdd_connection_ready) {
                    zmq::message_t sddreply;
                    (void)sdd.recv(sddreply, zmq::recv_flags::dontwait);
                    std::string sddenv_str = "";
                    sddenv_str = std::string(static_cast<char*>(sddreply.data()), sddreply.size());
                    if (sddenv_str != "") {
                        zmq::message_t sddmsg;
                        (void)sdd.recv(sddmsg, zmq::recv_flags::none);
                        std::string sddmsg_str = std::string(static_cast<char*>(sddmsg.data()), sddmsg.size());
                        if (sddmsg_str == "connection ready") {
                            sdd_connection_ready = true;
                            emit sendDeviceStatusToGUI("sdd", QString::fromStdString(sddmsg_str));
                        }
                    }
                }

                // check if datasink is ready
                if (!datasink_ready) {
                    zmq::message_t datasinkreply;
                    (void)datasink.recv(datasinkreply, zmq::recv_flags::dontwait);
                    std::string datasinkenv_str = "";
                    datasinkenv_str = std::string(static_cast<char*>(datasinkreply.data()), datasinkreply.size());
                    if (datasinkenv_str != "") {
                        zmq::message_t datasinkmsg;
                        (void)datasink.recv(datasinkmsg, zmq::recv_flags::none);
                        std::string datasinkmsg_str = std::string(static_cast<char*>(datasinkmsg.data()), datasinkmsg.size());
                        if (datasinkmsg_str == "ready") {
                            datasink_ready = true;
                            emit sendDeviceStatusToGUI("datasink", "ready");
                        }
                    }
                }
                QThread::msleep(100);
            }

            if (!metadata_sent) {
                // send beamline parameter
                // HERE: get beamline parameter
                animax::Metadata Metadata;

                // get current time
                QDate cd = QDate::currentDate();
                QTime ct = QTime::currentTime();

                QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

                // define protobuf values
                Metadata.set_acquisition_number(acquisition_number);
                Metadata.set_acquisition_time(datetime.toStdString());
                Metadata.set_set_energy(settings.energies[acquisition_number-1]);
                Metadata.set_beamline_energy(settings.energies[acquisition_number-1]+0.5);
                Metadata.set_ringcurrent(198);
                Metadata.set_horizontal_shutter(true);
                Metadata.set_vertical_shutter(true);

                // publish settings
                publisher.send(zmq::str_buffer("metadata"), zmq::send_flags::sndmore);
                size_t metadatasize = Metadata.ByteSizeLong();
                void *metadatabuffer = malloc(metadatasize);
                Metadata.SerializeToArray(metadatabuffer, metadatasize);
                zmq::message_t request(metadatasize);
                memcpy((void *)request.data(), metadatabuffer, metadatasize);
                publisher.send(request, zmq::send_flags::none);
                std::cout<<"sent metadata!"<<std::endl;

                std::cout<<"all connected"<<std::endl;
            }

            while ((!sdd_detector_ready) || (!ccd_detector_ready)) {

                if (!metadata_sent) {
                    // send beamline parameter
                    // HERE: get beamline parameter
                    animax::Metadata Metadata;
                    // get current time
                    QDate cd = QDate::currentDate();
                    QTime ct = QTime::currentTime();

                    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

                    // define protobuf values
                    Metadata.set_acquisition_number(acquisition_number);
                    Metadata.set_acquisition_time(datetime.toStdString());
                    Metadata.set_set_energy(settings.energies[acquisition_number-1]);
                    Metadata.set_beamline_energy(settings.energies[acquisition_number-1]+0.5);
                    Metadata.set_ringcurrent(198);
                    Metadata.set_horizontal_shutter(true);
                    Metadata.set_vertical_shutter(true);

                    // publish settings
                    publisher.send(zmq::str_buffer("metadata"), zmq::send_flags::sndmore);
                    size_t metadatasize = Metadata.ByteSizeLong();
                    void *metadatabuffer = malloc(metadatasize);
                    Metadata.SerializeToArray(metadatabuffer, metadatasize);
                    zmq::message_t request(metadatasize);
                    memcpy((void *)request.data(), metadatabuffer, metadatasize);
                    publisher.send(request, zmq::send_flags::none);
                    std::cout<<"sent metadata!"<<std::endl;

                    std::cout<<"all connected"<<std::endl;
                    metadata_sent = true;
                }

                if (!sdd_detector_ready) {
                // check if sdd detector is ready
                    zmq::message_t sddreply;
                    (void)sdd.recv(sddreply, zmq::recv_flags::dontwait);
                    std::string sddenv_str = "";
                    sddenv_str = std::string(static_cast<char*>(sddreply.data()), sddreply.size());
                    if (sddenv_str != "") {
                        zmq::message_t sddmsg;
                        (void)sdd.recv(sddmsg, zmq::recv_flags::none);
                        std::string sddmsg_str = std::string(static_cast<char*>(sddmsg.data()), sddmsg.size());
                        if (sddmsg_str == "detector ready") {
                            sdd_detector_ready = true;
                            emit sendDeviceStatusToGUI("sdd", QString::fromStdString(sddmsg_str));
                        }
                    }
                }

                if (!ccd_detector_ready) {
                // check if ccd detector is ready
                    zmq::message_t ccdreply;
                    (void)ccd.recv(ccdreply, zmq::recv_flags::dontwait);
                    std::string ccdenv_str = "";
                    ccdenv_str = std::string(static_cast<char*>(ccdreply.data()), ccdreply.size());
                    if (ccdenv_str != "") {
                        zmq::message_t ccdmsg;
                        (void)ccd.recv(ccdmsg, zmq::recv_flags::none);
                        std::string ccdmsg_str = std::string(static_cast<char*>(ccdmsg.data()), ccdmsg.size());
                        if (ccdmsg_str == "detector ready") {
                            ccd_detector_ready = true;
                            emit sendDeviceStatusToGUI("ccd", QString::fromStdString(ccdmsg_str));
                        }
                    }
                }
            }

            if ((datasink_ready) && (ccd_connection_ready) && (ccd_detector_ready) && (sdd_connection_ready) && (sdd_detector_ready) && (metadata_sent)) {
                zmq::message_t previewmessage;
                (void)datasink.recv(previewmessage, zmq::recv_flags::dontwait);
                if (previewmessage.size() > 0) {
                    std::string datasinkenv_str = "";
                    datasinkenv_str = std::string(static_cast<char*>(previewmessage.data()), previewmessage.size());
                    if (datasinkenv_str == "previewdata") {
                        zmq::message_t previewmsg;
                        (void)datasink.recv(previewmsg, zmq::recv_flags::none);
                        animax::preview Preview;
                        Preview.ParseFromArray(previewmsg.data(), previewmsg.size());
                        std::string previewtype = Preview.type();
                        std::string previewdata = Preview.previewdata();

                        // send preview data to GUI
                        emit sendPreviewDataToGUI(previewtype, previewdata);
                    }

                    if (datasinkenv_str == "roidata") {
                        zmq::message_t roimsg;
                        (void)datasink.recv(roimsg, zmq::recv_flags::none);
                        animax::ROI roi;
                        roi.ParseFromArray(roimsg.data(), roimsg.size());
                        std::string roielement = roi.element();
                        std::string roidata = roi.roidata();

                        // send preview data to GUI
                        emit sendROIDataToGUI(roielement, roidata);
                    }

                    if (datasinkenv_str == "scanstatus") {
                        zmq::message_t scanstatusmsg;
                        (void)datasink.recv(scanstatusmsg, zmq::recv_flags::none);
                        animax::scanstatus scanstatus;
                        scanstatus.ParseFromArray(scanstatusmsg.data(), scanstatusmsg.size());
                        std::string scanstatusstr = scanstatus.status();

                        std::cout<<"received scanstatus message: "<<scanstatusstr<<std::endl;

                        QThread::sleep(5);

                        sdd_detector_ready = false;
                        ccd_detector_ready = false;
                        // image is ready: change settings of beamline, positioner etc.
                        // if finished, get new metadata and send out new metadata
                        // send beamline parameter

                        // HERE: get beamline parameter
                        animax::Metadata Metadata;
                        // define protobuf values
                        acquisition_number++;

                        // get current time
                        QDate cd = QDate::currentDate();
                        QTime ct = QTime::currentTime();

                        QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

                        // define protobuf values
                        Metadata.set_acquisition_number(acquisition_number);
                        Metadata.set_acquisition_time(datetime.toStdString());
                        Metadata.set_set_energy(settings.energies[acquisition_number-1]);
                        Metadata.set_beamline_energy(settings.energies[acquisition_number-1]+0.5);
                        Metadata.set_ringcurrent(198);
                        Metadata.set_horizontal_shutter(true);
                        Metadata.set_vertical_shutter(true);

                        // publish settings
                        publisher.send(zmq::str_buffer("metadata"), zmq::send_flags::sndmore);
                        size_t metadatasize = Metadata.ByteSizeLong();
                        void *metadatabuffer = malloc(metadatasize);
                        Metadata.SerializeToArray(metadatabuffer, metadatasize);
                        zmq::message_t request(metadatasize);
                        memcpy((void *)request.data(), metadatabuffer, metadatasize);
                        publisher.send(request, zmq::send_flags::none);
                        std::cout<<"sent metadata!"<<std::endl;
                    }
                }
            }
        }
}
