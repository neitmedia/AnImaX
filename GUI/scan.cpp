#include "scan.h"
#include <QDebug>
#include <stdio.h>
#include <QThread>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>

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
        publisher.bind("tcp://*:5555");

        // Prepare ccd subscriber => open "statusdata" envelopes from ccd PC
        zmq::socket_t ccd(ctx, zmq::socket_type::sub);
        ccd.connect("tcp://127.0.0.1:5556");
        ccd.set(zmq::sockopt::subscribe, "statusdata");

        // Prepare sdd subscriber => open "statusdata" envelopes from sdd PC
        zmq::socket_t sdd(ctx, zmq::socket_type::sub);
        //sdd.connect("tcp://127.0.0.1:5557");
        sdd.connect("tcp://192.168.178.41:5557");
        sdd.set(zmq::sockopt::subscribe, "statusdata");

        // Prepare datasink subscriber => open "statusdata", "previewdata" and "roidata" envelopes from datasink PC
        zmq::socket_t datasink(ctx, zmq::socket_type::sub);
        datasink.connect("tcp://127.0.0.1:5558");
        datasink.set(zmq::sockopt::subscribe, "statusdata");
        datasink.set(zmq::sockopt::subscribe, "previewdata");
        datasink.set(zmq::sockopt::subscribe, "roidata");
        datasink.set(zmq::sockopt::subscribe, "scanstatus");

        // prepare "Measurement"-protobuf
        animax::Measurement Measurement;
        // define protobuf values
        Measurement.set_width(settings.scanWidth);
        Measurement.set_height(settings.scanHeight);
        Measurement.set_aquisition_time(10);
        Measurement.set_energy_count(1);
        Measurement.add_energies(100);
        Measurement.set_ccdheight(settings.ccdHeight);
        Measurement.set_ccdwidth(settings.ccdWidth);
        Measurement.set_roidefinitions(settings.roidefinitions);
        Measurement.set_scantype(settings.scantype);

        bool ccd_ready = false;
        bool sdd_ready = false;
        bool datasink_ready = false;
        bool metadata_sent = false;

        while (true) {

            // send settings and make sure all hosts and peripherals / devices are connected
            while ((!ccd_ready) || (!sdd_ready) || (!datasink_ready)) {
                // publish settings
                publisher.send(zmq::str_buffer("settings"), zmq::send_flags::sndmore);
                size_t settingssize = Measurement.ByteSizeLong();
                void *settingsbuffer = malloc(settingssize);
                Measurement.SerializeToArray(settingsbuffer, settingssize);
                zmq::message_t request(settingssize);
                memcpy((void *)request.data(), settingsbuffer, settingssize);
                publisher.send(request, zmq::send_flags::none);
                std::cout<<"sent!"<<std::endl;

                // check if ccd is ready
                if (!ccd_ready) {
                    zmq::message_t ccdreply;
                    (void)ccd.recv(ccdreply, zmq::recv_flags::dontwait);
                    std::string ccdenv_str = "";
                    ccdenv_str = std::string(static_cast<char*>(ccdreply.data()), ccdreply.size());
                    if (ccdenv_str != "") {
                        zmq::message_t ccdmsg;
                        (void)ccd.recv(ccdmsg, zmq::recv_flags::none);
                        std::string ccdmsg_str = std::string(static_cast<char*>(ccdmsg.data()), ccdmsg.size());
                        if (ccdmsg_str == "ready") {
                            ccd_ready = true;
                            emit sendDeviceStatusToGUI("ccd", "ready");
                        }
                    }
                }

                // check if sdd is ready
                if (!sdd_ready) {
                    zmq::message_t sddreply;
                    (void)sdd.recv(sddreply, zmq::recv_flags::dontwait);
                    std::string sddenv_str = "";
                    sddenv_str = std::string(static_cast<char*>(sddreply.data()), sddreply.size());
                    if (sddenv_str != "") {
                        zmq::message_t sddmsg;
                        (void)sdd.recv(sddmsg, zmq::recv_flags::none);
                        std::string sddmsg_str = std::string(static_cast<char*>(sddmsg.data()), sddmsg.size());
                        if (sddmsg_str == "ready") {
                            sdd_ready = true;
                            emit sendDeviceStatusToGUI("sdd", "ready");
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
                // define protobuf values
                Metadata.set_aquisition_number(aquisition_number);
                Metadata.set_beamline_enery(123);
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

            if ((datasink_ready) && (ccd_ready) && (sdd_ready) && (metadata_sent)) {
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

                        // image is ready: change settings of beamline, positioner etc.
                        // if finished, get new metadata and send out new metadata
                        // send beamline parameter

                        // HERE: get beamline parameter
                        animax::Metadata Metadata;
                        // define protobuf values
                        aquisition_number++;
                        Metadata.set_aquisition_number(aquisition_number);
                        Metadata.set_beamline_enery(123);
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
