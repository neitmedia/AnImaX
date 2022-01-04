#ifndef HDF5NEXUS_H
#define HDF5NEXUS_H
#include <structs.h>
#include <QString>
#include "H5Cpp.h"
using namespace H5;

typedef QMap<std::string, QVector<uint32_t>> roidata;
typedef QVector<uint32_t> spectrumdata;

class hdf5nexus
{
public:
    hdf5nexus();

    void createDataFile(QString, settingsdata);
    void openDataFile(QString);
    void writeEndTimeStamp();
    void closeDataFile();
    void newNeXusFileStringAttribute(std::string, std::string);
    void newNeXusGroupStringAttribute(Group*, std::string, std::string);
    void newNeXusDatasetStringAttribute(DataSet*, std::string, std::string);

    void writeScanIndexData(int, int, int);
    void writeLineBreakDataAndROIs(roidata, int, int, int, int, int);

    void writeMetadata(metadata);
    void writeSDDData(int32_t, spectrumdata);
    void writeCCDSettings(int width, int height);

    void writeScanNote(std::string);

    //DataSet* newNeXusScalarDataSet(std::string, std::string, bool, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, std::string, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, float, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, int32_t, bool);

    Group* newNeXusGroup(std::string, std::string, std::string, bool);
    DataSet* newNeXusChunkedCCDDataSet(std::string, int, int, H5::PredType, std::string, bool);
    DataSet* newNeXusChunkedTransmissionPreviewDataSet(std::string, H5::PredType, std::string, bool);
    DataSet* newNeXusChunkedSpectraDataSet(std::string, H5::PredType, std::string, bool);
    DataSet* newNeXusChunkedSDDLogDataSet(std::string, H5::PredType, std::string, bool);
    DataSet* newNeXusROIDataSet(std::string, int, int, H5::PredType, std::string, bool);
    DataSet* newNeXusChunkedMetadataDataSet(std::string, H5::PredType, std::string, bool);

    H5File *file;

private:

};

#endif // HDF5NEXUS_H
