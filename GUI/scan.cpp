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
        ccd.set(zmq::sockopt::subscribe, "ccdsettings");

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
        Measurement.set_x_step_size(settings.x_step_size);
        Measurement.set_y_step_size(settings.y_step_size);
        Measurement.set_scantitle(settings.scantitle);
        Measurement.set_acquisition_time(10);
        Measurement.set_energy_count(settings.energycount);
        for (int i=0;i<settings.energycount;i++) {
            Measurement.add_energies(settings.energies[i]);
        }
        Measurement.set_roidefinitions(settings.roidefinitions);
        Measurement.set_scantype(settings.scantype);
        Measurement.set_save_path(settings.save_path);
        Measurement.set_save_file(settings.save_file);
        Measurement.set_file_compression(settings.file_compression);
        Measurement.set_file_compression_level(settings.file_compression_level);

        std::cout<<"file compression: "<<Measurement.file_compression()<<std::endl;

        // network settings
        Measurement.set_ccdip(settings.ccdIP);
        Measurement.set_ccdport(settings.ccdPort);
        Measurement.set_sddip(settings.sddIP);
        Measurement.set_sddport(settings.sddPort);
        Measurement.set_datasinkip(settings.datasinkIP);
        Measurement.set_datasinkport(settings.datasinkPort);

        // ccd settings
        Measurement.set_binning_x(settings.binning_x);
        Measurement.set_binning_y(settings.binning_y);
        Measurement.set_ccdheight(settings.ccdHeight);
        Measurement.set_ccdwidth(settings.ccdWidth);
        Measurement.set_pixelcount(settings.pixelcount);
        Measurement.set_frametransfer_mode(settings.frametransfer_mode);
        Measurement.set_number_of_accumulations(settings.number_of_accumulations);
        Measurement.set_number_of_scans(settings.number_of_scans);
        Measurement.set_set_kinetic_cycle_time(settings.set_kinetic_cycle_time);
        Measurement.set_read_mode(settings.read_mode);
        Measurement.set_acquision_mode(settings.acquision_mode);
        Measurement.set_shutter_mode(settings.shutter_mode);
        Measurement.set_shutter_output_signal(settings.shutter_output_signal);
        Measurement.set_shutter_open_time(settings.shutter_open_time);
        Measurement.set_shutter_close_time(settings.shutter_close_time);
        Measurement.set_triggermode(settings.triggermode);
        Measurement.set_exposure_time(settings.exposure_time);
        Measurement.set_accumulation_time(settings.accumulation_time);
        Measurement.set_kinetic_time(settings.kinetic_time);
        Measurement.set_min_temp(settings.min_temp);
        Measurement.set_max_temp(settings.max_temp);
        Measurement.set_target_temp(settings.target_temp);
        Measurement.set_pre_amp_gain(settings.pre_amp_gain);
        Measurement.set_em_gain_mode(settings.em_gain_mode);
        Measurement.set_em_gain(settings.em_gain);

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

        // source settings
        Measurement.set_source_name(settings.source_name);
        Measurement.set_source_probe(settings.source_probe);
        Measurement.set_source_type(settings.source_type);

        // additional settings
        Measurement.set_notes(settings.notes);
        Measurement.set_userdata(settings.userdata);
        /* END SET PROTOBUF VALUES */

        bool ccd_connection_ready = false;
        bool ccd_detector_ready = false;
        bool ccd_settings_ready = false;
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

            if ((!metadata_sent) && (ccd_connection_ready) && (sdd_connection_ready) && (datasink_ready)) {

                std::cout<<"all connected!"<<std::endl;

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

                std::cout<<"sent initial metadata!"<<std::endl;

                metadata_sent = true;
            }

            while ((!sdd_detector_ready) || (!ccd_detector_ready)) {
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

                if (!ccd_detector_ready || !ccd_settings_ready) {

                    zmq::message_t ccdreply;
                    (void)ccd.recv(ccdreply, zmq::recv_flags::dontwait);
                    std::string ccdenv_str = "";
                    ccdenv_str = std::string(static_cast<char*>(ccdreply.data()), ccdreply.size());

                    if (!ccd_detector_ready) {
                    // check if ccd detector is ready
                        if (ccdenv_str == "statusdata") {
                            zmq::message_t ccdmsg;
                            (void)ccd.recv(ccdmsg, zmq::recv_flags::none);
                            std::string ccdmsg_str = std::string(static_cast<char*>(ccdmsg.data()), ccdmsg.size());
                            if (ccdmsg_str == "detector ready") {
                                ccd_detector_ready = true;
                                emit sendDeviceStatusToGUI("ccd", QString::fromStdString(ccdmsg_str));
                            }
                        }
                    }

                    if (!ccd_settings_ready) {
                        if (ccdenv_str == "ccdsettings") {
                            zmq::message_t ccdmsg;
                            (void)ccd.recv(ccdmsg, zmq::recv_flags::none);

                            std::cout<<"received calculated real ccd settings"<<std::endl;

                            // deserialize ccdsettings protobuf
                            animax::ccdsettings real_ccdsettings;
                            real_ccdsettings.ParseFromArray(ccdmsg.data(), ccdmsg.size());
                            int32_t set_kinetic_cycle_time = real_ccdsettings.set_kinetic_cycle_time();
                            int32_t exposure_time = real_ccdsettings.exposure_time();
                            int32_t accumulation_time = real_ccdsettings.accumulation_time();
                            int32_t kinetic_time = real_ccdsettings.kinetic_time();

                            // Print values for debugging purposes
                            std::cout<<"set kinetic cycle time: "<<set_kinetic_cycle_time<<std::endl;
                            std::cout<<"exposure time: "<<exposure_time<<std::endl;
                            std::cout<<"accumulation time: "<<accumulation_time<<std::endl;
                            std::cout<<"kinetic time: "<<kinetic_time<<std::endl;

                            ccd_settings_ready = true;
                            metadata_sent = false;
                        }
                    }
                }
            }

            if ((datasink_ready) && (ccd_connection_ready) && (ccd_detector_ready) && (ccd_settings_ready) && (sdd_connection_ready) && (sdd_detector_ready) && (metadata_sent)) {
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

                        if (scanstatusstr == "part finished") {
                            std::cout<<"received scanstatus message: "<<scanstatusstr<<std::endl;

                            //QThread::sleep(5);

                            acquisition_number++;

                            sdd_detector_ready = false;
                            ccd_detector_ready = false;

                            metadata_sent = false;
                        }

                        if (scanstatusstr == "whole scan finished") {
                            std::cout<<"received scanstatus message: "<<scanstatusstr<<std::endl;
                            emit sendScanFinished();
                        }
                    }
                }
            }

            if (stopscan || pausescan || resumescan) {
                // declare ScanNote object
                animax::scanstatus scanstatus;

                // define protobuf values
                if ((stopscan) && (!pausescan) && (!resumescan)) {
                    scanstatus.set_status("stop");
                }

                if ((pausescan) && (!stopscan) && (!resumescan)) {
                    scanstatus.set_status("pause");
                }

                if ((resumescan) && (!stopscan) && (!pausescan)) {
                    scanstatus.set_status("resume");
                }

                stopscan = false;
                pausescan = false;
                resumescan = false;

                // publish status data
                publisher.send(zmq::str_buffer("scanstatus"), zmq::send_flags::sndmore);
                size_t scanstatusdatasize = scanstatus.ByteSizeLong();
                void *scanstatusdatabuffer = malloc(scanstatusdatasize);
                scanstatus.SerializeToArray(scanstatusdatabuffer, scanstatusdatasize);
                zmq::message_t request(scanstatusdatasize);
                memcpy((void *)request.data(), scanstatusdatabuffer, scanstatusdatasize);
                publisher.send(request, zmq::send_flags::none);

                std::cout<<"sent scan '"<<scanstatus.status()<<"' message!"<<std::endl;
            }


            if (scannote != "") {
                // declare ScanNote object
                animax::scannote scannotebuf;
                scannotebuf.set_text(scannote);
                // publish status data
                publisher.send(zmq::str_buffer("scannote"), zmq::send_flags::sndmore);
                size_t scannotedatasize = scannotebuf.ByteSizeLong();
                void *scannotedatabuffer = malloc(scannotedatasize);
                scannotebuf.SerializeToArray(scannotedatabuffer, scannotedatasize);
                zmq::message_t request(scannotedatasize);
                memcpy((void *)request.data(), scannotedatabuffer, scannotedatasize);
                publisher.send(request, zmq::send_flags::none);

                std::cout<<"sent scan note '"<<scannotebuf.text()<<"'!"<<std::endl;

                // clear scan note
                scannote = "";
            }
        }
}
