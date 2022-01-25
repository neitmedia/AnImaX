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

// constructor of sddThread object
sddThread::sddThread(QString s, QString roijson, int scanX, int scanY) : ip(s), roijson(roijson), scanX(scanX), scanY(scanY)
{
    /* generate ROI data structure / deserialize json string */

    // declare QJsonParseError object
    QJsonParseError jsonError;

    // declare QJsonDocument object
    QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);

    // declare QJsonObject object
    QJsonObject jsonObj = document.object();

    // create list of all wanted ROIs
    foreach(QString key, jsonObj.keys()) {
        // declare and define std::string variable and write current JSON key to this variable
        std::string k(key.toLocal8Bit());

        // define QVector of uint32_t
        QVector<uint32_t> roivect;

        // insert key and QVector into global ROImap variable of type roidata (QMap<std::string, QVector<uint32_t>>)
        ROImap.insert(k, roivect);
    }
}

// run function is started when thread is launched
void sddThread::run()
{
        // declare zmq context and subscriber object
        zmq::context_t context(1);
        zmq::socket_t subscriber(context, ZMQ_SUB);

        // connect to sdd endpoint
        subscriber.connect("tcp://"+ip.toStdString());

        // subscribe to messages with envelope "statusdata" that contain status of sdd
        subscriber.set(zmq::sockopt::subscribe, "statusdata");

        // subscribe to messages with envelope "sdd" that contain raw detector data
        subscriber.set(zmq::sockopt::subscribe, "sdd");

        // give out some debug info
        std::cout<<"waiting for ready signal from sdd..."<<std::endl;

        /* START INITIALIZATION LOOP */

        // wait for ready signal from ccd and sdd
        while (!sddReadyState) {

            // declare zmq::message_t object env for message envelope
            zmq::message_t env;

            // wait for message and receive message envelope
            subscriber.recv(env);

            // cast message envelope data to string
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());

            // give out some debug info
            std::cout << "Received envelope '" << env_str << "'" << std::endl;

            // declare zmq::message_t object msg for message content
            zmq::message_t msg;

            // receive message data
            subscriber.recv(msg);

            // cast message content to string
            std::string msg_str = std::string(static_cast<char*>(msg.data()), msg.size());

            // give out some debug info
            std::cout << "Received:" << msg_str << std::endl;

            // if ccd is ready, set public variable "sddReadyState" to leave the initialization loop
            if (msg_str == "connection ready") {

                // give out some debug info
                std::cout<<"sdd connection is ready"<<std::endl;

                // set global variable to leave the initialization loop
                sddReadyState = true;

                // send sdd status to mainwindow.cpp / datasink GUI
                emit sddReady();
            }
        }

        /* END INITIALIZATION LOOP */

        // declare and initialize pixel number / channel variable
        int x = 0;

        // declare and initialize spectrum counter variable
        int y = 0;

        // declare variable of type spectrumdata (QVector<uint32_t>)
        spectrumdata spekdata;

        // declare and initialize variable for dataindex (dataindex is later incremented every 2 bytes of raw data)
        long dataindex = 0;

        // make sure that the spectrum array is empty
        for (int i=0;i<4096;i++) {
            spekdata.append(0);
        }

        /* START MAIN LOOP */

        while (!stop)
        {
            try {
                // declare zmq::message_t variable env for message envelope
                zmq::message_t env;

                // wait for message and receive message envelope
                subscriber.recv(env);

                // cast message envelope to string
                std::string env_str = std::string(static_cast<char*>(env.data()), env.size());

                // is message envelope contains string "sdd", sdd raw data has been received
                if (env_str == "sdd") {

                    // declare zmq::message_t variable msg
                    zmq::message_t msg;

                    // receive message content
                    subscriber.recv(msg);

                    // declare animax::sdd object sdd
                    animax::sdd sdd;

                    // parse / deserialize message content to animax::sdd object
                    sdd.ParseFromArray(msg.data(), msg.size());

                    // declare std::string variable sdd_rawdata for raw sdd data (google protobuf bytes type is string in C/C++)
                    std::string sdd_rawdata;

                    // write sdd raw data from animax::sdd object to sdd_rawdata
                    sdd_rawdata = sdd.pixeldata();

                    // if data length is > 0, process raw sdd data
                    if (sdd_rawdata.size() > 0) {

                        /* START SDD RAW DATA PROCESSING */

                        // iterate through the raw sdd data
                        for (unsigned long i=0; i<=sdd_rawdata.size()-1; i=i+2) {

                            // increment dataindex
                            dataindex++;

                            // in each iteration, write 2 bytes into variable
                            uint16_t eventdata = ((uint16_t) sdd_rawdata[i+1] << 8) | (uint8_t)sdd_rawdata[i];

                            // if first 4 bit of eventdata (eventdata >> 12) is 1, data chunk contains a detected count,
                            // the other 12 bit (eventdata%4096) contain the count of the pixel where the count was detected
                            if ((eventdata >> 12) == 1) {

                                // write the pixel count where the count was detected to variable y
                                y = eventdata%4096;

                                // increment the value of the spectrum array at the specified position
                                spekdata[y]++;
                            }

                            // if first 4 bit of eventdata (eventdata >> 12) is 8, the current spectrum is complete
                            if ((eventdata >> 12) == 8) {

                                // send spectrum to mainwindow.cpp / datasink GUI
                                emit sendSpectrumDataToGUI(x, spekdata);

                                /* START ROI GENERATION */

                                // declare QJsonParseError object
                                QJsonParseError jsonError;

                                // declare QJsonDocument object document and parse JSON data into this object
                                QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);

                                // declare QJsonObject
                                QJsonObject jsonObj = document.object();

                                // iterate through JSON object
                                foreach(QString key, jsonObj.keys()) {

                                    // write current key string to variable k
                                    std::string k(key.toLocal8Bit());

                                    // declare and define QJsonObject intervals that contains the ROI intervals from JSON
                                    QJsonObject intervals = jsonObj[key].toObject();

                                    // iterate through intervals
                                    foreach(QString intkey, intervals.keys()) {

                                        // write interval key to std::string variable ik
                                        std::string ik(intkey.toLocal8Bit());

                                        // get counts from data
                                        QJsonValue counts = intervals[intkey];

                                        // convert to array
                                        QJsonArray countarr = counts.toArray();

                                        // write interval start to variable
                                        int channel_start = countarr[0].toInt();

                                        // write interval end to variable
                                        int channel_end = countarr[1].toInt();

                                        // declare and initalize sum variable
                                        uint32_t sum = 0;

                                        // iterate through interval
                                        for (int i=channel_start; i<=channel_end; i++) {
                                            // add counts of spectrum at current position to sum variable
                                            sum += (uint32_t)spekdata.at(i);
                                        }

                                        // add ROI to ROI list
                                        ROI_P.append(sum);

                                        // write ROI data to ROI map
                                        auto &roivect = ROImap[k];
                                        roivect.append(sum);
                                    }
                                }

                                /* END ROI GENERATION */

                                // make sure that the spectrum array is empty
                                for (int i=0;i<4096;i++) {
                                    spekdata[i] = 0;
                                }

                                // increase the spectrum counter
                                x++;
                            }

                            // if first 4 bit of eventdata (eventdata >> 12) is 10, last 12 bit contains scan index data
                            if ((eventdata >> 12) == 10) {

                                // last 12 bit contain scan index data
                                int xstop = eventdata%4096;
                                int nopx = x;

                                // send write index to mainwindow.cpp / data sink GUI thread
                                emit sendScanIndexDataToGUI(dataindex-1, nopx, xstop);
                            }

                            // if first 4 bit of eventdata (eventdata >> 12) is 11, last 12 bit contains line break data
                            if ((eventdata >> 12) == 11) {

                                // last 12 bit contain line break data
                                int xstop = eventdata%4096;

                                // send line break data and ROIs to mainwindow.cpp / data sink GUI thread
                                emit sendLineBreakDataToGUI(ROImap, dataindex-1, x, xstop);

                                // if spectrum counter x == scanX*scanY, scan is complete
                                if (x == scanX*scanY) {

                                    // give out some debug info
                                    std::cout<<"sdd scan received"<<std::endl;
                                    std::cout<<"clear variables..."<<std::endl;

                                    // clear counter variables
                                    x = 0;
                                    y = 0;
                                    dataindex = 0;

                                    // clear the spectrum array
                                    for (int i=0;i<4096;i++) {
                                        spekdata.append(0);
                                    }

                                    // clear ROI list and ROI map
                                    ROIs.clear();
                                    ROImap.clear();

                                    // give out some debug info
                                    std::cout<<"cleared variables..."<<std::endl;
                                }

                            }
                        }

                        /* END SDD RAW DATA PROCESSING */
                  }
                }
            }  catch (zmq::error_t &err)
            {
                printf("%s", err.what());
            }
        }

        /* END MAIN LOOP */
}
