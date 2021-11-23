#include "sddthread.h"
#include <QDebug>
#include <stdio.h>
#include <QThread>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <bitset>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <zmq.hpp>

sddThread::sddThread(QString s, QString roijson, int scanX, int scanY) : ip(s), roijson(roijson), scanX(scanX), scanY(scanY)
{
    // generate ROI data structure
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);
    QJsonObject jsonObj = document.object();
    foreach(QString key, jsonObj.keys()) {
        std::string k(key.toLocal8Bit());
        QVector<uint32_t> roivect;
        ROImap.insert(k, roivect);
    }
}

// We overrides the QThread's run() method here
// run() will be called when a thread starts
// the code will be shared by all threads
void sddThread::run()
{
        zmq::context_t context(1);
        zmq::socket_t subscriber(context, ZMQ_SUB);
        //subscriber.connect("tcp://"+ip.toStdString()+":5557");
        subscriber.connect("tcp://192.168.178.41:5557");
        subscriber.set(zmq::sockopt::subscribe, "statusdata");
        subscriber.set(zmq::sockopt::subscribe, "sdd");
        std::cout<<"waiting for ready signal from sdd..."<<std::endl;

        // wait for ready signal from ccd and sdd
        while (!sddReadyState) {
            zmq::message_t env;
            subscriber.recv(env);
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
            std::cout << "Received envelope '" << env_str << "'" << std::endl;

            zmq::message_t msg;
            subscriber.recv(msg);
            std::string msg_str = std::string(static_cast<char*>(msg.data()), msg.size());

            std::cout << "Received:" << msg_str << std::endl;

            // if ccd is ready, set public variable "sddReady" give controlthread the permission to get this info
            if (msg_str == "ready") {
                std::cout<<"sdd is ready"<<std::endl;
                sddReadyState = true;
                emit sddReady();
            }
        }

        int x = 0;
        int y = 0;
        spectrumdata spekdata;
        int dataindex = 0;

        // make sure that the spectrum array is empty
        for (int i=0;i<4096;i++) {
            spekdata.append(0);
        }

        while (!stop)
        {
            try {
                zmq::message_t env;
                subscriber.recv(env);
                std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
                std::cout << "Received envelope '" << env_str << "'" << std::endl;

                if (env_str == "sdd") {
                    zmq::message_t msg;
                    subscriber.recv(msg);

                    animax::sdd sdd;
                    sdd.ParseFromArray(msg.data(), msg.size());
                    std::string text_str;
                    text_str = sdd.pixeldata();

                    std::cout << "Received" << text_str.size() << std::endl;

                    // iterate through the data buffer
                    for (unsigned long i=0;i<=text_str.size()-1;i=i+2) {
                        dataindex++;
                        // in each iteration, write 2 bytes into dummy variable
                        uint16_t dummy = ((uint16_t) text_str[i+1] << 8) | (uint8_t)text_str[i];
                        // print content of the dummy variable to the terminal for debugging purposes
                        // count data if first 4 bit of the short is "0001",
                        // other bits stand for the pixel where the count was detected
                        if ((dummy >> 12) == 1) {
                            y = dummy%4096;
                            // increment the value of the spectrum array at the specified position
                            spekdata[y]++;
                        }

                        if ((dummy >> 12) == 8) {
                            //std::cout << " spectrum finished +"<<x<< std::endl;

                            //std::cout<<"spektrum an position 391: "<<spekdata.at(391)<<std::endl;

                            emit sendSpectrumDataToGUI(x, spekdata);

                            // generate ROIs
                            QJsonParseError jsonError;
                            QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);
                            QJsonObject jsonObj = document.object();
                            foreach(QString key, jsonObj.keys()) {
                                std::string k(key.toLocal8Bit());
                                //std::cout <<"k: "<<k<<std::endl;

                                QJsonObject intervals = jsonObj[key].toObject();

                                foreach(QString intkey, intervals.keys()) {

                                    std::string ik(intkey.toLocal8Bit());

                                    QJsonValue counts = intervals[intkey];
                                    QJsonArray countarr = counts.toArray();

                                    int channel_start = countarr[0].toInt();
                                    int channel_end = countarr[1].toInt();

                                    uint32_t sum = 0;

                                    for (int i=channel_start; i<=channel_end; i++) {
                                        sum += (uint32_t)spekdata.at(i);
                                    }

                                    ROI_P.append(sum);
                                    auto &roivect = ROImap[k];
                                    roivect.append(sum);

                                }

                            }


                            // make sure that the spectrum array is empty
                            for (int i=0;i<4096;i++) {
                                spekdata[i] = 0;
                            }

                            // increase the spectrum counter
                            x++;
                        }

                        if ((dummy >> 12) == 10) {
                            int xstop = dummy%4096;
                            int nopx = x;
                            //std::cout << "scan index is given: "<< dataindex-1 << " " << nopx << " " << xstop << std::endl;
                            emit sendScanIndexDataToGUI(dataindex-1, nopx, xstop);
                        }
                        if ((dummy >> 12) == 11) {
                            int xstop = dummy%4096;
                            //std::cout << "line break is given: "<< dataindex-1 << " " << x << " " << xstop << std::endl;
                            emit sendLineBreakDataToGUI(ROImap, dataindex-1, x, xstop);

                            if (x == scanX*scanY) {
                                std::cout<<"sdd scan received"<<std::endl;
                                std::cout<<"clear variables..."<<std::endl;
                                x = 0;
                                y = 0;
                                dataindex = 0;

                                // make sure that the spectrum array is empty
                                for (int i=0;i<4096;i++) {
                                    spekdata.append(0);
                                }

                                ROIs.clear();
                                ROImap.clear();

                                std::cout<<"cleared variables..."<<std::endl;
                            }

                        }
                    }
                }
            }  catch (zmq::error_t &err)
            {
                printf("%s", err.what());
            }
        }
}
