#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>

// other
#include "DataFormats/HcalDetId/interface/HcalDetId.h"
#include "DataFormats/HcalDetId/interface/HcalTrigTowerDetId.h"
#include "DataFormats/HcalDetId/interface/HcalElectronicsId.h"
#include "Geometry/CaloTopology/interface/HcalTopology.h"


// pool
#include "PluginManager/PluginManager.h"
#include "POOLCore/POOLContext.h"
#include "POOLCore/Token.h"
#include "FileCatalog/URIParser.h"
#include "FileCatalog/IFileCatalog.h"
#include "StorageSvc/DbType.h"
#include "PersistencySvc/DatabaseConnectionPolicy.h"
#include "PersistencySvc/ISession.h"
#include "PersistencySvc/ITransaction.h"
#include "PersistencySvc/IDatabase.h"
#include "PersistencySvc/Placement.h"
#include "DataSvc/DataSvcFactory.h"
#include "DataSvc/IDataSvc.h"
#include "DataSvc/ICacheSvc.h"
#include "DataSvc/Ref.h"
#include "RelationalAccess/SchemaException.h"
#include "Collection/Collection.h"
#include "CoralBase/AttributeList.h"
#include "FileCatalog/FCSystemTools.h"
#include "FileCatalog/FCException.h"
#include "FileCatalog/IFCAction.h"

// conditions
#include "CondCore/IOVService/interface/IOV.h"
#include "CondCore/MetaDataService/interface/MetaData.h"

// Hcal calibrations
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbHardcode.h"
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbASCIIIO.h"
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbXml.h"
#include "CondFormats/HcalObjects/interface/HcalPedestals.h"
#include "CondFormats/HcalObjects/interface/HcalPedestalWidths.h"
#include "CondFormats/HcalObjects/interface/HcalGains.h"
#include "CondFormats/HcalObjects/interface/HcalGainWidths.h"
#include "CondFormats/HcalObjects/interface/HcalElectronicsMap.h"
#include "CondFormats/HcalObjects/interface/HcalQIEShape.h"
#include "CondFormats/HcalObjects/interface/HcalChannelQuality.h"
#include "CondFormats/HcalObjects/interface/HcalQIEData.h"

//using namespace cms;

class Args {
 public:
  Args () {};
  ~Args () {};
  void defineOption (const std::string& fOption, const std::string& fComment = "");
  void defineParameter (const std::string& fParameter, const std::string& fComment = "");
  void parse (int nArgs, char* fArgs []);
  void printOptionsHelp () const;
  std::string command () const;
  std::vector<std::string> arguments () const;
  bool optionIsSet (const std::string& fOption) const;
  std::string getParameter (const std::string& fKey);
 private:
  std::string mProgramName;
  std::vector <std::string> mOptions;
  std::vector <std::string> mParameters;
  std::vector <std::string> mArgs;
  std::map <std::string, std::string> mParsed;
  std::map <std::string, std::string> mComments;
};

class PoolData {
 public:
  PoolData (const std::string& fConnect);
  ~PoolData ();

  template <class T>
  bool storeObject (T* fObject, const std::string& fContainer, pool::Ref<T>* fObject);

  template <class T>
  bool updateObject (T* fObject, pool::Ref<T>* fUpdate);

  template <class T>
  bool updateObject (pool::Ref<T>* fUpdate);

  template <class T>
  bool storeIOV (const pool::Ref<T>& fObject, unsigned fMaxRun, pool::Ref<cond::IOV>* fIov);

  template <class T>
  bool getObject (const pool::Ref<cond::IOV>& fIOV, unsigned fRun, pool::Ref<T>* fObject);

  template <class T> 
  bool getObject (const std::string& fToken, pool::Ref<T>* fObject);

  pool::IDataSvc* service () {return  mService;}
  std::string connect () {return mConnect;}
 private:
  std::string mConnect;
  pool::IFileCatalog mCatalog;
  pool::IDataSvc* mService;
  pool::Placement mPlacement;
};

template <class T>
std::vector<HcalDetId> undefinedCells (const T& fData) {
  static std::vector<HcalDetId> result;
  if (result.size () <= 0) {
    HcalTopology topology;
    for (int eta = -50; eta < 50; eta++) {
      for (int phi = 0; phi < 100; phi++) {
	for (int depth = 1; depth < 5; depth++) {
	  for (int det = 1; det < 5; det++) {
	    HcalDetId cell ((HcalSubdetector) det, eta, phi, depth);
	    if (topology.valid(cell) && !fData.getValues (cell.rawId())) result.push_back (cell);
	  }
	}
      }
    }
  }
  return result;
}

void fillDefaults (HcalPedestals* fPedestals) {
  std::vector<HcalDetId> cells = undefinedCells (*fPedestals);
  for (std::vector <HcalDetId>::const_iterator cell = cells.begin (); cell != cells.end (); cell++) {
    HcalPedestal item = HcalDbHardcode::makePedestal (*cell);
    fPedestals->addValue (*cell, item.getValues ());
  }
  fPedestals->sort ();
}

void fillDefaults (HcalGains* fGains) {
  std::vector<HcalDetId> cells = undefinedCells (*fGains);
  for (std::vector <HcalDetId>::const_iterator cell = cells.begin (); cell != cells.end (); cell++) {
    HcalGain item = HcalDbHardcode::makeGain (*cell);
    fGains->addValue (*cell, item.getValues ());
  }
}

void fillDefaults (HcalElectronicsMap* fMap) {
  std::cerr << "ERROR: fillDefaults (HcalElectronicsMap* fMap) is not implemented. Ignore." << std::endl;
}

void printHelp (const Args& args) {
  char buffer [1024];
  std::cout << "Tool to manipulate by Hcal Calibrations" << std::endl;
  std::cout << "    feedback -> ratnikov@fnal.gov" << std::endl;
  std::cout << "Use:" << std::endl;
  sprintf (buffer, " %s dump <what> <options> <parameters>\n", args.command ().c_str());
  std::cout << buffer;
  sprintf (buffer, " %s fill <what> <options> <parameters>\n", args.command ().c_str());
  std::cout << buffer;
  sprintf (buffer, " %s add <what> <options> <parameters>\n", args.command ().c_str());
  std::cout << buffer;
  std::cout << "  where <what> is: \n    pedestals\n    gains\n    emap\n" << std::endl;
  args.printOptionsHelp ();
}



int main (int argn, char* argv []) {

  Args args;
  args.defineParameter ("-connect", "DB connection string, POOL format");
  args.defineParameter ("-input", "input file where to get constants from");
  args.defineParameter ("-output", "output file where to dump constants to");
  args.defineParameter ("-run", "run # for which constands should be dumped");
  args.defineParameter ("-tag", "tag for the constants set");
  args.defineOption ("-help", "this help");
  args.defineOption ("-defaults", "fill default values for not specifyed cells");
  args.defineOption ("-xml", "dump constants in XML format suitable for feeling online DB");
  
  args.parse (argn, argv);
  
  std::vector<std::string> arguments = args.arguments ();

  if (arguments.size () < 2 || args.optionIsSet ("-help")) {
    printHelp (args);
    return -1;
  }

  bool dump = arguments [0] == "dump";
  bool fill = arguments [0] == "fill";
  bool add = arguments [0] == "add";
  bool getPedestals = arguments [1] == "pedestals";
  bool getGains = arguments [1] == "gains";
  bool emap = arguments [1] == "emap";
  bool defaults = args.optionIsSet ("-defaults");
  bool xml = args.optionIsSet ("-xml");

  std::string what = arguments [1];

  std::string connect = args.getParameter ("-connect");
  if (connect.empty ()) {
    std::cerr << "ERROR: -connect is mandatory parameter" << std::endl;
    return -1;
  }
  std::string tag = args.getParameter ("-tag");
  if (tag.empty ()) {
    std::cerr << "ERROR: -tag is mandatory parameter" << std::endl;
    return -1;
  }
  std::string runStr = args.getParameter ("-run");
  if (runStr.empty ()) runStr = "0";
  unsigned run = atoi (runStr.c_str());
  
  std::string metadataToken; // need to separate RAL and ORA operations

  if (dump || add) { // get metadata
    cond::MetaData md (connect);
    metadataToken = md.getToken (tag);
    if (metadataToken.empty ()) {
      std::cerr << "ERROR: can not find metadata for tag " << tag << std::cerr;
      return 2;
    }
  }
  {
    PoolData db (connect);
    
    // get IOV object
    pool::Ref<cond::IOV> iov;
    if (!metadataToken.empty ()) {
      db.getObject (metadataToken, &iov);
      if (iov.isNull ()) {
	std::cerr << "ERROR: can not find IOV for token " << metadataToken << std::endl;;
	return 2;
      }
    }
    
    if (dump) { // dump DB
      std::string output = args.getParameter ("-output");
      if (output.empty ()) output = "/dev/null";
      std::ofstream outStream (output.c_str ());
      if (getPedestals) {
	pool::Ref<HcalPedestals> ref;
	if (db.getObject (iov, run, &ref)) {
	  if (xml) {
	    HcalPedestalWidths widths; widths.sort ();
	    HcalDbXml::dumpObject (outStream, run, tag, *ref, widths);
	  }
	  else {
	    HcalDbASCIIIO::dumpObject (outStream, *ref);
	  }
	}
      }
      else if (getGains) {
	pool::Ref<HcalGains> ref;
	if (db.getObject (iov, run, &ref)) {
	  if (xml) {
	    HcalGainWidths widths; widths.sort ();
	    HcalDbXml::dumpObject (outStream, run, tag, *ref, widths);
	  }
	  else {
	    HcalDbASCIIIO::dumpObject (outStream, *ref);
	  }
	}
      }
      else if (emap) {
	pool::Ref<HcalElectronicsMap> ref;
	if (db.getObject (iov, run, &ref)) {
	  HcalDbASCIIIO::dumpObject (outStream, *ref);
	}
      }
      else {
	std::cerr << "ERROR object " << what << " is not supported" << std::endl;
      }
    }
    else if (fill || add) { // fill DB
      std::string input = args.getParameter ("-input");
      if (input.empty ()) input = "/dev/null";
      std::ifstream inStream (input.c_str ());
      
      if (getPedestals) {
	HcalPedestals* obj = new HcalPedestals ();
	HcalDbASCIIIO::getObject (inStream, obj); 
	if (defaults) fillDefaults (obj); 
	pool::Ref<HcalPedestals> ref;
	if (!db.storeObject (obj, "HcalPedestals", &ref) ||
	    !db.storeIOV (ref, run, &iov)) {
	  std::cerr << "ERROR: failed to store object or its IOV" << std::endl;
	  return 1;
	}
      }
      else if (getGains) {
	HcalGains* obj = new HcalGains ();
	HcalDbASCIIIO::getObject (inStream, obj); 
	if (defaults) fillDefaults (obj); 
	pool::Ref<HcalGains> ref;
	if (!db.storeObject (obj, "HcalGains", &ref) ||
	    !db.storeIOV (ref, run, &iov)) {
	  std::cerr << "ERROR: failed to store object or its IOV" << std::endl;
	  return 1;
	}
      }
      else if (emap) {
	HcalElectronicsMap* obj = new HcalElectronicsMap ();
	HcalDbASCIIIO::getObject (inStream, obj); 
	pool::Ref<HcalElectronicsMap> ref;
	if (defaults) fillDefaults (obj); 
	if (!db.storeObject (obj, "HcalElectronicsMap", &ref) ||
	    !db.storeIOV (ref, run, &iov)) {
	  std::cerr << "ERROR: failed to store object or its IOV" << std::endl;
	  return 1;
	}
      }
      metadataToken = iov.toString ();
    }
    else {
      std::cerr << "Unknown option. Try -help option for more details" << std::endl;
    }
  }
  
  if (fill) { // update metadata
    cond::MetaData md (connect);
    md.addMapping (tag, metadataToken);
  }
  return 0;
}



//==================== Args ===== BEGIN ==============================
void Args::defineOption (const std::string& fOption, const std::string& fComment) {
  mOptions.push_back (fOption);
  mComments [fOption] = fComment;
}

void Args::defineParameter (const std::string& fParameter, const std::string& fComment) {
  mParameters.push_back (fParameter);
  mComments [fParameter] = fComment;
}

void Args::parse (int nArgs, char* fArgs []) {
  if (nArgs <= 0) return;
  mProgramName = std::string (fArgs [0]);
  int iarg = 0;
  while (++iarg < nArgs) {
    std::string arg (fArgs [iarg]);
    if (arg [0] != '-') mArgs.push_back (arg);
    else {
      if (std::find (mOptions.begin(), mOptions.end (), arg) !=  mOptions.end ()) {
	mParsed [arg] = "";
      }
      if (std::find (mParameters.begin(), mParameters.end (), arg) !=  mParameters.end ()) {
	if (iarg >= nArgs) {
	  std::cerr << "ERROR: Parameter " << arg << " has no value specified. Ignore parameter." << std::endl;
	}
	else {
	  mParsed [arg] = std::string (fArgs [++iarg]);
	}
      }
    }
  }
}

void Args::printOptionsHelp () const {
  char buffer [1024];
  std::cout << "Parameters:" << std::endl;
  for (unsigned i = 0; i < mParameters.size (); i++) {
    std::map<std::string, std::string>::const_iterator it = mComments.find (mParameters [i]);
    std::string comment = it != mComments.end () ? it->second : "uncommented";
    sprintf (buffer, "  %-8s <value> : %s", (mParameters [i]).c_str(),  comment.c_str());
    std::cout << buffer << std::endl;
  }
  std::cout << "Options:" << std::endl;
  for (unsigned i = 0; i < mOptions.size (); i++) {
    std::map<std::string, std::string>::const_iterator it = mComments.find (mOptions [i]);
    std::string comment = it != mComments.end () ? it->second : "uncommented";
    sprintf (buffer, "  %-8s  : %s", (mOptions [i]).c_str(),  comment.c_str());
    std::cout << buffer << std::endl;
  }
}

std::string Args::command () const {
  int ipos = mProgramName.rfind ('/');
  return std::string (mProgramName, ipos+1);
}

std::vector<std::string> Args::arguments () const {return mArgs;}

bool Args::optionIsSet (const std::string& fOption) const {
  return mParsed.find (fOption) != mParsed.end ();
}

std::string Args::getParameter (const std::string& fKey) {
  if (optionIsSet (fKey)) return mParsed [fKey];
  return "";
}
//==================== Args ===== END ==============================

//==================== PoolData ===== BEGIN ==============================
PoolData::PoolData (const std::string& fConnect) 
  : mConnect (fConnect)
{
    std::cout << "PoolData-> Using DB connection: " << fConnect << std::endl; 
    seal::PluginManager::get()->initialise();
    pool::POOLContext::loadComponent( "SEAL/Services/MessageService" );
    pool::POOLContext::loadComponent( "POOL/Services/EnvironmentAuthenticationService" );
    
    pool::URIParser parser;
    parser.parse();
    
    //	mCatalog.setWriteCatalog (parser.contactstring ());
    mCatalog.setWriteCatalog(parser.contactstring());
    mCatalog.connect();
    mCatalog.start();
    
    mService = pool::DataSvcFactory::create (&mCatalog);
    
    pool::DatabaseConnectionPolicy policy;  
    policy.setWriteModeForNonExisting(pool::DatabaseConnectionPolicy::CREATE);
    policy.setWriteModeForExisting(pool::DatabaseConnectionPolicy::UPDATE); 
    mService->session().setDefaultConnectionPolicy(policy);
    mPlacement.setDatabase(mConnect, pool::DatabaseSpecification::PFN); 
    mPlacement.setTechnology(pool::POOL_RDBMS_StorageType.type());
  }

PoolData::~PoolData () {
  mService->session().disconnectAll();
  mCatalog.commit ();
  mCatalog.disconnect ();
  delete mService;
}

template <class T>
bool PoolData::storeObject (T* fObject, const std::string& fContainer, pool::Ref<T>* fRef) {
  if (!fRef->isNull ()) {
    std::cerr << "PoolData::storeObject-> Ref is not empty. Ignore." << std::endl;
    return false;
  }
  try {
    mService->transaction().start(pool::ITransaction::UPDATE);
    *fRef = pool::Ref <T> (mService, fObject);
    mPlacement.setContainerName (fContainer);
    fRef->markWrite (mPlacement);
    mService->transaction().commit();
  }
  catch (...) {
    std::cerr << "PoolData::storeObject  error "  << std::endl;
    return false;
  }
  return true;
} 

template <class T>
bool PoolData::updateObject (T* fObject, pool::Ref<T>* fUpdate) {
  try {
    mService->transaction().start(pool::ITransaction::UPDATE);
    if (fObject) *(fUpdate->ptr ()) = *fObject; // update object
    fUpdate->markUpdate();
    mService->transaction().commit();
  }
  catch (...) {
    std::cerr << "PoolData::updateObject  error "  << std::endl;
    return false;
  }
  return true;
}

template <class T>
bool PoolData::updateObject (pool::Ref<T>* fUpdate) {
  return updateObject ((T*)0, fUpdate);
}

template <class T>
bool PoolData::storeIOV (const pool::Ref<T>& fObject, unsigned fMaxRun, pool::Ref<cond::IOV>* fIov) {
  unsigned maxRun = fMaxRun == 0 ? 0xffffffff : fMaxRun;
  if (fIov->isNull ()) {
    cond::IOV* newIov = new cond::IOV ();
    newIov->iov.insert (std::make_pair (maxRun, fObject.toString ()));
    return storeObject (newIov, "IOV", fIov);
  }
  else {
    (*fIov)->iov.insert (std::make_pair (maxRun, fObject.toString ()));
    return updateObject (fIov);
  }
}

template <class T>
bool PoolData::getObject (const pool::Ref<cond::IOV>& fIOV, unsigned fRun, pool::Ref<T>* fObject) {
  if (!fIOV.isNull ()) {
    // scan IOV, search for valid data
    for (std::map<unsigned long,std::string>::iterator iovi = fIOV->iov.begin (); iovi != fIOV->iov.end (); iovi++) {
      std::cout << "PoolData::getObject-> size: " << fIOV->iov.size () << " run:" << iovi->first << std::endl;
      if (fRun <= iovi->first) {
	std::string token = iovi->second;
	return getObject (token, fObject);
      }
    }
    std::cerr << "PoolData::getObject-> no object for run " << fRun << " is found" << std::endl;
  }
  else {
    std::cerr << "PoolData::getObject-> IOV reference is not set" << std::endl;
  }
  return false;
}

template <class T> 
bool PoolData::getObject (const std::string& fToken, pool::Ref<T>* fObject) {
  try {
    *fObject = pool::Ref <T> (service (), fToken);
    service ()->transaction().start(pool::ITransaction::READ);
    fObject->isNull ();
    mService->transaction().commit();
  }
  catch(const coral::TableNotExistingException& e) {
    std::cerr << "PoolData::getObject-> coral::TableNotExisting Exception" << std::endl;
  }
  catch (const seal::Exception& e) {
    std::cerr<<"PoolData::getObject-> seal exception: " << e.what() << std::endl;
  }
  catch(...){
    std::cerr << "PoolData::getObject-> Funny error" << std::endl;
  }
  return !(fObject->isNull ());
}

//============== PoolData ===== END =====================
