#include "featureExtractor.h"
#include "utils.h"
namespace feature_extractor {
FeatureExtractor::FeatureExtractor () {
  InitializeSRS();
}

void FeatureExtractor::CloseDstDS() {
  GDALClose(this->dstDS);
}

void FeatureExtractor::CloseSrcDS() {
  GDALClose(this->srcDS);
}

void FeatureExtractor::InitializeSRS () {
  srcSRS.importFromEPSG(3067);
  dstSRS.importFromEPSG(4326);
  poCT = OGRCreateCoordinateTransformation(&this->srcSRS, &this->dstSRS);
}

GDALDriver* FeatureExtractor::GetDstDriver() {
  const char *pszDriverName = "GeoJSON";
  GDALDriver *dstDriver;
  dstDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
  if (dstDriver == NULL) {
    printf("error creating dstDriver");
  }
  return dstDriver;
}

void FeatureExtractor::CreateDstDS(std::string dstDSName) {
  GDALDriver *poDriver = GetDstDriver();
  dstDS = poDriver->Create(dstDSName.c_str(), 0, 0, 0, GDT_Unknown, NULL);
}

void FeatureExtractor::AcquireSrcDS(std::string srcDSName) {
  std::string path = "/vsizip/" + srcDSName;
  this->srcDS = (GDALDataset*) GDALOpenEx(path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
  if( this->srcDS == NULL )
  {
    printf("Opening %s failed.\n", path.c_str());
    exit(1);
  }
}

void FeatureExtractor::CreateDstLayer(std::string dstLayerName, std::string dstLayerType) {
  OGRwkbGeometryType geomType;
  if (dstLayerType == "polygon") {
    geomType = wkbPolygon;
  } else if (dstLayerType == "linestring") {
    geomType = wkbLineString;
  } else if (dstLayerType == "point") {
    geomType = wkbPoint;
  }
  this->dstLayer = dstDS->CreateLayer(dstLayerName.c_str(), &dstSRS, wkbPolygon, NULL );
}

void FeatureExtractor::LayerFromDst2Src(std::string srcLayerName) {
  if (!utils::checkLayerExists(srcDS, srcLayerName)) {
    return;
  }
  OGRLayer* srcLayer = srcDS->GetLayerByName(srcLayerName.c_str());
  OGRFeature *poFeature;
  srcLayer->ResetReading();
  while( (poFeature = srcLayer->GetNextFeature()) != NULL )
  {
    poFeature->GetGeometryRef()->transform(poCT);
    poFeature->GetGeometryRef()->flattenTo2D();
    poFeature->GetGeometryRef()->swapXY();
    OGRErr err = dstLayer->CreateFeature(poFeature);
    if (err != 0) {
      printf("error creating feature");
      exit(1);
    }
    OGRFeature::DestroyFeature(poFeature);
  }
}
}
