#ifndef HcalPedestalsHandler_h
#define HcalPedestalsHandler_h

// Radek Ofierzynski, 27.02.2008


#include <string>
#include <iostream>
#include <typeinfo>
#include <fstream>

#include "FWCore/Framework/interface/MakerMacros.h"
#include "CondCore/PopCon/interface/PopConSourceHandler.h"
#include "CondCore/PopCon/interface/LogReader.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/EventSetup.h"
// user include files
#include "CondFormats/HcalObjects/interface/HcalPedestals.h"
#include "CondFormats/DataRecord/interface/HcalPedestalsRcd.h"
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbASCIIIO.h"


class HcalPedestalsHandler : public popcon::PopConSourceHandler<HcalPedestals>
{
 public:
  void getNewObjects();
  std::string id() const { return m_name;}
  ~HcalPedestalsHandler();
  HcalPedestalsHandler(edm::ParameterSet const &);

 private:
  unsigned int sinceTime;
  std::string fFile;

  std::string m_name;

};
#endif