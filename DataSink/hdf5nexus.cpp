#include "hdf5nexus.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

hdf5nexus::hdf5nexus()
{

}

void hdf5nexus::createDataFile(QString filename, settingsdata settings) {
    std::cout<<"creating HDF5/NeXus file with filename \""<<filename.toStdString()<<"\"..."<<std::endl;

    std::string ROIdefinitions = settings.ROIdefinitions;

    int ccdX = settings.ccdWidth;
    int ccdY = settings.ccdHeight;

    int scanX = settings.scanWidth;
    int scanY = settings.scanHeight;

    file = new H5File(filename.toLocal8Bit(), H5F_ACC_TRUNC);

    hsize_t maxdims[3]    = {H5S_UNLIMITED, (hsize_t)ccdX, (hsize_t)ccdY, };
    hsize_t chunk_dims[3] = {1, (hsize_t)ccdX, (hsize_t)ccdY};

    hsize_t chunk_dimsspec[2] = {1, 4096};

    unsigned majver;
    unsigned minnum;
    unsigned relnum;

    H5::H5Library::getLibVersion(majver, minnum, relnum);

    char versionstring[10];
    std::sprintf(versionstring, "%d.%d.%d", majver, minnum, relnum);

    //std::cout<<"hdf5 version: "<<versionstring;

    hsize_t size[3];
    size[1] = (hsize_t)ccdX;
    size[2] = (hsize_t)ccdY;

    hsize_t offset[3];
    offset[1] = 0;
    offset[2] = 0;

    hsize_t offsetspec[2];
    offsetspec[0] = 0;
    offsetspec[1] = 0;

    hsize_t dims[3];
    dims[0] = 0;
    dims[1] = (hsize_t)ccdX;
    dims[2] = (hsize_t)ccdY;

    hsize_t dimsspec[2];
    dimsspec[0] = 0;
    dimsspec[1] = 4096;

    hsize_t sizespec[2];
    sizespec[0] = 1;
    sizespec[1] = 4096;

    hsize_t maxdimsspec[2] = {H5S_UNLIMITED, 4096};

    StrType str_type(PredType::C_S1, H5T_VARIABLE);
    str_type.setCset(H5T_CSET_UTF8);
    DataSpace att_space(H5S_SCALAR);

    Attribute att = file->createAttribute( "HDF5_Version", str_type, att_space);
    att.write(str_type, std::string(versionstring));

    att = file->createAttribute( "default", str_type, att_space);
    att.write(str_type, std::string("entry"));

    att = file->createAttribute( "detectorRank", str_type, att_space);
    att.write(str_type, std::string("detectorRank"));

    att = file->createAttribute( "file_name", str_type, att_space);
    att.write(str_type, std::string("scantest.h5"));

    att = file->createAttribute( "file_time", str_type, att_space);
    att.write(str_type, std::string("2021-03-29T15:51:42.829454"));

    att = file->createAttribute( "nE", str_type, att_space);
    att.write(str_type, std::string("number of energies scanned"));

    att = file->createAttribute( "nP", str_type, att_space);
    att.write(str_type, std::string("total number of scan points"));

    att = file->createAttribute( "nX", str_type, att_space);
    att.write(str_type, std::string("number of pixels in X direction"));

    att = file->createAttribute( "nY", str_type, att_space);
    att.write(str_type, std::string("number of pixels in Y direction"));

    // create file structure
    Group* entrygroup = new Group(file->createGroup( "/measurement" ));

    att = entrygroup->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXentry"));

    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);
    float testdata[] = {1.5};

    DataSet* entrystarttime = new DataSet(file->createDataSet("/measurement/start_time", str_type, fspace));
    entrystarttime->write(std::string("2021-03-29T15:51:42.829454"), str_type, att_space);
    att = entrystarttime->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_DATE_TIME"));

    DataSet* entryendtime = new DataSet(file->createDataSet("/measurement/end_time", str_type, fspace));
    entryendtime->write(std::string("2021-03-29T15:51:42.829454"), str_type, att_space);
    att = entryendtime->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_DATE_TIME"));

    DataSet* entrydefinition = new DataSet(file->createDataSet("/measurement/definition", str_type, fspace));
    entrydefinition->write(std::string("NXstxm"), str_type, att_space);
    att = entrydefinition->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));

    DataSet* entrytitle = new DataSet(file->createDataSet("/measurement/title", str_type, fspace));
    entrytitle->write(std::string("TITLE"), str_type, att_space);
    att = entrytitle->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));

    Group* entrydatamonitor = new Group(file->createGroup( "/measurement/monitor" ));
    att = entrydatamonitor->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXmonitor"));

    DataSet* entrymonitordata = new DataSet(file->createDataSet("/measurement/monitor/data", PredType::NATIVE_FLOAT, fspace));
    entrymonitordata->write(testdata, PredType::NATIVE_FLOAT, fspace);
    att = entrymonitordata->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));

    Group* entrydata = new Group(file->createGroup( "/measurement/data" ));
    att = entrydata->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXdata"));
    DataSet* entrydataenergy = new DataSet(file->createDataSet("/measurement/data/energy", PredType::NATIVE_FLOAT, fspace));
    att = entrydataenergy->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));
    entrydataenergy->write(testdata, PredType::NATIVE_FLOAT, fspace);
    DataSet* entrydatasamplex = new DataSet(file->createDataSet("/measurement/data/sample_x", PredType::NATIVE_FLOAT, fspace));
    att = entrydatasamplex->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));
    entrydatasamplex->write(testdata, PredType::NATIVE_FLOAT, fspace);
    DataSet* entrydatasampley = new DataSet(file->createDataSet("/measurement/data/sample_y", PredType::NATIVE_FLOAT, fspace));
    att = entrydatasampley->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));
    entrydatasampley->write(testdata, PredType::NATIVE_FLOAT, fspace);
    DataSet* entrydatastxmscantype = new DataSet(file->createDataSet("/measurement/data/stxm_scan_type", str_type, fspace));
    att = entrydatastxmscantype->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));
    entrydatastxmscantype->write(std::string("sample image"), str_type, fspace);
    Group* entryinstrument = new Group(file->createGroup( "/measurement/instruments" ));
    att = entryinstrument->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXinstrument"));

    Group* entryinstrumentCCD = new Group(file->createGroup( "/measurement/instruments/ccd" ));
    att = entryinstrumentCCD->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXdetector"));

    Group* entryinstrumentSDD = new Group(file->createGroup( "/measurement/instruments/sdd" ));
    att = entryinstrumentSDD->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXdetector"));

    Group* entryinstrumentmonochromator = new Group(file->createGroup( "/measurement/instruments/monochromator" ));
    att = entryinstrumentmonochromator->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXmonochromator"));

    DataSet* entryinstrumentmonochromatorenergy = new DataSet(file->createDataSet("/measurement/instruments/monochromator/energy", PredType::NATIVE_FLOAT, fspace));
    att = entryinstrumentmonochromatorenergy->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));
    entryinstrumentmonochromatorenergy->write(testdata, PredType::NATIVE_FLOAT, fspace);

    Group* entryinstrumentsource = new Group(file->createGroup( "/measurement/instruments/source" ));
    att = entryinstrumentsource->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXsource"));
    DataSet* entryinstrumentsourcename = new DataSet(file->createDataSet("/measurement/instruments/source/name", str_type, fspace));
    entryinstrumentsourcename->write(std::string("BEISPIELNAME"), str_type, att_space);
    att = entryinstrumentsourcename->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));

    DataSet* entryinstrumentsourceprobe = new DataSet(file->createDataSet("/measurement/instruments/source/probe", str_type, fspace));
    entryinstrumentsourceprobe->write(std::string("BEISPIELNAME"), str_type, att_space);
    att = entryinstrumentsourceprobe->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));

    DataSet* entryinstrumentsourcetype = new DataSet(file->createDataSet("/measurement/instruments/source/type", str_type, fspace));
    entryinstrumentsourcetype->write(std::string("BEISPIELNAME"), str_type, att_space);
    att = entryinstrumentsourcetype->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_CHAR"));

    Group* entrysample = new Group(file->createGroup( "/measurement/sample" ));
    att = entrysample->createAttribute( "NX_class", str_type, att_space);
    att.write(str_type, std::string("NXsample"));
    DataSet* entrysamplerotationangle = new DataSet(file->createDataSet("/measurement/sample/rotation_angle", PredType::NATIVE_FLOAT, fspace));
    entrysamplerotationangle->write(testdata, PredType::NATIVE_FLOAT, fspace);
    att = entrysamplerotationangle->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));


    // Turn off the auto-printing when failure occurs so that we can
    // handle the errors appropriately
    Exception::printErrorStack();

    // Create the data space for the dataset.  Note the use of pointer
    // for the instance 'dataspace'.  It can be deleted and used again
    // later for another dataspace.  An HDF5 identifier can be closed
    // by the destructor or the method 'close()'.
    DataSpace *dataspace = new DataSpace(3, dims, maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);

    // Create the chunked dataset.  Note the use of pointer.
    DataSet *dataset = new DataSet(file->createDataSet("/measurement/instruments/ccd/data", PredType::STD_U16LE, *dataspace, prop));
    att = dataset->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));

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

    DataSet *datasetsumimage = new DataSet(file->createDataSet("/measurement/data/data", PredType::STD_I64LE, *dataspacesumimage, propsumimage));
    att = datasetsumimage->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));
    att = datasetsumimage->createAttribute( "signal", str_type, att_space);
    att.write(str_type, std::string("1"));

    // write fluorescence data
    DataSpace *dataspacespectrum = new DataSpace(2, dimsspec, maxdimsspec);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList propspectrum;
    propspectrum.setChunk(2, chunk_dimsspec);

    // Create the chunked dataset.  Note the use of pointer.
    DataSet *entryfluoinstrumentfluorescencedata = new DataSet(file->createDataSet("/measurement/instruments/sdd/data", PredType::STD_I16LE, *dataspacespectrum, propspectrum));
    att = entryfluoinstrumentfluorescencedata->createAttribute( "type", str_type, att_space);
    att.write(str_type, std::string("NX_FLOAT"));

    file->link( H5L_TYPE_HARD, "/measurement/instruments/sdd/data", "/measurement/data/sdd" );

    // create log group
    Group* sddloggroup = new Group(file->createGroup( "/measurement/instruments/sdd/log" ));

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
    DataSet *entryscanindex = new DataSet(file->createDataSet("/measurement/instruments/sdd/log/scanindex", PredType::STD_I32LE, *dataspacelinebreak, proplinebreak));
    DataSet *entrylinebreaks = new DataSet(file->createDataSet("/measurement/instruments/sdd/log/linebreaks", PredType::STD_I32LE, *dataspacelinebreak, proplinebreak));

    // if ROIs are enabled (ROIdefinitions != ""), create datasets for ROIs
    if (ROIdefinitions != "") {
        // create log group
        Group* sddroigroup = new Group(file->createGroup( "/measurement/instruments/sdd/roi" ));

        // set hdf settings for ROIs
        hsize_t roimaxdims[2]    = {(hsize_t)scanY, (hsize_t)scanX};
        hsize_t roichunk[2] = {1, (hsize_t)scanX};

        hsize_t dimsroi[2] = {0, (hsize_t)scanX};

        DataSpace *dataspaceroi = new DataSpace(2, dimsroi, roimaxdims);
        // Modify dataset creation property to enable chunking
        DSetCreatPropList proproi;
        proproi.setChunk(2, roichunk);

        // create ROI datasets
        QJsonParseError jsonError;
        QJsonDocument document = QJsonDocument::fromJson(ROIdefinitions.c_str(), &jsonError);
        QJsonObject jsonObj = document.object();
        foreach(QString key, jsonObj.keys()) {
            std::string k(key.toLocal8Bit());
            DataSet *roidataset = new DataSet(file->createDataSet("/measurement/instruments/sdd/roi/"+k, PredType::STD_I32LE, *dataspaceroi, proproi));
        }
    }

    // create settings group
    Group* settingsgroup = new Group(file->createGroup( "/measurement/settings" ));

    DataSet* settingsgroupscanWidth = new DataSet(file->createDataSet("/measurement/settings/scanWidth", PredType::STD_I32LE, fspace));
    int scanWidth = settings.scanWidth;
    settingsgroupscanWidth->write(&scanWidth, PredType::STD_I32LE, att_space);

    DataSet* settingsgroupscanHeight = new DataSet(file->createDataSet("/measurement/settings/scanHeight", PredType::STD_I32LE, fspace));
    int scanHeight = settings.scanHeight;
    settingsgroupscanHeight->write(&scanHeight, PredType::STD_I32LE, att_space);

    DataSet* settingsgroupccdHeight = new DataSet(file->createDataSet("/measurement/settings/ccdHeight", PredType::STD_I32LE, fspace));
    /*int ccdHeight = settings.ccdHeight;
    settingsgroupccdHeight->write(&ccdHeight, PredType::STD_I32LE, att_space);*/

    DataSet* settingsgroupccdWidth = new DataSet(file->createDataSet("/measurement/settings/ccdWidth", PredType::STD_I32LE, fspace));
    /*int ccdWidth = settings.ccdWidth;
    settingsgroupccdWidth->write(&ccdWidth, PredType::STD_I32LE, att_space);*/

    // create metadata group
    Group* metadatagroup = new Group(file->createGroup( "/measurement/metadata" ));
    // create linebreak log dataset
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
    DataSet *entrymetadatabeamline_energy = new DataSet(file->createDataSet("/measurement/metadata/beamline_energy", PredType::STD_I32LE, *dataspacemetadata, propmetadata));
    DataSet *entrymetadataaquisition_number = new DataSet(file->createDataSet("/measurement/metadata/aquisition_number", PredType::STD_I32LE, *dataspacemetadata, propmetadata));
    prop.close();
    //file->close();

}
