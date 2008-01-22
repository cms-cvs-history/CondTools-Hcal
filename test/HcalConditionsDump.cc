
/*----------------------------------------------------------------------

R.Ofierzynski - 2.Oct. 2007
   modified to dump all pedestals on screen, see 
   testHcalDBFake.cfg
   testHcalDBFrontier.cfg

----------------------------------------------------------------------*/

#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CondFormats/DataRecord/interface/HcalPedestalsRcd.h"
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbASCIIIO.h"

using namespace std;

namespace edmtest
{
  class HcalConditionsDump : public edm::EDAnalyzer
  {
  public:
    explicit  HcalConditionsDump(edm::ParameterSet const& p) 
    {
      front = p.getUntrackedParameter<string>("outFilePrefix","Dump");
    }

    explicit  HcalConditionsDump(int i) 
    { }
    virtual ~ HcalConditionsDump() { }
    virtual void analyze(const edm::Event& e, const edm::EventSetup& c);
  private:
    string front;
  };
  
  void
   HcalConditionsDump::analyze(const edm::Event& e, const edm::EventSetup& context)
  {
    using namespace edm::eventsetup;
    // Context is not used.
    std::cout <<"HcalConditionsDump::analyze-> I AM IN RUN NUMBER "<<e.id().run() <<std::endl;
    //    std::cout <<"HcalConditionsDump::analyze->  ---EVENT NUMBER "<<e.id().run() <<std::endl;
    //front = "Dump";

    int iov = 0;

    // pedestals
    edm::ESHandle<HcalPedestals> pPeds;
    context.get<HcalPedestalsRcd>().get(pPeds);
    const HcalPedestals* myped = pPeds.product();

    // dump pedestals:
    std::ostringstream filename;
    filename << front << "HcalPedestals" << "_" << iov << ".txt";
    std::ofstream outStream(filename.str().c_str());
    cout << "--- Dumping Pedestals ---" << endl;
    HcalDbASCIIIO::dumpObject (outStream, (*myped) );


//    // pedestal widths
//    edm::ESHandle<HcalPedestalWidths> pPedWs;
//    context.get<HcalPedestalWidthsRcd>().get(pPedWs);
//    const HcalPedestalWidths* mypedwid = pPedWs.product();
//
//    // dump pedestal widths:
//    std::ostringstream filenamePW;
//    filenamePW << front << "HcalPedestalWidths" << "_" << iov << ".txt";
//    std::ofstream outStreamPW(filenamePW.str().c_str());
//    cout << "--- Dumping Pedestal Widths ---" << endl;
//    HcalDbASCIIIO::dumpObject (outStreamPW, (*mypedwid) );
//
//    // gains
//    edm::ESHandle<HcalGains> pGains;
//    context.get<HcalGainsRcd>().get(pGains);
//    const HcalGains* mygains = pGains.product();
//
//    // dump gains:
//    std::ostringstream filenameG;
//    filenameG << front << "HcalGains" << "_" << iov << ".txt";
//    std::ofstream outStreamG(filenameG.str().c_str());
//    cout << "--- Dumping Gains ---" << endl;
//    HcalDbASCIIIO::dumpObject (outStreamG, (*mygains) );
//
//    // gainwidths
//    edm::ESHandle<HcalGainWidths> pGainWs;
//    context.get<HcalGainWidthsRcd>().get(pGainWs);
//    const HcalGainWidths* mygwid = pGainWs.product();
//
//    // dump gain widths:
//    std::ostringstream filenameGW;
//    filenameGW << front << "HcalGainWidths" << "_" << iov << ".txt";
//    std::ofstream outStreamGW(filenameGW.str().c_str());
//    cout << "--- Dumping Gain Widths ---" << endl;
//    HcalDbASCIIIO::dumpObject (outStreamGW, (*mygwid) );


//    std::cout <<" Hcal peds for channel HB eta=15, phi=5, depth=2 "<<std::endl;
//    int channelID = HcalDetId (HcalBarrel, 15, 5, 2).rawId();
//    const HcalPedestals* myped=pPeds.product();
//    const HcalPedestalWidths* mypedW=pPedWs.product();
//    const HcalGains* mygain=pGains.product();
//    const HcalGainWidths* mygainW=pGainWs.product();
//
//    const float* values = myped->getValues (channelID);
//    if (values) std::cout << "pedestals for channel " << channelID << ": "
//			  << values [0] << '/' << values [1] << '/' << values [2] << '/' << values [3] << std::endl; 
//    values = mypedW->getValues (channelID);
//    if (values) std::cout << "pedestal widths for channel " << channelID << ": "
//			  << values [0] << '/' << values [1] << '/' << values [2] << '/' << values [3] << std::endl; 
//    values = mygain->getValues (channelID);
//    if (values) std::cout << "gains for channel " << channelID << ": "
//			  << values [0] << '/' << values [1] << '/' << values [2] << '/' << values [3] << std::endl; 
//    values = mygainW->getValues (channelID);
//    if (values) std::cout << "gain widts for channel " << channelID << ": "
//			  << values [0] << '/' << values [1] << '/' << values [2] << '/' << values [3] << std::endl; 
  }
  DEFINE_FWK_MODULE(HcalConditionsDump);
}
