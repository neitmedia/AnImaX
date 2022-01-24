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

DataSet* hdf5nexus::newNeXusScalarBooleanDataSet(std::string location, std::string type, bool content, bool close) {

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
}

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

DataSet* hdf5nexus::newNeXusChunkedCCDDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
    hsize_t dims[3];
    dims[0] = (hsize_t)x;
    dims[1] = (hsize_t)y;
    dims[2] = 0;

    hsize_t maxdims[3] = {(hsize_t)x, (hsize_t)y, H5S_UNLIMITED};
    hsize_t chunk_dims[3] = {(hsize_t)x, (hsize_t)y, 1};

    DataSpace *dataspace = new DataSpace(3, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);

    if (compression) {
        prop.setDeflate(compressionlevel);
    }

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

DataSet* hdf5nexus::newNeXusChunked1DDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
    hsize_t dims[1];
    dims[0] = 0;

    hsize_t maxdims[1] = {H5S_UNLIMITED};
    hsize_t chunk_dims[1] = {1};

    DataSpace *dataspace = new DataSpace(1, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(1, chunk_dims);
    if (compression) {
        prop.setDeflate(compressionlevel);
    }

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

DataSet* hdf5nexus::newNeXusChunkedTransmissionPreviewDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
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
    if (compression) {
        propsumimage.setDeflate(compressionlevel);
    }

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

DataSet* hdf5nexus::newNeXusChunkedSpectraDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
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
    if (compression) {
        propspectrum.setDeflate(compressionlevel);
    }

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

DataSet* hdf5nexus::newNeXusChunkedSDDLogDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
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
    if (compression) {
        proplinebreak.setDeflate(compressionlevel);
    }

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

DataSet* hdf5nexus::newNeXusROIDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {
    // set hdf settings for ROIs
    hsize_t roimaxdims[2]    = {(hsize_t)y, (hsize_t)x};
    hsize_t roichunk[2] = {1, (hsize_t)x};

    hsize_t dimsroi[2] = {0, (hsize_t)x};

    DataSpace *dataspaceroi = new DataSpace(2, dimsroi, roimaxdims);
    // Modify dataset creation property to enable chunking
    DSetCreatPropList proproi;
    proproi.setChunk(2, roichunk);
    if (compression) {
        proproi.setDeflate(compressionlevel);
    }

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

    int ccdX = settings.ccd_width;
    int ccdY = settings.ccd_height;

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

    Group* measurement_group = newNeXusGroup("/measurement", "NX_class", "NXentry", false);
    newNeXusGroupStringAttribute(measurement_group, "default", "transmission");
    measurement_group->close();
    delete(measurement_group);
    newNeXusScalarDataSet("/measurement/start_time", "NX_DATE_TIME", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/definition", "NX_CHAR", "NXstxm", true);
    newNeXusScalarDataSet("/measurement/scan_type", "NX_CHAR", settings.scantype, true);
    newNeXusScalarDataSet("/measurement/scan_width", "NX_CHAR", settings.scanWidth, true);
    newNeXusScalarDataSet("/measurement/scan_height", "NX_CHAR", settings.scanHeight, true);

    newNeXusScalarDataSet("/measurement/title", "NX_CHAR", settings.scantitle, true);
    newNeXusScalarDataSet("/measurement/x_step_size", "NX_FLOAT", settings.x_step_size, true);
    newNeXusScalarDataSet("/measurement/y_step_size", "NX_FLOAT", settings.y_step_size, true);

    Group* transmission_group = newNeXusGroup("/measurement/transmission", "NX_class", "NXdata", false);
    newNeXusGroupStringAttribute(transmission_group, "signal", "data");
    newNeXusGroupStringAttribute(transmission_group, "axes", "[\"sample_x\",\"sample_y\"]");
    transmission_group->close();
    delete(transmission_group);

    DataSet* transmission_sample_x_ds = newNeXusChunked1DDataSet("/measurement/transmission/sample_x", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(transmission_sample_x_ds, "axis", "0");
    transmission_sample_x_ds->close();
    delete(transmission_sample_x_ds);
    DataSet* transmission_sample_y_ds = newNeXusChunked1DDataSet("/measurement/transmission/sample_y", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(transmission_sample_y_ds, "axis", "1");
    transmission_sample_y_ds->close();
    delete(transmission_sample_y_ds);

    newNeXusScalarDataSet("/measurement/transmission/stxm_scan_type", "NX_CHAR", "sample image", true);

    Group* fluorescence_group = newNeXusGroup("/measurement/fluorescence", "NX_class", "NXdata", false);
    newNeXusGroupStringAttribute(fluorescence_group, "signal", "data");
    newNeXusGroupStringAttribute(fluorescence_group, "axes", "[\".\",\".\"]");
    fluorescence_group->close();
    delete(fluorescence_group);

    DataSet* fluorescence_sample_x_ds = newNeXusChunked1DDataSet("/measurement/fluorescence/sample_x", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(fluorescence_sample_x_ds, "axis", "0");
    fluorescence_sample_x_ds->close();
    delete(fluorescence_sample_x_ds);

    DataSet* fluorescence_sample_y_ds = newNeXusChunked1DDataSet("/measurement/fluorescence/sample_y", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(fluorescence_sample_y_ds, "axis", "1");
    fluorescence_sample_y_ds->close();
    delete(fluorescence_sample_y_ds);
    //file->link(H5L_TYPE_HARD, "/measurement/transmission/sample_x", "/measurement/fluorescence/sample_x");
    //file->link(H5L_TYPE_HARD, "/measurement/transmission/sample_y", "/measurement/fluorescence/sample_y");

    newNeXusScalarDataSet("/measurement/fluorescence/stxm_scan_type", "NX_CHAR", "generic scan", true);

    newNeXusGroup("/measurement/instruments", "NX_class", "NXinstrument", true);
    newNeXusGroup("/measurement/instruments/ccd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/sdd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/monochromator", "NX_class", "NXmonochromator", true);
    newNeXusGroup("/measurement/instruments/beamline", "NX_class", "NXcollection", true);

    // TODO: if interferometer is included into system, create dataset and write interferometer values for every scan point
    // up to then: empty data sets
    newNeXusGroup("/measurement/instruments/sample_x", "NX_class", "NXdetector", true);
    newNeXusChunked1DDataSet("/measurement/instruments/sample_x/data", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, true);
    newNeXusGroup("/measurement/instruments/sample_y", "NX_class", "NXdetector", true);
    newNeXusChunked1DDataSet("/measurement/instruments/sample_y/data", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, true);

    // create group for photodiode
    newNeXusGroup("/measurement/instruments/photodiode", "NX_class", "NXdetector", true);
    newNeXusScalarDataSet("/measurement/instruments/photodiode/data", "NX_INT", 1, true);
    // link photodiode data with monitor data
    // monitor data must have the same dimensions as data matrices => include, when monitoring is included into system (photodiode or electron flux)
    // newNeXusGroup("/measurement/monitor", "NX_class", "NXmonitor", true);
    // file->link(H5L_TYPE_HARD, "/measurement/instruments/photodiode/data", "/measurement/monitor/data");

    // create group source parameters
    newNeXusGroup("/measurement/instruments/source", "NX_class", "NXsource", true);
    newNeXusScalarDataSet("/measurement/instruments/source/name", "NX_CHAR", settings.source_name, true);
    newNeXusScalarDataSet("/measurement/instruments/source/probe", "NX_CHAR", settings.source_probe, true);
    newNeXusScalarDataSet("/measurement/instruments/source/type", "NX_CHAR", settings.source_type, true);

    // sample group
    newNeXusGroup("/measurement/sample", "NX_class", "NXsample", true);

    // sample parameters
    newNeXusScalarDataSet("/measurement/sample/name", "NX_CHAR", settings.sample_name, true);
    newNeXusScalarDataSet("/measurement/sample/type", "NX_CHAR", settings.sample_type, true);
    newNeXusScalarDataSet("/measurement/sample/note", "NX_CHAR", settings.sample_note, true);
    newNeXusScalarDataSet("/measurement/sample/width", "NX_FLOAT", settings.sample_width, true);
    newNeXusScalarDataSet("/measurement/sample/height", "NX_FLOAT", settings.sample_height, true);
    newNeXusScalarDataSet("/measurement/sample/rotation_angle", "NX_FLOAT", settings.sample_rotation_angle, true);

    // ccd parameters
    newNeXusGroup("/measurement/instruments/ccd/settings", "NX_class", "NXcollection", true);
    newNeXusGroup("/measurement/instruments/ccd/settings/set", "NX_class", "NXcollection", true);
    newNeXusGroup("/measurement/instruments/ccd/settings/calculated", "NX_class", "NXcollection", true);

    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/binning_x", "NX_INT", settings.ccd_binning_x, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/binning_y", "NX_INT", settings.ccd_binning_y, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/ccd_height", "NX_INT", settings.ccd_height, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/ccd_width", "NX_INT", settings.ccd_width, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/pixelcount", "NX_INT", settings.ccd_pixelcount, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/frametransfer_mode", "NX_INT", settings.ccd_frametransfer_mode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/number_of_accumulations", "NX_INT", settings.ccd_number_of_accumulations, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/number_of_scans", "NX_INT", settings.ccd_number_of_scans, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/set_kinetic_cycle_time", "NX_FLOAT", settings.ccd_set_kinetic_cycle_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/read_mode", "NX_INT", settings.ccd_read_mode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/acquisition_mode", "NX_INT", settings.ccd_acquisition_mode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/shutter_mode", "NX_INT", settings.ccd_shutter_mode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/shutter_output_signal", "NX_INT", settings.ccd_shutter_output_signal, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/shutter_open_time", "NX_FLOAT", settings.ccd_shutter_open_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/shutter_close_time", "NX_FLOAT", settings.ccd_shutter_close_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/triggermode", "NX_INT", settings.ccd_triggermode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/exposure_time", "NX_FLOAT", settings.ccd_exposure_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/accumulation_time", "NX_FLOAT", settings.ccd_accumulation_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/kinetic_time", "NX_FLOAT", settings.ccd_kinetic_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/min_temp", "NX_INT", settings.ccd_min_temp, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/max_temp", "NX_INT", settings.ccd_max_temp, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/target_temp", "NX_INT", settings.ccd_target_temp, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/pre_amp_gain", "NX_INT", settings.ccd_pre_amp_gain, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/em_gain_mode", "NX_INT", settings.ccd_em_gain_mode, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/set/em_gain", "NX_INT", settings.ccd_em_gain, true);

    newNeXusGroup("/measurement/instruments/sdd/settings", "NX_class", "NXcollection", true);
    // sdd parameters
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/sebitcount", "NX_INT", settings.sdd_sebitcount, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/filter", "NX_INT", settings.sdd_filter, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/energyrange", "NX_INT", settings.sdd_energyrange, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/tempmode", "NX_INT", settings.sdd_tempmode, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/zeropeakperiod", "NX_INT", settings.sdd_zeropeakperiod, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/acquisitionmode", "NX_INT", settings.sdd_acquisitionmode, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/checktemperature", "NX_INT", settings.sdd_checktemperature, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/sdd1", "NX_BOOLEAN", settings.sdd_sdd1, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/sdd2", "NX_BOOLEAN", settings.sdd_sdd2, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/sdd3", "NX_BOOLEAN", settings.sdd_sdd3, true);
    newNeXusScalarDataSet("/measurement/instruments/sdd/settings/sdd4", "NX_BOOLEAN", settings.sdd_sdd4, true);

    // additional settings
    // newNeXusGroup("/measurement/user", "NX_class", "NXuser", true);
    //
    if (settings.userdata != "") {
        try {
            QJsonParseError user_jsonError;
            QJsonDocument user_document = QJsonDocument::fromJson(settings.userdata.c_str(), &user_jsonError);
            QJsonArray user_jsonArray = user_document.array();
            for (int i_c=0; i_c < user_jsonArray.count(); i_c++) {
                QJsonObject userobj = user_jsonArray.at(i_c).toObject();

                newNeXusGroup("/measurement/user_"+QString::number(i_c).toStdString(), "NX_class", "NXuser", true);

                if (userobj.contains("name")) {
                    QString name = userobj.take("name").toString();
                    if (name != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/name", "NX_CHAR", name.toStdString(), true);
                    }
                }

                if (userobj.contains("role")) {
                    QString role = userobj.take("role").toString();
                    if (role != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/role", "NX_CHAR", role.toStdString(), true);
                    }
                }

                if (userobj.contains("affiliation")) {
                    QString affiliation = userobj.take("affiliation").toString();
                    if (affiliation != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/affiliation", "NX_CHAR", affiliation.toStdString(), true);
                    }
                }

                if (userobj.contains("address")) {
                    QString address = userobj.take("address").toString();
                    if (address != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/address", "NX_CHAR", address.toStdString(), true);
                    }
                }

                if (userobj.contains("telephone_number")) {
                    QString telephone_number = userobj.take("telephone_number").toString();
                    if (telephone_number != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/telephone_number", "NX_CHAR", telephone_number.toStdString(), true);
                    }
                }

                if (userobj.contains("fax_number")) {
                    QString fax_number = userobj.take("fax_number").toString();
                    if (fax_number != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/fax_number", "NX_CHAR", fax_number.toStdString(), true);
                    }
                }

                if (userobj.contains("email")) {
                    QString email = userobj.take("email").toString();
                    if (email != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/email", "NX_CHAR", email.toStdString(), true);
                    }
                }

                if (userobj.contains("facility_user_id")) {
                    QString facility_user_id = userobj.take("facility_user_id").toString();
                    if (facility_user_id != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/facility_user_id", "NX_CHAR", facility_user_id.toStdString(), true);
                    }
                }

                if (userobj.contains("ORCID")) {
                    QString ORCID = userobj.take("ORCID").toString();
                    if (ORCID != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/ORCID", "NX_CHAR", ORCID.toStdString(), true);
                    }
                }
            }
        }  catch (const std::exception& ex) {
            std::cout<<"error parsing user data json, ignoring user data.."<<std::endl;
        }
    }

    newNeXusGroup("/measurement/pre_scan_note", "NX_class", "NXnote", true);
    cd = QDate::currentDate();
    ct = QTime::currentTime();

    datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);
    newNeXusScalarDataSet("/measurement/pre_scan_note/date", "NX_CHAR", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/pre_scan_note/description", "NX_CHAR", settings.notes, true);

    newNeXusChunkedCCDDataSet("/measurement/instruments/ccd/data", ccdX, ccdY, PredType::STD_U16LE, "NX_INT", settings.file_compression, settings.file_compression_level, true);

    DataSet* transmissionpreviewdataset = newNeXusChunkedTransmissionPreviewDataSet("/measurement/transmission/data", PredType::STD_I64LE, "NX_INT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(transmissionpreviewdataset, "signal", "1");
    newNeXusDatasetStringAttribute(transmissionpreviewdataset, "axes", "[\"sample_x\",\"sample_y\"]");
    transmissionpreviewdataset->close();
    delete transmissionpreviewdataset;

    DataSet* fluorescencedataset = newNeXusChunkedSpectraDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, "NX_INT", settings.file_compression, settings.file_compression_level, false);
    newNeXusDatasetStringAttribute(fluorescencedataset, "signal", "1");
    fluorescencedataset->close();
    delete fluorescencedataset;

    file->link(H5L_TYPE_HARD, "/measurement/instruments/sdd/data", "/measurement/fluorescence/data");

    newNeXusGroup("/measurement/instruments/sdd/log", "NX_class", "NXcollection", true);

    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/scanindex", PredType::STD_I32LE, "scan index log", settings.file_compression, settings.file_compression_level, true);
    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/linebreaks", PredType::STD_I32LE, "line break log", settings.file_compression, settings.file_compression_level, true);

    // if ROIs are enabled (ROIdefinitions != ""), create datasets for ROIs
    if (ROIdefinitions != "") {
        // create log group
        newNeXusGroup("/measurement/instruments/sdd/roi", "NX_class", "NXcollection", true);

        // create ROI datasets
        QJsonParseError jsonError;
        QJsonDocument document = QJsonDocument::fromJson(ROIdefinitions.c_str(), &jsonError);
        QJsonObject jsonObj = document.object();
        foreach(QString key, jsonObj.keys()) {
            std::string k(key.toLocal8Bit());
            // create ROI dataset
            newNeXusROIDataSet("/measurement/instruments/sdd/roi/"+k, scanX, scanY, PredType::STD_I32LE, k+" ROI", settings.file_compression, settings.file_compression_level, true);
        }
    }
}

void hdf5nexus::openDataFile(QString fname) {
    file = new H5File(fname.toLocal8Bit(), H5F_ACC_RDWR);
}

void hdf5nexus::writeMetadata(metadata metadata) {
    // write current energy setpoint
    newNeXusScalarDataSet("/measurement/transmission/energy", "NX_FLOAT", metadata.set_energy, true); 
    file->link(H5L_TYPE_HARD, "/measurement/transmission/energy", "/measurement/fluorescence/energy");
    file->link(H5L_TYPE_HARD, "/measurement/transmission/energy", "/measurement/instruments/monochromator/energy");

    // write metadata
    newNeXusScalarDataSet("/measurement/acquisition_number", "NX_INT", metadata.acquisition_number, true);
    newNeXusScalarDataSet("/measurement/acquisition_time", "NX_DATE_TIME", metadata.acquisition_time, true);
    newNeXusScalarDataSet("/measurement/instruments/source/current", "NX_FLOAT", metadata.ringcurrent, true);
    newNeXusScalarBooleanDataSet("/measurement/instruments/beamline/horizontal_shutter", "NX_BOOLEAN", metadata.horizontal_shutter, true);
    newNeXusScalarBooleanDataSet("/measurement/instruments/beamline/vertical_shutter", "NX_BOOLEAN", metadata.vertical_shutter, true);
    newNeXusScalarDataSet("/measurement/instruments/beamline/energy", "NX_FLOAT", metadata.beamline_energy, true);

    std::cout<<metadata.acquisition_number<<std::endl;
    std::cout<<metadata.acquisition_time<<std::endl;
    std::cout<<metadata.beamline_energy<<std::endl;
    std::cout<<metadata.ringcurrent<<std::endl;
    std::cout<<metadata.horizontal_shutter<<std::endl;
    std::cout<<metadata.vertical_shutter<<std::endl;
    std::cout<<metadata.set_energy<<std::endl;
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

void hdf5nexus::writeCCDSettings(ccdsettings ccdsettingsdata) {
    // write calculated ccd settings to data file
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/set_kinetic_cycle_time", "NX_FLOAT", ccdsettingsdata.set_kinetic_cycle_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/exposure_time", "NX_FLOAT", ccdsettingsdata.exposure_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/accumulation_time", "NX_FLOAT", ccdsettingsdata.accumulation_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/kinetic_time", "NX_FLOAT", ccdsettingsdata.kinetic_time, true);
}

void hdf5nexus::writeScanNote(std::string scannote, int notecounter) {
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    newNeXusGroup("/measurement/post_scan_note_"+QString::number(notecounter).toStdString(), "NX_class", "NXnote", true);
    newNeXusScalarDataSet("/measurement/post_scan_note_"+QString::number(notecounter).toStdString()+"/date", "NX_CHAR", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/post_scan_note_"+QString::number(notecounter).toStdString()+"/description", "NX_CHAR", scannote, true);
}

void hdf5nexus::appendValueTo1DDataSet(std::string location, int position, float value) {
    // write data to file
    hsize_t size[1];

    hsize_t offset[1];

    float dataarr[1];
    dataarr[0] = value;

    // WRITE DETECTOR DATA TO FILE
    size[0] = position+1;
    offset[0] = position;

    hsize_t dimsext[1] = {1}; // extend dimensions

    DataSet *dataset = new DataSet(this->file->openDataSet(location));
    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspace = new DataSpace(1, dimsext, NULL);

    // Write data to the extended portion of the dataset.
    dataset->write(dataarr, PredType::NATIVE_FLOAT, *memspace, *filespace);

    filespace->close();
    delete(filespace);
    memspace->close();
    delete(memspace);
    dataset->close();
    delete(dataset);
}
