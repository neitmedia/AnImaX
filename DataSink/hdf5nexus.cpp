#include "hdf5nexus.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDate>

hdf5nexus::hdf5nexus()
{
    qRegisterMetaType<roidata>( "roidata" );
    qRegisterMetaType<spectrumdata>( "spectrumdata" );
}

void hdf5nexus::closeDataFile() {
    // close file
    file->close();
    delete file;
}

void hdf5nexus::writeEndTimeStamp() {
    // write current timestamp to "/measurement/end_time"
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);
    newNeXusScalarDataSet("/measurement/end_time", "NX_DATE_TIME", datetime.toStdString(), true);
}

void hdf5nexus::newNeXusFileStringAttribute(std::string location, std::string content) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);

    Attribute att = file->createAttribute(location, str_type, att_space);
    att.write(str_type, std::string(content));
    att.close();
    str_type.close();
    att_space.close();
}

void hdf5nexus::newNeXusGroupStringAttribute(Group* group, std::string location, std::string content) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);

    Attribute att = group->createAttribute(location, str_type, att_space);
    att.write(str_type, std::string(content));
    att.close();
    str_type.close();
    att_space.close();
}

void hdf5nexus::newNeXusDatasetStringAttribute(DataSet* dataset, std::string location, std::string content) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);

    Attribute att = dataset->createAttribute(location, str_type, att_space);
    att.write(str_type, std::string(content));
    att.close();
    str_type.close();
    att_space.close();
}

/*DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, bool content, bool close) {

    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_HBOOL, fspace));
    newNeXusDatasetStringAttribute(dataset, "type", type);
    dataset->write(&content, PredType::NATIVE_HBOOL, att_space);

    att_space.close();
    fspace.close();

    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}*/

DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, std::string content, bool close) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* dataset = new DataSet(file->createDataSet(location, str_type, fspace));
    newNeXusDatasetStringAttribute(dataset, "type", type);
    dataset->write(std::string(content), str_type, att_space);

    str_type.close();
    att_space.close();
    fspace.close();

    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}

DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, float content, bool close) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    float contentarr[] = {content};

    DataSet* dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_FLOAT, fspace));
    newNeXusDatasetStringAttribute(dataset, "type", type);
    dataset->write(contentarr, PredType::NATIVE_FLOAT, att_space);

    str_type.close();
    att_space.close();
    fspace.close();

    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}

DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, int32_t content, bool close) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* dataset;

    if (type == "NX_FLOAT") {
        float contentarr[] = {(float)content};
        dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_FLOAT, fspace));
        newNeXusDatasetStringAttribute(dataset, "type", type);
        dataset->write(contentarr, PredType::NATIVE_FLOAT, att_space);
    } else {
        int32_t contentarr[] = {content};
        dataset = new DataSet(file->createDataSet(location, PredType::STD_I32LE, fspace));
        newNeXusDatasetStringAttribute(dataset, "type", type);
        dataset->write(contentarr, PredType::STD_I32LE, att_space);
    }

    str_type.close();
    att_space.close();
    fspace.close();

    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}

Group* hdf5nexus::newNeXusGroup(std::string location, std::string attrname, std::string attrcontent, bool close) {
    Group* group = new Group(file->createGroup(location));
    newNeXusGroupStringAttribute(group, attrname, attrcontent);
    if (close) {
        group->close();
        delete group;
    }
    return group;
}

DataSet* hdf5nexus::newNeXusChunkedCCDDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool close) {
    hsize_t dims[3];
    dims[0] = 0;
    dims[1] = (hsize_t)x;
    dims[2] = (hsize_t)y;

    hsize_t maxdims[3] = {H5S_UNLIMITED, (hsize_t)x, (hsize_t)y};
    hsize_t chunk_dims[3] = {1, (hsize_t)x, (hsize_t)y};

    DataSpace *dataspace = new DataSpace(3, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);

    // Create the chunked dataset.
    DataSet *dataset = new DataSet(file->createDataSet(location, predtype, *dataspace, prop));
    newNeXusDatasetStringAttribute(dataset, "type", typestr);

    prop.close();
    delete dataspace;

    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}

DataSet* hdf5nexus::newNeXusChunkedTransmissionPreviewDataSet(std::string location, H5::PredType predtype, std::string typestr, bool close) {
    // set variables
    hsize_t maxdimssumimage[2]    = {H5S_UNLIMITED, H5S_UNLIMITED};
    hsize_t chunk_dimssumimage[2] = {1, 1};

    hsize_t dimssumimage[2];
    dimssumimage[0] = 0;
    dimssumimage[1] = 0;

    DataSpace *dataspacesumimage = new DataSpace(2, dimssumimage, maxdimssumimage);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList propsumimage;
    propsumimage.setChunk(2, chunk_dimssumimage);

    DataSet *datasetsumimage = new DataSet(file->createDataSet(location, predtype, *dataspacesumimage, propsumimage));
    newNeXusDatasetStringAttribute(datasetsumimage, "type", typestr);

    propsumimage.close();
    delete dataspacesumimage;

    if (close) {
        datasetsumimage->close();
        delete datasetsumimage;
    }

    return datasetsumimage;
}

DataSet* hdf5nexus::newNeXusChunkedSpectraDataSet(std::string location, H5::PredType predtype, std::string typestr, bool close) {
    hsize_t dimsspec[2];
    dimsspec[0] = 0;
    dimsspec[1] = 4096;

    hsize_t sizespec[2];
    sizespec[0] = 1;
    sizespec[1] = 4096;

    hsize_t maxdimsspec[2] = {H5S_UNLIMITED, 4096};

    hsize_t chunk_dimsspec[2] = {1, 4096};

    // write fluorescence data
    DataSpace *dataspacespectrum = new DataSpace(2, dimsspec, maxdimsspec);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList propspectrum;
    propspectrum.setChunk(2, chunk_dimsspec);

    // Create the chunked dataset.  Note the use of pointer.
    DataSet *entryfluoinstrumentfluorescencedata = new DataSet(file->createDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, *dataspacespectrum, propspectrum));
    newNeXusDatasetStringAttribute(entryfluoinstrumentfluorescencedata, "type", "NX_FLOAT");

    propspectrum.close();
    delete dataspacespectrum;

    if (close) {
        entryfluoinstrumentfluorescencedata->close();
        delete entryfluoinstrumentfluorescencedata;
    }

    return entryfluoinstrumentfluorescencedata;
}

DataSet* hdf5nexus::newNeXusChunkedSDDLogDataSet(std::string location, H5::PredType predtype, std::string typestr, bool close) {
    // create linebreak log dataset
    hsize_t maxdimslinebreak[2]    = {H5S_UNLIMITED, 3};
    hsize_t chunk_dimslinebreak[2] = {1, 3};

    hsize_t dimslinebreak[2];
    dimslinebreak[0] = 0;
    dimslinebreak[1] = 3;

    DataSpace *dataspacelinebreak = new DataSpace(2, dimslinebreak, maxdimslinebreak);
    // Modify dataset creation property to enable chunking
    DSetCreatPropList proplinebreak;
    proplinebreak.setChunk(2, chunk_dimslinebreak);

    // Create the chunked datasets.  Note the use of pointer.
    DataSet *entrylog = new DataSet(file->createDataSet(location, predtype, *dataspacelinebreak, proplinebreak));
    newNeXusDatasetStringAttribute(entrylog, "type", typestr);

    proplinebreak.close();
    delete dataspacelinebreak;

    if (close) {
        entrylog->close();
        delete entrylog;
    }

    return entrylog;
}

DataSet* hdf5nexus::newNeXusChunkedMetadataDataSet(std::string location, H5::PredType predtype, std::string typestr, bool close) {
    // create metadata dataset
    hsize_t maxdimsmetadata[2]    = {H5S_UNLIMITED, 1};
    hsize_t chunk_maxdimsmetadata[2] = {1, 1};

    hsize_t dimsmetadata[2];
    dimsmetadata[0] = 0;
    dimsmetadata[1] = 1;

    DataSpace *dataspacemetadata = new DataSpace(2, dimsmetadata, maxdimsmetadata);
    // Modify dataset creation property to enable chunking
    DSetCreatPropList propmetadata;
    propmetadata.setChunk(2, chunk_maxdimsmetadata);

    // Create the chunked datasets.  Note the use of pointer.
    DataSet *entrymetadatabeamline_energy = new DataSet(file->createDataSet(location, predtype, *dataspacemetadata, propmetadata));

    propmetadata.close();
    delete dataspacemetadata;

    if (close) {
        entrymetadatabeamline_energy->close();
        delete entrymetadatabeamline_energy;
    }

    return entrymetadatabeamline_energy;
}

DataSet* hdf5nexus::newNeXusROIDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool close) {
    // set hdf settings for ROIs
    hsize_t roimaxdims[2]    = {(hsize_t)y, (hsize_t)x};
    hsize_t roichunk[2] = {1, (hsize_t)x};

    hsize_t dimsroi[2] = {0, (hsize_t)x};

    DataSpace *dataspaceroi = new DataSpace(2, dimsroi, roimaxdims);
    // Modify dataset creation property to enable chunking
    DSetCreatPropList proproi;
    proproi.setChunk(2, roichunk);

    DataSet *roidataset = new DataSet(file->createDataSet(location, PredType::STD_I32LE, *dataspaceroi, proproi));

    proproi.close();
    delete dataspaceroi;

    if (close) {
        roidataset->close();
        delete roidataset;
    }

    return roidataset;
}

void hdf5nexus::writeScanIndexData(int dataindex, int nopx, int stopx) {
    // WRITE BEAMLINE PARAMETER TO FILE
    hsize_t dimsext[2] = {1, 3}; // extend dimensions

    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/log/scanindex"));

    hsize_t offset[2];

    H5::ArrayType arrtype = dataset->getArrayType();

    hsize_t size[2];

    DataSpace *filespace = new DataSpace(dataset->getSpace());
    int n_dims = filespace->getSimpleExtentDims(size);

    offset[0] = size[0];
    offset[1] = 0;

    size[0]++;
    size[1] = 3;

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    int scanindexdata[3] = {dataindex, nopx, stopx};

    // Write data to the extended portion of the dataset.
    dataset->write(scanindexdata, PredType::STD_I32LE, *memspacenew, *filespacenew);

    std::cout<<"wrote scan index data to file, size: <<"<<size[0]<<", offset: "<<offset[0]<<std::endl;

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;
}

void hdf5nexus::writeLineBreakDataAndROIs(roidata ROImap, int dataindex, int nopx, int stopx, int scanX, int scanY) {
    // WRITE LINE BREAKS AND ROIS TO FILE
    hsize_t dimsext[2] = {1, 3}; // extend dimensions

    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/log/linebreaks"));

    hsize_t offset[2];

    H5::ArrayType arrtype = dataset->getArrayType();

    hsize_t size[2];

    DataSpace *filespace = new DataSpace(dataset->getSpace());
    int n_dims = filespace->getSimpleExtentDims(size);

    offset[0] = size[0];
    offset[1] = 0;

    size[0]++;
    size[1] = 3;

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    int scanindexdata[3] = {dataindex, nopx, stopx};

    // Write data to the extended portion of the dataset.
    dataset->write(scanindexdata, PredType::STD_I32LE, *memspacenew, *filespacenew);

    if ((nopx != 0) && (stopx == 0)) {
        stopx = scanY;
    }

    // write ROIs
    if ((nopx != 0) && (stopx != 0)) {
        auto const ROIkeys = ROImap.keys();
        for (std::string e : ROIkeys) {
            std::cout<<"writing ROI data for "<<e<<" to file..."<<std::endl;
            hsize_t dimsextroi[2] = {1, (hsize_t)scanX}; // extend dimensions
            DataSet *datasetroi = new DataSet(file->openDataSet("/measurement/instruments/sdd/roi/"+e));
            hsize_t sizeroi[2];
            hsize_t offsetroi[2];
            offsetroi[0] = stopx-1;
            offsetroi[1] = 0;

            sizeroi[0] = stopx;
            sizeroi[1] = (hsize_t)scanX;

            datasetroi->extend(sizeroi);
            // Select a hyperslab in extended portion of the dataset.
            DataSpace *filespacenewroi = new DataSpace(datasetroi->getSpace());

            filespacenewroi->selectHyperslab(H5S_SELECT_SET, dimsextroi, offsetroi);
            // Define memory space.
            DataSpace *memspacenewroi = new DataSpace(2, dimsextroi, NULL);

            QVector<uint32_t> roidata = ROImap[e];

            uint32_t writedata[scanX];

            std::cout<<"länge des datenarrays: "<<roidata.length()<<"stopx: "<<stopx<<std::endl;
            int pxcounter = 0;
            for (int i=scanX*(stopx-1); i<roidata.length(); i++) {
                writedata[pxcounter] = roidata.at(i);
                pxcounter++;
            }

            // Write data to the extended portion of the dataset.
            datasetroi->write(writedata, PredType::STD_I32LE, *memspacenewroi, *filespacenewroi);

            datasetroi->close();
            delete datasetroi;
            filespacenewroi->close();
            delete filespacenewroi;
            memspacenewroi->close();
            delete memspacenewroi;

        }
    }

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;
}

void hdf5nexus::createDataFile(QString filename, settingsdata settings) {
    std::cout<<"creating HDF5/NeXus file with filename \""<<filename.toStdString()<<"\"..."<<std::endl;

    std::string ROIdefinitions = settings.roidefinitions;

    int ccdX = settings.ccdWidth;
    int ccdY = settings.ccdHeight;

    int scanX = settings.scanWidth;
    int scanY = settings.scanHeight;

    file = new H5File(filename.toLocal8Bit(), H5F_ACC_TRUNC);

    unsigned majver;
    unsigned minnum;
    unsigned relnum;

    H5::H5Library::getLibVersion(majver, minnum, relnum);

    char versionstring[10];
    std::sprintf(versionstring, "%d.%d.%d", majver, minnum, relnum);

    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    newNeXusFileStringAttribute("HDF5_Version", versionstring);
    newNeXusFileStringAttribute("default", "measurement");
    newNeXusFileStringAttribute("detectorRank", "2");
    newNeXusFileStringAttribute("file_name", filename.toStdString());
    newNeXusFileStringAttribute("file_time", datetime.toStdString());
    newNeXusFileStringAttribute("nE", "1");
    newNeXusFileStringAttribute("nP", QString::number(settings.scanHeight*settings.scanWidth).toStdString());
    newNeXusFileStringAttribute("nX", QString::number(settings.scanWidth).toStdString());
    newNeXusFileStringAttribute("nY", QString::number(settings.scanHeight).toStdString());

    newNeXusGroup("/measurement", "NX_class", "NXentry", true);
    newNeXusScalarDataSet("/measurement/start_time", "NX_DATE_TIME", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/definition", "NX_CHAR", "NXstxm", true);
    newNeXusScalarDataSet("/measurement/title", "NX_CHAR", settings.scantitle, true);

    //newNeXusScalarDataSet("/measurement/monitor/data", "NX_FLOAT", 123, true);

    newNeXusGroup("/measurement/data", "NX_class", "NXdata", true);
    //newNeXusScalarDataSet("/measurement/data/energy", "NX_FLOAT", 100, true);
    newNeXusScalarDataSet("/measurement/data/sample_x", "NX_FLOAT", 99, true);
    newNeXusScalarDataSet("/measurement/data/sample_y", "NX_FLOAT", 87, true);
    newNeXusScalarDataSet("/measurement/data/stxm_scan_type", "NX_CHAR", "sample image", true);

    newNeXusGroup("/measurement/instruments", "NX_class", "NXinstrument", true);
    newNeXusGroup("/measurement/instruments/ccd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/sdd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/monochromator", "NX_class", "NXmonochromator", true);

    // create group for photodiode
    newNeXusGroup("/measurement/instruments/photodiode", "class", "NXdetector", true);
    newNeXusScalarDataSet("/measurement/instruments/photodiode/data", "NX_INT", 1, true);
    // link photodiode data with monitor data
    newNeXusGroup("/measurement/monitor", "NX_class", "NXmonitor", true);
    file->link(H5L_TYPE_HARD, "/measurement/instruments/photodiode/data", "/measurement/monitor/data");

    // create group for function generator
    newNeXusGroup("/measurement/instruments/function_generator", "class", "function generator", true);
    //newNeXusScalarDataSet("/measurement/instruments/function_generator/", "NX_INT", 1, true);

    //newNeXusScalarDataSet("/measurement/instruments/monochromator/energy", "NX_FLOAT", 11, true);

    newNeXusGroup("/measurement/instruments/source", "NX_class", "NXsource", true);
    newNeXusScalarDataSet("/measurement/instruments/source/name", "NX_CHAR", "PETRA III", true);
    newNeXusScalarDataSet("/measurement/instruments/source/probe", "NX_CHAR", "probe name", true);
    newNeXusScalarDataSet("/measurement/instruments/source/type", "NX_CHAR", "synchrotron", true);

    // sample group
    newNeXusGroup("/measurement/sample", "NX_class", "NXsample", true);

    // sample parameters
    newNeXusScalarDataSet("/measurement/sample/name", "NX_CHAR", settings.sample_name, true);
    newNeXusScalarDataSet("/measurement/sample/type", "NX_CHAR", settings.sample_type, true);
    newNeXusScalarDataSet("/measurement/sample/note", "NX_CHAR", settings.sample_note, true);
    newNeXusScalarDataSet("/measurement/sample/width", "NX_FLOAT", settings.sample_width, true);
    newNeXusScalarDataSet("/measurement/sample/height", "NX_FLOAT", settings.sample_height, true);
    newNeXusScalarDataSet("/measurement/sample/rotation_angle", "NX_FLOAT", settings.sample_rotation_angle, true);

    // sdd parameters
    newNeXusScalarDataSet("/measurement/instruments/sdd/sebitcount", "NX_INT", settings.sebitcount, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/filter", "NX_INT", settings.filter, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/energyrange", "NX_INT", settings.energyrange, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/tempmode", "NX_INT", settings.tempmode, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/zeropeakperiod", "NX_INT", settings.zeropeakperiod, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/acquisitionmode", "NX_INT", settings.acquisitionmode, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/checktemperature", "NX_INT", settings.checktemperature, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/sdd1", "NX_BOOLEAN", settings.sdd1, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/sdd2", "NX_BOOLEAN", settings.sdd2, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/sdd3", "NX_BOOLEAN", settings.sdd3, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/sdd4", "NX_BOOLEAN", settings.sdd4, true);

    // additional settings
    newNeXusGroup("/measurement/user", "NX_class", "NXuser", true);
    newNeXusScalarDataSet("/measurement/user/info", "NX_CHAR", settings.userdata, true);
    newNeXusScalarDataSet("/measurement/notes", "NX_CHAR", settings.notes, true);

    // ccd parameters
    // newNeXusScalarDataSet("/measurement/instruments/ccd/height", "NX_INT", settings.ccdHeight, true);
    // newNeXusScalarDataSet("/measurement/instruments/ccd/width", "NX_INT", settings.ccdWidth, true);

    newNeXusChunkedCCDDataSet("/measurement/instruments/ccd/data", ccdX, ccdY, PredType::STD_U16LE, "NX_INT", true);

    DataSet* transmissionpreviewdataset = newNeXusChunkedTransmissionPreviewDataSet("/measurement/data/data", PredType::STD_I64LE, "NX_INT", false);
    newNeXusDatasetStringAttribute(transmissionpreviewdataset, "signal", "1");
    transmissionpreviewdataset->close();
    delete transmissionpreviewdataset;

    newNeXusChunkedSpectraDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, "NX_INT", true);
    file->link(H5L_TYPE_HARD, "/measurement/instruments/sdd/data", "/measurement/data/sdd");

    newNeXusGroup("/measurement/instruments/sdd/log", "class", "log", true);

    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/scanindex", PredType::STD_I32LE, "scan index log", true);
    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/linebreaks", PredType::STD_I32LE, "line break log", true);

    // if ROIs are enabled (ROIdefinitions != ""), create datasets for ROIs
    if (ROIdefinitions != "") {
        // create log group
        newNeXusGroup("/measurement/instruments/sdd/roi", "class", "ROI", true);

        // create ROI datasets
        QJsonParseError jsonError;
        QJsonDocument document = QJsonDocument::fromJson(ROIdefinitions.c_str(), &jsonError);
        QJsonObject jsonObj = document.object();
        foreach(QString key, jsonObj.keys()) {
            std::string k(key.toLocal8Bit());
            // create ROI dataset
            newNeXusROIDataSet("/measurement/instruments/sdd/roi/"+k, scanX, scanY, PredType::STD_I32LE, k+" ROI", true);
        }
    }

    // create settings group and datasets
    // newNeXusGroup("/measurement/settings", "class", "measurement settings", true);
    // newNeXusScalarDataSet("/measurement/settings/scanWidth", "NX_INT", settings.scanWidth, true);
    // newNeXusScalarDataSet("/measurement/settings/scanHeight", "NX_INT", settings.scanHeight, true);

    // create metadata group and datasets
    newNeXusGroup("/measurement/metadata", "class", "metadata", true);
    //newNeXusChunkedSDDLogDataSet("/measurement/metadata/beamline_energy", PredType::STD_I32LE, "beamline energy", true);
    //newNeXusChunkedSDDLogDataSet("/measurement/metadata/acquisition_number", PredType::STD_I32LE, "acquisition number", true);

}

void hdf5nexus::openDataFile(QString fname) {
    file = new H5File(fname.toLocal8Bit(), H5F_ACC_RDWR);
}

void hdf5nexus::writeMetadata(metadata metadata) {
    // write current energy setpoint
    newNeXusScalarDataSet("/measurement/data/energy", "NX_FLOAT", metadata.set_energy, true);

    // write metadata
    newNeXusScalarDataSet("/measurement/metadata/acquisition_number", "NX_INT", metadata.acquisition_number, true);
    newNeXusScalarDataSet("/measurement/metadata/acquisition_time", "NX_CHAR", metadata.acquisition_time, true);
    newNeXusScalarDataSet("/measurement/metadata/beamline_energy", "NX_FLOAT", metadata.beamline_energy, true);
    newNeXusScalarDataSet("/measurement/metadata/ringcurrent", "NX_FLOAT", metadata.ringcurrent, true);
    newNeXusScalarDataSet("/measurement/metadata/horizontal_shutter", "NX_BOOLEAN", metadata.horizontal_shutter, true);
    newNeXusScalarDataSet("/measurement/metadata/vertical_shutter", "NX_BOOLEAN", metadata.vertical_shutter, true);
    newNeXusScalarDataSet("/measurement/instruments/monochromator/energy", "NX_FLOAT", metadata.beamline_energy, true);

    std::cout<<metadata.acquisition_number<<std::endl;
    std::cout<<metadata.acquisition_time<<std::endl;
    std::cout<<metadata.beamline_energy<<std::endl;
    std::cout<<metadata.ringcurrent<<std::endl;
    std::cout<<metadata.horizontal_shutter<<std::endl;
    std::cout<<metadata.vertical_shutter<<std::endl;
    std::cout<<metadata.set_energy<<std::endl;

    /* WRITE BEAMLINE PARAMETER TO FILE */
    /*
    hsize_t dimsext[2] = {1, 1}; // extend dimensions

    DataSet *dataset = new DataSet(file->openDataSet("/measurement/metadata/acquisition_number"));

    hsize_t offset[2];

    H5::ArrayType arrtype = dataset->getArrayType();

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespace = new DataSpace(dataset->getSpace());
    hsize_t size[2];
    int n_dims = filespace->getSimpleExtentDims(size);

    offset[0] = size[0];
    offset[1] = 0;

    size[0]++;
    size[1] = 1;

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    int acquisition_number = metadata.acquisition_number;

    // Write data to the extended portion of the dataset.
    dataset->write(&acquisition_number, PredType::STD_I32LE, *memspacenew, *filespacenew);

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;*/
}

void hdf5nexus::writeSDDData(int32_t pxnum, spectrumdata specdata) {
    hsize_t size[2];
    size[1] = 4096;

    hsize_t offset[2];
    offset[0] = pxnum;
    offset[1] = 0;

    // END PIXEL VALUES
    int32_t data[4096];
    for (int i=0;i<4096;i++) {
        data[i] = specdata.at(i);
    }

    // WRITE DETECTOR DATA TO FILE
    size[0] = pxnum+1;
    offset[0] = pxnum;

    hsize_t dimsext[2] = {1, 4096}; // extend dimensions

    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/data"));

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspace = new DataSpace(2, dimsext, NULL);

    // Write data to the extended portion of the dataset.
    dataset->write(data, PredType::STD_I32LE, *memspace, *filespace);

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    memspace->close();
    delete memspace;
}

void hdf5nexus::writeCCDSettings(int width, int height) {
    newNeXusScalarDataSet("/measurement/instruments/ccd/height", "NX_INT", height, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/width", "NX_INT", width, true);

    /*hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* settingsgroupccdWidth = new DataSet(file->openDataSet("/measurement/settings/ccdWidth"));
    int ccdWidth = width;
    settingsgroupccdWidth->write(&ccdWidth, PredType::STD_I32LE, fspace);
    settingsgroupccdWidth->close();
    delete settingsgroupccdWidth;

    DataSet* settingsgroupccdHeight = new DataSet(file->openDataSet("/measurement/settings/ccdHeight"));
    int ccdHeight = height;
    settingsgroupccdHeight->write(&ccdHeight, PredType::STD_I32LE, fspace);
    settingsgroupccdHeight->close();
    delete settingsgroupccdHeight;

    fspace.close();*/
}

void hdf5nexus::writeScanNote(std::string scannote) {
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* dataset = new DataSet(file->openDataSet("/measurement/notes"));
    dataset->write(scannote, str_type, att_space);
    fspace.close();

    dataset->close();
    delete dataset;
}
