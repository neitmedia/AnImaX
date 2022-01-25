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

// hdf5nexus constructor
hdf5nexus::hdf5nexus()
{

    // register metatypes to be able to use them as function parameters
    qRegisterMetaType<roidata>( "roidata" );
    qRegisterMetaType<spectrumdata>( "spectrumdata" );

}

// function that closes the opened datafile
void hdf5nexus::closeDataFile() {

    // close file
    file->close();
    // free allocated memory space
    delete file;

}

// function that writes the current timestamp to the end_time dataset
void hdf5nexus::writeEndTimeStamp() {

    // get current timestamp
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();
    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    // create new dataset "/measurement/end_time" and write value to it
    newNeXusScalarDataSet("/measurement/end_time", "NX_DATE_TIME", datetime.toStdString(), true);

}

// wrapper function that creates a new attribute (file level)
void hdf5nexus::newNeXusAttribute(std::string location, std::string content) {

    // declare and set string type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);

    // declare dataspace for the attribute
    DataSpace att_space(H5S_SCALAR);

    // create file attribute
    Attribute att = file->createAttribute(location, str_type, att_space);

    // write string value to attribute
    att.write(str_type, std::string(content));

    // close objects
    att.close();
    str_type.close();
    att_space.close();

}

// wrapper function that creates a new attribute (group level), 1st function overloading
void hdf5nexus::newNeXusAttribute(Group* group, std::string location, std::string content) {

    // declare and set string type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);

    // declare dataspace for the attribute
    DataSpace att_space(H5S_SCALAR);

    // create group attribute
    Attribute att = group->createAttribute(location, str_type, att_space);

    // write string value to attribute
    att.write(str_type, std::string(content));

    // close objects
    att.close();
    str_type.close();
    att_space.close();

}

// wrapper function that creates a new attribute (dataset level), 2nd function overloading
void hdf5nexus::newNeXusAttribute(DataSet* dataset, std::string location, std::string content) {

    // declare and set string type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);

    // declare dataspace for the attribute
    DataSpace att_space(H5S_SCALAR);

    // create dataset attribute
    Attribute att = dataset->createAttribute(location, str_type, att_space);

    // write string value to attribute
    att.write(str_type, std::string(content));

    // close objects
    att.close();
    str_type.close();
    att_space.close();
}

// wrapper function that creates a new scalar data set and writes a boolean value to it
DataSet* hdf5nexus::newNeXusScalarBooleanDataSet(std::string location, std::string type, bool content, bool close) {

    // declare and set dataspace and dimensions
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    // create new dataset
    DataSet* dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_HBOOL, fspace));

    // add "type" attribute to data set
    newNeXusAttribute(dataset, "type", type);

    // write data to dataset
    dataset->write(&content, PredType::NATIVE_HBOOL, att_space);

    // close objects
    att_space.close();
    fspace.close();

    // if closed flag is set true
    if (close) {

        // close data set
        dataset->close();

        // free memory
        delete dataset;

    }

    return dataset;
}

// wrapper function that creates a new scalar data set and writes a string value to it
DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, std::string content, bool close) {

    // declare and set data type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);

    // set charset
    str_type.setCset(H5T_CSET_UTF8);

    // declare and set dataspace and dimensions
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    // create new dataset
    DataSet* dataset = new DataSet(file->createDataSet(location, str_type, fspace));

    // add "type" attribute to data set
    newNeXusAttribute(dataset, "type", type);

    // write data to dataset
    dataset->write(std::string(content), str_type, att_space);

    // close objects
    str_type.close();
    att_space.close();
    fspace.close();

    // close flag has been set
    if (close) {

        // close dataset
        dataset->close();

        // delete dataset
        delete dataset;

    }

    return dataset;
}

// wrapper function that creates a new scalar data set and writes a float value to it
DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, float content, bool close) {

    // declare and set data type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);

    // set charset
    str_type.setCset(H5T_CSET_UTF8);

    // declare and set dataspace and dimensions
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    // declare array for data and write data into it
    float contentarr[] = {content};

    // create new dataset
    DataSet* dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_FLOAT, fspace));

    // add attribute to dataset
    newNeXusAttribute(dataset, "type", type);

    // write data to dataset
    dataset->write(contentarr, PredType::NATIVE_FLOAT, att_space);

    // close objects
    str_type.close();
    att_space.close();
    fspace.close();

    // if close flag has been set
    if (close) {

        // close dataset
        dataset->close();

        // delete dataset to free memory space
        delete dataset;
    }

    return dataset;
}

// wrapper function that creates a new scalar data set and writes a int32_t value to it
DataSet* hdf5nexus::newNeXusScalarDataSet(std::string location, std::string type, int32_t content, bool close) {

    // declare and set data type
    StrType str_type(PredType::C_S1, H5T_VARIABLE);

    // set charset
    str_type.setCset(H5T_CSET_UTF8);

    // declare and set dataspace and dimensions
    DataSpace att_space(H5S_SCALAR);
    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    // create dataset object pointer
    DataSet* dataset;

    // if type == "NX_FLOAT"
    if (type == "NX_FLOAT") {

        // declare data array and write data into it
        float contentarr[] = {(float)content};

        // create dataset
        dataset = new DataSet(file->createDataSet(location, PredType::NATIVE_FLOAT, fspace));

        // add "type" attribute
        newNeXusAttribute(dataset, "type", type);

        // write data to dataset
        dataset->write(contentarr, PredType::NATIVE_FLOAT, att_space);

    } else {

        // declare data array and write data into it
        int32_t contentarr[] = {content};

        // create dataset
        dataset = new DataSet(file->createDataSet(location, PredType::STD_I32LE, fspace));

        // add "type" attribute
        newNeXusAttribute(dataset, "type", type);

        // write data to dataset
        dataset->write(contentarr, PredType::STD_I32LE, att_space);
    }

    // close objects
    str_type.close();
    att_space.close();
    fspace.close();

    // if close flag is true
    if (close) {

        // close dataset
        dataset->close();

        // delete dataset
        delete dataset;

    }

    return dataset;
}

// wrapper function that creates a new HDF5/NeXus group
Group* hdf5nexus::newNeXusGroup(std::string location, std::string attrname, std::string attrcontent, bool close) {

    // create new group
    Group* group = new Group(file->createGroup(location));

    // add attribute
    newNeXusAttribute(group, attrname, attrcontent);

    // if close flag is true, close group
    if (close) {

        // close group
        group->close();

        // delete group
        delete group;

    }

    return group;
}

// wrapper function that creates a new three dimensional chunked CCD data set
DataSet* hdf5nexus::newNeXusChunkedCCDDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // create and set dimension array
    hsize_t dims[3];

    // first dimension of dimension array is the width of the ccd image
    dims[0] = (hsize_t)x;

    // second dimension of dimension array is the height of the ccd image
    dims[1] = (hsize_t)y;

    // third dimension is set to 0 -> empty dataset
    dims[2] = 0;

    // set maximum dimensions to ccdX, ccdY, unlimited
    hsize_t maxdims[3] = {(hsize_t)x, (hsize_t)y, H5S_UNLIMITED};

    // set chunk dimensions, every ccd image (x,y) is one chunk
    hsize_t chunk_dims[3] = {(hsize_t)x, (hsize_t)y, 1};

    // create dataspace with specified dimensions
    DataSpace *dataspace = new DataSpace(3, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);

    // if compression flag is true
    if (compression) {
        // set compression level
        prop.setDeflate(compressionlevel);
    }

    // create the chunked dataset
    DataSet *dataset = new DataSet(file->createDataSet(location, predtype, *dataspace, prop));

    // add "type" attribute
    newNeXusAttribute(dataset, "type", typestr);

    // close objects
    prop.close();
    dataspace->close();

    // delete dataspace
    delete dataspace;

    // if close flag is true, close and delete dataset
    if (close) {
        dataset->close();
        delete dataset;
    }

    return dataset;
}

// wrapper function that creates a one dimensional dataset
DataSet* hdf5nexus::newNeXusChunked1DDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // one dimension => dims[1]
    hsize_t dims[1];

    // empty dataset => dims[0] = 0
    dims[0] = 0;

    // set maxdims to unlimited
    hsize_t maxdims[1] = {H5S_UNLIMITED};

    // chunk_dims is set to 1
    hsize_t chunk_dims[1] = {1};

    // create the dataset
    DataSpace *dataspace = new DataSpace(1, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(1, chunk_dims);

    // if compression flag is true
    if (compression) {
        // set compression level
        prop.setDeflate(compressionlevel);
    }

    // Create the chunked dataset.
    DataSet *dataset = new DataSet(file->createDataSet(location, predtype, *dataspace, prop));
    newNeXusAttribute(dataset, "type", typestr);

    // close objects
    prop.close();
    dataspace->close();

    // delete dataspace
    delete dataspace;

    // if close flag is true
    if (close) {

        // close and delete dataset
        dataset->close();
        delete dataset;

    }

    return dataset;
}

// wrapper function that creates a two dimensional dataset (needed for transmission image)
DataSet* hdf5nexus::newNeXusChunkedTransmissionPreviewDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // set both dimensions to unlimited
    hsize_t maxdimssumimage[2]    = {H5S_UNLIMITED, H5S_UNLIMITED};

    // set chunking to 1x1
    hsize_t chunk_dimssumimage[2] = {1, 1};

    // set dimensions to 0x0 => empty dataset
    hsize_t dimssumimage[2];
    dimssumimage[0] = 0;
    dimssumimage[1] = 0;

    // create dataspace
    DataSpace *dataspacesumimage = new DataSpace(2, dimssumimage, maxdimssumimage);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList propsumimage;
    propsumimage.setChunk(2, chunk_dimssumimage);

    // if compression flag is set
    if (compression) {

        // set compression level
        propsumimage.setDeflate(compressionlevel);

    }

    // create dataset
    DataSet *datasetsumimage = new DataSet(file->createDataSet(location, predtype, *dataspacesumimage, propsumimage));

    // add "type" attribute
    newNeXusAttribute(datasetsumimage, "type", typestr);

    // close objects
    propsumimage.close();
    dataspacesumimage->close();

    // delete dataspacesumimage
    delete dataspacesumimage;

    // if close flag is true
    if (close) {
        // close datasetsumimage
        datasetsumimage->close();
        // delete datasetsumimage
        delete datasetsumimage;
    }

    return datasetsumimage;
}

// wrapper function that creates a two dimensional fluorescence dataset (needed for fluorescence data)
DataSet* hdf5nexus::newNeXusChunkedSpectraDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // set dimensions (0x4096)
    hsize_t dimsspec[2];
    dimsspec[0] = 0;
    dimsspec[1] = 4096;

    // set size (1x4096)
    hsize_t sizespec[2];
    sizespec[0] = 1;
    sizespec[1] = 4096;

    // set max dims to (unlimited x 4096)
    hsize_t maxdimsspec[2] = {H5S_UNLIMITED, 4096};

    // set chunk size (1 x 4096)
    hsize_t chunk_dimsspec[2] = {1, 4096};

    // create dataspace
    DataSpace *dataspacespectrum = new DataSpace(2, dimsspec, maxdimsspec);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList propspectrum;
    propspectrum.setChunk(2, chunk_dimsspec);

    // if compression flag is true
    if (compression) {

        // set compression level
        propspectrum.setDeflate(compressionlevel);

    }

    // Create the chunked dataset.  Note the use of pointer.
    DataSet *entryfluoinstrumentfluorescencedata = new DataSet(file->createDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, *dataspacespectrum, propspectrum));

    // add "type" attribute
    newNeXusAttribute(entryfluoinstrumentfluorescencedata, "type", "NX_FLOAT");

    // close objects
    propspectrum.close();
    dataspacespectrum->close();

    // delete dataspacespectrum
    delete dataspacespectrum;

    // if close flag is true
    if (close) {

        // close entryfluoinstrumentfluorescencedata
        entryfluoinstrumentfluorescencedata->close();

        // delete entryfluoinstrumentfluorescencedata
        delete entryfluoinstrumentfluorescencedata;

    }

    return entryfluoinstrumentfluorescencedata;
}

// wrapper function that creates a dataset for sdd logs (scan index log / line break log)
DataSet* hdf5nexus::newNeXusChunkedSDDLogDataSet(std::string location, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // set maximum dimensions to (unlimited x 3)
    hsize_t maxdimslinebreak[2]    = {H5S_UNLIMITED, 3};

    // set chunking to (1 x 3)
    hsize_t chunk_dimslinebreak[2] = {1, 3};

    // set dimensions (0 x 3) => empty dataset
    hsize_t dimslinebreak[2];
    dimslinebreak[0] = 0;
    dimslinebreak[1] = 3;

    // create dataspace
    DataSpace *dataspacelinebreak = new DataSpace(2, dimslinebreak, maxdimslinebreak);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList proplinebreak;
    proplinebreak.setChunk(2, chunk_dimslinebreak);

    // if compression flag is true
    if (compression) {

        // set compression level
        proplinebreak.setDeflate(compressionlevel);


    }

    // create the chunked dataset
    DataSet *entrylog = new DataSet(file->createDataSet(location, predtype, *dataspacelinebreak, proplinebreak));

    // add "type" attribute
    newNeXusAttribute(entrylog, "type", typestr);

    // close objects
    proplinebreak.close();
    dataspacelinebreak->close();

    // delete dataspacelinebreak
    delete dataspacelinebreak;

    // if close flag is true
    if (close) {

        // close entrylog
        entrylog->close();

        // delete entrylog
        delete entrylog;
    }

    return entrylog;
}

// wrapper function that creates a ROI dataset
DataSet* hdf5nexus::newNeXusROIDataSet(std::string location, int x, int y, H5::PredType predtype, std::string typestr, bool compression, int compressionlevel, bool close) {

    // set max dims (Y x X)
    hsize_t roimaxdims[2]    = {(hsize_t)y, (hsize_t)x};

    // set chunking (1 x X)
    hsize_t roichunk[2] = {1, (hsize_t)x};

    // set dimensions (0 x X) => empty dataset
    hsize_t dimsroi[2] = {0, (hsize_t)x};

    // create dataspace
    DataSpace *dataspaceroi = new DataSpace(2, dimsroi, roimaxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList proproi;
    proproi.setChunk(2, roichunk);

    // if compression flag is true
    if (compression) {

        // set compression level
        proproi.setDeflate(compressionlevel);

    }

    // create chunked dataset
    DataSet *roidataset = new DataSet(file->createDataSet(location, PredType::STD_I32LE, *dataspaceroi, proproi));

    // close objects
    proproi.close();
    dataspaceroi->close();

    // delete dataspaceroi
    delete dataspaceroi;

    // if close flag is true
    if (close) {

        // close roidataset
        roidataset->close();

        // delete roidataset
        delete roidataset;
    }

    return roidataset;
}

// wrapper function that writes scan index data to data file
void hdf5nexus::writeScanIndexData(long dataindex, int nopx, int stopx) {

    // extend dimensions by (1 x 3)
    hsize_t dimsext[2] = {1, 3}; // extend dimensions

    // open dataset
    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/log/scanindex"));

    // declare offset array
    hsize_t offset[2];

    // get array type of dataset
    H5::ArrayType arrtype = dataset->getArrayType();

    // declare size array
    hsize_t size[2];

    // create filespace pointer
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    // get dimensions of filespace
    filespace->getSimpleExtentDims(size);

    // append data, so offset[0] has to be the old size
    offset[0] = size[0];

    // offset[1] = 0 => no offset in second dimension
    offset[1] = 0;

    // increment first dimension size
    size[0]++;

    // no change of second dimension
    size[1] = 3;

    // extend dataset
    dataset->extend(size);

    // create dataspace pointer to extended dataset
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    // select a hyperslab in extended portion of the dataset
    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // define memory space
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    // declare and set data array
    long scanindexdata[3] = {dataindex, nopx, stopx};

    // Write data to the extended portion of the dataset.
    dataset->write(scanindexdata, PredType::STD_I64LE, *memspacenew, *filespacenew);

    // give out some debug infos
    std::cout<<"wrote scan index data to file, size: <<"<<size[0]<<", offset: "<<offset[0]<<std::endl;

    // close and delete objects
    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;
}

// wrapper function that writes line break logs and ROIs to data file
void hdf5nexus::writeLineBreakDataAndROIs(roidata ROImap, long dataindex, int nopx, int stopx, int scanX, int scanY) {

    /* BEGIN WRITING LINE BREAK LOG DATA */

    // line break log will be extended by (1 x 3) matrix
    hsize_t dimsext[2] = {1, 3}; // extend dimensions

    // open dataset
    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/log/linebreaks"));

    // declare offset variable
    hsize_t offset[2];

    // get array type
    H5::ArrayType arrtype = dataset->getArrayType();

    // declare size array
    hsize_t size[2];

    // create filespace pointer
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    // get dataset dimensions
    filespace->getSimpleExtentDims(size);

    // append data, so offset[0] has to be the old size
    offset[0] = size[0];

    // offset[1] = da => no offset in second dimension
    offset[1] = 0;

    // increment first dimension size
    size[0]++;

    // no change of second dimension
    size[1] = 3;

    // extend dataset
    dataset->extend(size);

    // create dataspace pointer
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    // select a hyperslab in extended portion of the dataset.
    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    // write data to data array
    long scanindexdata[3] = {dataindex, nopx, stopx};

    // Write data to the extended portion of the dataset.
    dataset->write(scanindexdata, PredType::STD_I64LE, *memspacenew, *filespacenew);

    // close and delete objects
    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;

    /* END WRITING LINE BREAK LOG DATA */

    /* BEGIN WRITING ROI IMAGE DATA */

    // if nopx != 0 and stopx == 0, stopx has to be scanY
    if ((nopx != 0) && (stopx == 0)) {
        stopx = scanY;
    }

    // if nopx and stopx are not zero
    if ((nopx != 0) && (stopx != 0)) {

        // get keys of ROI map
        auto const ROIkeys = ROImap.keys();

        // iterate through all ROIs
        for (std::string e : ROIkeys) {

            // give out some debug info
            std::cout<<"writing ROI data for "<<e<<" to file..."<<std::endl;

            // extend dimensions by (1 x scanX) => add one line to ROI image
            hsize_t dimsextroi[2] = {1, (hsize_t)scanX}; // extend dimensions

            // open data set
            DataSet *datasetroi = new DataSet(file->openDataSet("/measurement/instruments/sdd/roi/"+e));

            // declare size array
            hsize_t sizeroi[2];

            // declare offset array
            hsize_t offsetroi[2];

            // the offset of the first dimension has to be stopx-1
            offsetroi[0] = stopx-1;

            // the offset of the second dimension has to be 0
            offsetroi[1] = 0;

            // the first dimension of the new size has to be stopx
            sizeroi[0] = stopx;

            // the second dimension of the new size has to be scanX
            sizeroi[1] = (hsize_t)scanX;

            // extend ROI dataset
            datasetroi->extend(sizeroi);

            // get pointer to dataspace of extended dataset
            DataSpace *filespacenewroi = new DataSpace(datasetroi->getSpace());

            // select a hyperslab in extended portion of the dataset
            filespacenewroi->selectHyperslab(H5S_SELECT_SET, dimsextroi, offsetroi);

            // define memory space
            DataSpace *memspacenewroi = new DataSpace(2, dimsextroi, NULL);

            // declare and define a QVector with the ROI data of the given element
            QVector<uint32_t> roidata = ROImap[e];

            // declare ata array
            uint32_t writedata[scanX];

            // give out some debug info
            std::cout<<"länge des datenarrays: "<<roidata.length()<<"stopx: "<<stopx<<std::endl;

            // declare and initialize a pixel counter variable
            int pxcounter = 0;

            // iterate through the complete roi data
            for (int i=scanX*(stopx-1); i<roidata.length(); i++) {

                // copy ROI image pixel data from roidata vector to data array
                writedata[pxcounter] = roidata.at(i);

                // increment pixel counter
                pxcounter++;
            }

            // Write data to the extended portion of the dataset.
            datasetroi->write(writedata, PredType::STD_I32LE, *memspacenewroi, *filespacenewroi);

            // close and delete objects
            datasetroi->close();
            delete datasetroi;
            filespacenewroi->close();
            delete filespacenewroi;
            memspacenewroi->close();
            delete memspacenewroi;

        }
    }

    /* END WRITING ROI IMAGE DATA */

}

// wrapper function that creates the HDF5/NeXus data file
void hdf5nexus::createDataFile(QString filename, settingsdata settings) {

    // give out some debug info
    std::cout<<"creating HDF5/NeXus file with filename \""<<filename.toStdString()<<"\"..."<<std::endl;

    // copy ROI definitions to local variable
    std::string ROIdefinitions = settings.roidefinitions;

    // copy ccd width and height to local variables
    int ccdX = settings.ccd_width;
    int ccdY = settings.ccd_height;

    // copy scan width and height to local variables
    int scanX = settings.scanWidth;
    int scanY = settings.scanHeight;

    // create new HDF5 file
    file = new H5File(filename.toLocal8Bit(), H5F_ACC_TRUNC);

    // declare variables for the HDF5 version numbers
    unsigned majver;
    unsigned minnum;
    unsigned relnum;

    // get HDF5 library version
    H5::H5Library::getLibVersion(majver, minnum, relnum);

    // declare char array for version
    char versionstring[10];

    // create a formatted version string and write if to the version char array
    std::sprintf(versionstring, "%d.%d.%d", majver, minnum, relnum);

    // get current date and time
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    // build a string from date and time
    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    // create file attributes
    newNeXusAttribute("HDF5_Version", versionstring);
    newNeXusAttribute("default", "measurement");
    newNeXusAttribute("detectorRank", "2");
    newNeXusAttribute("file_name", filename.toStdString());
    newNeXusAttribute("file_time", datetime.toStdString());
    newNeXusAttribute("nE", "1");
    newNeXusAttribute("nP", QString::number(settings.scanHeight*settings.scanWidth).toStdString());
    newNeXusAttribute("nX", QString::number(settings.scanWidth).toStdString());
    newNeXusAttribute("nY", QString::number(settings.scanHeight).toStdString());

    // create "/measurement" group
    Group* measurement_group = newNeXusGroup("/measurement", "NX_class", "NXentry", false);

    // add "default" attribute to created group (NeXus simple plotting)
    newNeXusAttribute(measurement_group, "default", "transmission");

    // close and delete group objects
    measurement_group->close();
    delete(measurement_group);

    // create datasets in "/measurement"
    newNeXusScalarDataSet("/measurement/start_time", "NX_DATE_TIME", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/definition", "NX_CHAR", "NXstxm", true);
    newNeXusScalarDataSet("/measurement/scan_type", "NX_CHAR", settings.scantype, true);
    newNeXusScalarDataSet("/measurement/scan_width", "NX_CHAR", settings.scanWidth, true);
    newNeXusScalarDataSet("/measurement/scan_height", "NX_CHAR", settings.scanHeight, true);
    newNeXusScalarDataSet("/measurement/title", "NX_CHAR", settings.scantitle, true);
    newNeXusScalarDataSet("/measurement/x_step_size", "NX_FLOAT", settings.x_step_size, true);
    newNeXusScalarDataSet("/measurement/y_step_size", "NX_FLOAT", settings.y_step_size, true);

    // create "/measurement/transmission" group
    Group* transmission_group = newNeXusGroup("/measurement/transmission", "NX_class", "NXdata", false);

    // add signal and axes attributes to transmission group (NeXus simple plotting)
    newNeXusAttribute(transmission_group, "signal", "data");
    newNeXusAttribute(transmission_group, "axes", "[\"sample_x\",\"sample_y\"]");

    // close and delete group objects
    transmission_group->close();
    delete(transmission_group);

    // create "/measurement/transmission/sample_x" dataset
    DataSet* transmission_sample_x_ds = newNeXusChunked1DDataSet("/measurement/transmission/sample_x", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);

    // add "axis" attribute to "/measurement/transmission/sample_x" dataset (NeXus simple plotting)
    newNeXusAttribute(transmission_sample_x_ds, "axis", "0");

    // close and delete dataset objects
    transmission_sample_x_ds->close();
    delete(transmission_sample_x_ds);

    // create "/measurement/transmission/sample_y" dataset
    DataSet* transmission_sample_y_ds = newNeXusChunked1DDataSet("/measurement/transmission/sample_y", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);

    // add "axis" attribute to "/measurement/transmission/sample_y" dataset (NeXus simple plotting)
    newNeXusAttribute(transmission_sample_y_ds, "axis", "1");

    // close and delete dataset object
    transmission_sample_y_ds->close();
    delete(transmission_sample_y_ds);

    // create "/measurement/transmission/stxm_scan_type" dataset with content "sample image"
    newNeXusScalarDataSet("/measurement/transmission/stxm_scan_type", "NX_CHAR", "sample image", true);

    // create "/measurement/fluorescence" group
    Group* fluorescence_group = newNeXusGroup("/measurement/fluorescence", "NX_class", "NXdata", false);

    // add "signal" and "data" attributes (NeXus simple plotting)
    newNeXusAttribute(fluorescence_group, "signal", "data");
    newNeXusAttribute(fluorescence_group, "axes", "[\".\",\".\"]");

    // close and delete group object
    fluorescence_group->close();
    delete(fluorescence_group);

    // create "/measurement/fluorescence/sample_x" dataset
    DataSet* fluorescence_sample_x_ds = newNeXusChunked1DDataSet("/measurement/fluorescence/sample_x", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);

    // add "axis" attribute to "/measurement/fluorescence/sample_x" dataset (NeXus simple plotting)
    newNeXusAttribute(fluorescence_sample_x_ds, "axis", "0");

    // close and delete dataset object
    fluorescence_sample_x_ds->close();
    delete(fluorescence_sample_x_ds);

    // create "/measurement/fluorescence/sample_y" dataset
    DataSet* fluorescence_sample_y_ds = newNeXusChunked1DDataSet("/measurement/fluorescence/sample_y", PredType::NATIVE_FLOAT, "NX_FLOAT", settings.file_compression, settings.file_compression_level, false);

    // add "axis" attribute to "/measurement/fluorescence/sample_y" dataset (NeXus simple plotting)
    newNeXusAttribute(fluorescence_sample_y_ds, "axis", "1");

    // close and delete dataset object
    fluorescence_sample_y_ds->close();
    delete(fluorescence_sample_y_ds);

    // create "/measurement/fluorescence/stxm_scan_type" dataset with content "generic scan"
    newNeXusScalarDataSet("/measurement/fluorescence/stxm_scan_type", "NX_CHAR", "generic scan", true);

    // create groups in "/measurement" group
    newNeXusGroup("/measurement/instruments", "NX_class", "NXinstrument", true);
    newNeXusGroup("/measurement/instruments/ccd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/sdd", "NX_class", "NXdetector", true);
    newNeXusGroup("/measurement/instruments/monochromator", "NX_class", "NXmonochromator", true);
    newNeXusGroup("/measurement/instruments/beamline", "NX_class", "NXcollection", true);

    // create "/measurement/instruments/sample_x" and "/measurement/instruments/sample_y" datasets
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

    // create groups for ccd settings
    newNeXusGroup("/measurement/instruments/ccd/settings", "NX_class", "NXcollection", true);
    newNeXusGroup("/measurement/instruments/ccd/settings/set", "NX_class", "NXcollection", true);
    newNeXusGroup("/measurement/instruments/ccd/settings/calculated", "NX_class", "NXcollection", true);

    // create ccd settings datasets and write ccd settings to data file
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

    // create group for sdd settings
    newNeXusGroup("/measurement/instruments/sdd/settings", "NX_class", "NXcollection", true);

    // create sdd settings datasets and write sdd settings to data file
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

    // if userdata is provided in the settings.userdata field
    if (settings.userdata != "") {

        // try to deserialize the json data
        try {

            // declare QJsonParseError
            QJsonParseError user_jsonError;

            // parse QJsonDocument from settings.userdata string
            QJsonDocument user_document = QJsonDocument::fromJson(settings.userdata.c_str(), &user_jsonError);

            // declare and set QJsonArray
            QJsonArray user_jsonArray = user_document.array();

            // iterate through the whole JSON array
            for (int i_c=0; i_c < user_jsonArray.count(); i_c++) {

                // write data at current JSON array position to QJsonObject
                QJsonObject userobj = user_jsonArray.at(i_c).toObject();

                // create new group for user
                newNeXusGroup("/measurement/user_"+QString::number(i_c).toStdString(), "NX_class", "NXuser", true);

                // if field "name" exists
                if (userobj.contains("name")) {

                    // get name string from name field of JSON Object
                    QString name = userobj.take("name").toString();

                    // if name is not empty
                    if (name != "") {
                        // create dataset and write name into dataset
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/name", "NX_CHAR", name.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("role")) {
                    QString role = userobj.take("role").toString();
                    if (role != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/role", "NX_CHAR", role.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("affiliation")) {
                    QString affiliation = userobj.take("affiliation").toString();
                    if (affiliation != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/affiliation", "NX_CHAR", affiliation.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("address")) {
                    QString address = userobj.take("address").toString();
                    if (address != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/address", "NX_CHAR", address.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("telephone_number")) {
                    QString telephone_number = userobj.take("telephone_number").toString();
                    if (telephone_number != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/telephone_number", "NX_CHAR", telephone_number.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("fax_number")) {
                    QString fax_number = userobj.take("fax_number").toString();
                    if (fax_number != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/fax_number", "NX_CHAR", fax_number.toStdString(), true);
                    }
                }

                //logic analogous to name
                if (userobj.contains("email")) {
                    QString email = userobj.take("email").toString();
                    if (email != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/email", "NX_CHAR", email.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("facility_user_id")) {
                    QString facility_user_id = userobj.take("facility_user_id").toString();
                    if (facility_user_id != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/facility_user_id", "NX_CHAR", facility_user_id.toStdString(), true);
                    }
                }

                // logic analogous to name
                if (userobj.contains("ORCID")) {
                    QString ORCID = userobj.take("ORCID").toString();
                    if (ORCID != "") {
                        newNeXusScalarDataSet("/measurement/user_"+QString::number(i_c).toStdString()+"/ORCID", "NX_CHAR", ORCID.toStdString(), true);
                    }
                }
            }
        // catch errors
        }  catch (const std::exception& ex) {
            // give out error text
            std::cout<<"error parsing user data json, ignoring user data.."<<std::endl;
        }
    }

    // create new group for pre scan note
    newNeXusGroup("/measurement/pre_scan_note", "NX_class", "NXnote", true);

    // get current date and time
    cd = QDate::currentDate();
    ct = QTime::currentTime();

    // build a string from date and time
    datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    // create datasets for date and description in pre scan note group
    newNeXusScalarDataSet("/measurement/pre_scan_note/date", "NX_CHAR", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/pre_scan_note/description", "NX_CHAR", settings.notes, true);

    // add ccd data dataset
    newNeXusChunkedCCDDataSet("/measurement/instruments/ccd/data", ccdX, ccdY, PredType::STD_U16LE, "NX_INT", settings.file_compression, settings.file_compression_level, true);

    // create dataset for transmission preview image
    DataSet* transmissionpreviewdataset = newNeXusChunkedTransmissionPreviewDataSet("/measurement/transmission/data", PredType::STD_I32LE, "NX_INT", settings.file_compression, settings.file_compression_level, false);

    // add signal and axes attributes to transmission preview image dataset (NeXus simple plotting)
    newNeXusAttribute(transmissionpreviewdataset, "signal", "1");
    newNeXusAttribute(transmissionpreviewdataset, "axes", "[\"sample_x\",\"sample_y\"]");

    // close and delete transmission preview image dataset object
    transmissionpreviewdataset->close();
    delete transmissionpreviewdataset;

    // create dataset for fluorescence data
    DataSet* fluorescencedataset = newNeXusChunkedSpectraDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, "NX_INT", settings.file_compression, settings.file_compression_level, false);

    // add signal attribute to fluorescence dataset (NeXus simple plotting)
    newNeXusAttribute(fluorescencedataset, "signal", "1");

    // close and delete fluorescence dataset object
    fluorescencedataset->close();
    delete fluorescencedataset;

    // link "/measurement/fluorescence/data" to "/measurement/instruments/sdd/data"
    file->link(H5L_TYPE_HARD, "/measurement/instruments/sdd/data", "/measurement/fluorescence/data");

    // create group for sdd linebreak and scan index logs
    newNeXusGroup("/measurement/instruments/sdd/log", "NX_class", "NXcollection", true);

    // create datasets for linebreak and scan index logs
    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/scanindex", PredType::STD_I64LE, "scan index log", settings.file_compression, settings.file_compression_level, true);
    newNeXusChunkedSDDLogDataSet("/measurement/instruments/sdd/log/linebreaks", PredType::STD_I64LE, "line break log", settings.file_compression, settings.file_compression_level, true);

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

// wrapper function that creates opens a HDF5/NeXus data file
void hdf5nexus::openDataFile(QString fname) {
    // open data file
    file = new H5File(fname.toLocal8Bit(), H5F_ACC_RDWR);
}

// wrapper function that writes metadata to the the HDF5/NeXus data file
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

    // give out some debug info
    std::cout<<metadata.acquisition_number<<std::endl;
    std::cout<<metadata.acquisition_time<<std::endl;
    std::cout<<metadata.beamline_energy<<std::endl;
    std::cout<<metadata.ringcurrent<<std::endl;
    std::cout<<metadata.horizontal_shutter<<std::endl;
    std::cout<<metadata.vertical_shutter<<std::endl;
    std::cout<<metadata.set_energy<<std::endl;
}

// wrapper function that writes sdd data to the HDF5/NeXus data file
void hdf5nexus::writeSDDData(int32_t pxnum, spectrumdata specdata) {

    // spectrum has 4096 pixel
    hsize_t size[2];
    size[1] = 4096;

    // offset[0] is the number of current scan pixel => data will be appended to existing data in dataset
    hsize_t offset[2];
    offset[0] = pxnum;
    offset[1] = 0;

    // declare data array
    int32_t data[4096];

    // iterate through all pixels of the spectrum
    for (int i=0;i<4096;i++) {
        // copy pixel counts to data array
        data[i] = specdata.at(i);
    }

    // size is current pixel number + 1
    size[0] = pxnum+1;

    // offset is current pixel number
    offset[0] = pxnum;

    // extend dimensions by (1 x 4096)
    hsize_t dimsext[2] = {1, 4096}; // extend dimensions

    // create pointer to opened datasez
    DataSet *dataset = new DataSet(file->openDataSet("/measurement/instruments/sdd/data"));

    // extend dataset
    dataset->extend(size);

    // select a hyperslab in extended portion of the dataset
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // define memory space
    DataSpace *memspace = new DataSpace(2, dimsext, NULL);

    // write data to the extended portion of the dataset
    dataset->write(data, PredType::STD_I32LE, *memspace, *filespace);

    // close and delete objects
    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    memspace->close();
    delete memspace;
}

// wrapper function that writes calculated ccd settings the HDF5/NeXus data file
void hdf5nexus::writeCCDSettings(ccdsettings ccdsettingsdata) {
    // write calculated ccd settings to data file
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/set_kinetic_cycle_time", "NX_FLOAT", ccdsettingsdata.set_kinetic_cycle_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/exposure_time", "NX_FLOAT", ccdsettingsdata.exposure_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/accumulation_time", "NX_FLOAT", ccdsettingsdata.accumulation_time, true);
    newNeXusScalarDataSet("/measurement/instruments/ccd/settings/calculated/kinetic_time", "NX_FLOAT", ccdsettingsdata.kinetic_time, true);
}

// wrapper function that writes a scan note to the HDF5/NeXus data file
void hdf5nexus::writeScanNote(std::string scannote, int notecounter) {
    // get current date and time
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    // create a string from date and time
    QString datetime = cd.toString(Qt::ISODate)+" "+ct.toString(Qt::ISODate);

    // create note group and write note date and content to datasets in this group
    newNeXusGroup("/measurement/post_scan_note_"+QString::number(notecounter).toStdString(), "NX_class", "NXnote", true);
    newNeXusScalarDataSet("/measurement/post_scan_note_"+QString::number(notecounter).toStdString()+"/date", "NX_CHAR", datetime.toStdString(), true);
    newNeXusScalarDataSet("/measurement/post_scan_note_"+QString::number(notecounter).toStdString()+"/description", "NX_CHAR", scannote, true);
}

// wrapper function that appends a value to a existing one dimensional data set in the HDF5/NeXus data file
void hdf5nexus::appendValueTo1DDataSet(std::string location, int position, float value) {

    // declare arrays for size and offset
    hsize_t size[1];
    hsize_t offset[1];

    // declare data array
    float dataarr[1];

    // write data value to data array
    dataarr[0] = value;

    // set the value of size[0] to the given position+1
    size[0] = position+1;

    // set offset[0] to the given position
    offset[0] = position;

    // dataset will be extended by 1
    hsize_t dimsext[1] = {1};

    // get pointer to opened dataset
    DataSet *dataset = new DataSet(this->file->openDataSet(location));

    // extend dataset
    dataset->extend(size);

    // get pointer to the dataspace from extended dataset
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    // select a hyperslab in extended portion of the dataset.
    filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // define memory space
    DataSpace *memspace = new DataSpace(1, dimsext, NULL);

    // write data to the extended portion of the dataset.
    dataset->write(dataarr, PredType::NATIVE_FLOAT, *memspace, *filespace);

    // close and delete objects
    filespace->close();
    delete(filespace);
    memspace->close();
    delete(memspace);
    dataset->close();
    delete(dataset);
}
