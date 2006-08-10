
/**
   \class HcalDbPOOL
   \brief IO for POOL instances of Hcal Calibrations
   \author Fedor Ratnikov Oct. 28, 2005
   $Id: HcalDbPool.cc,v 1.13 2006/07/26 21:00:31 fedor Exp $
*/

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

// Coral
#include "RelationalAccess/IRelationalService.h"
#include "RelationalAccess/RelationalServiceException.h"
#include "RelationalAccess/IRelationalDomain.h"
#include "RelationalAccess/SchemaException.h"
#include "RelationalAccess/ISession.h"
#include "RelationalAccess/ITransaction.h"
#include "RelationalAccess/ISchema.h"
#include "RelationalAccess/ITable.h"
#include "RelationalAccess/TableDescription.h"
#include "RelationalAccess/ITablePrivilegeManager.h"
#include "RelationalAccess/IPrimaryKey.h"
#include "RelationalAccess/ICursor.h"
#include "RelationalAccess/IQuery.h"
#include "RelationalAccess/ITableDataEditor.h"
#include "CoralBase/Exception.h"
#include "CoralBase/AttributeList.h"
#include "CoralBase/AttributeSpecification.h"
#include "CoralBase/Attribute.h"

// Seal
#include "SealKernel/Context.h"
#include "SealKernel/ComponentLoader.h"


// conditions
#include "CondCore/IOVService/interface/IOV.h"

#include "CondFormats/HcalObjects/interface/AllObjects.h"

#include "CondTools/Hcal/interface/HcalDbPool.h"

namespace {
  pool::Ref<cond::IOV> iovCache;
}

typedef std::map<unsigned long long,std::string> IOVCollection;

template <class T>
bool HcalDbPool::storeObject (T* fObject, const std::string& fContainer, pool::Ref<T>* fRef) {
  if (mVerbose) std::cout << "HcalDbPool::storeObject-> start..." << std::endl;
  if (!fRef->isNull ()) {
    std::cerr << "storeObject-> Ref is not empty. Ignore." << std::endl;
    return false;
  }
  try {
    service ()->transaction().update();
    if (mVerbose) std::cout << "transaction is active: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
    if (mVerbose) std::cout << "transaction ---> start" << std::endl;
    service ()->transaction().start(pool::ITransaction::UPDATE);
    if (mVerbose) std::cout << "transaction is active1: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
    
    *fRef = pool::Ref <T> (service (), fObject);
    if (mVerbose) std::cout << "transaction is active2: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
    mPlacement->setContainerName (fContainer);
    if (mVerbose) std::cout << "transaction is active3: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
    fRef->markWrite (*mPlacement);
    if (mVerbose) std::cout << "transaction is active4: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
    service ()->transaction().commit();
    if (mVerbose) std::cout << "transaction ---> commit" << std::endl;
    if (mVerbose) std::cout << "transaction is active5: " << service ()->transaction().isActive() << ", type=" << (int) service ()->transaction().type() << std::endl;
  }
  catch (pool::Exception& e) {
    std::cerr << "storeObject->  POOL error: "  << e.what() << std::endl;
    return false;
  }
  catch (const std::exception& e) {
    std::cerr << "storeObject->  standard error: "  << e.what() << std::endl;
    return false;
  }
   catch (...) {
     std::cerr << "storeObject->  not standard error "  << std::endl;
     return false;
   }
  if (mVerbose) std::cout << "HcalDbPool::storeObject-> end..." << std::endl;
  return true;
} 

template <class T>
bool HcalDbPool::updateObject (T* fObject, pool::Ref<T>* fUpdate) {
  if (mVerbose) std::cout << "HcalDbPool::updateObject-> start..." << std::endl;
  try {
    if (mVerbose) std::cout << "transaction ---> start" << std::endl;
    service ()->transaction().start(pool::ITransaction::UPDATE);
    if (fObject) *(fUpdate->ptr ()) = *fObject; // update object
    fUpdate->markUpdate();
    service ()->transaction().commit();
    if (mVerbose) std::cout << "transaction ---> commit" << std::endl;
  }
  catch (const pool::Exception& e) {
    std::cerr<<"updateObject-> POOL error: " << e.what() << std::endl;
  }
  catch (std::exception& e) {
    std::cerr << "updateObject->  error: " << e.what () << std::endl;
    return false;
  }
  catch (...) {
    std::cerr << "HcalDbPool::updateObject-> General error" << std::endl;
  }
  if (mVerbose) std::cout << "HcalDbPool::updateObject-> end..." << std::endl;
  return true;
}

template <class T>
bool HcalDbPool::updateObject (pool::Ref<T>* fUpdate) {
  return updateObject ((T*)0, fUpdate);
}

template <class T>
bool HcalDbPool::storeIOV (const pool::Ref<T>& fObject, unsigned fMaxRun, pool::Ref<cond::IOV>* fIov) {
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
bool HcalDbPool::getObject (const pool::Ref<cond::IOV>& fIOV, unsigned fRun, pool::Ref<T>* fObject) {
  if (!fIOV.isNull ()) {
    // scan IOV, search for valid data
    for (IOVCollection::iterator iovi = fIOV->iov.begin (); iovi != fIOV->iov.end (); iovi++) {
      if (fRun <= iovi->first) {
	std::string token = iovi->second;
	return getObject (token, fObject);
      }
    }
    std::cerr << "getObject-> no object for run " << fRun << " is found" << std::endl;
  }
  else {
    std::cerr << "getObject-> IOV reference is not set" << std::endl;
  }
  return false;
}

template <class T> 
bool HcalDbPool::getObject (const std::string& fToken, pool::Ref<T>* fObject) {
  if (mVerbose) std::cout << "HcalDbPool::getObject-> start..." << std::endl;
  try {
    *fObject = pool::Ref <T> (service (), fToken);
    if (mVerbose) std::cout << "transaction ---> start" << std::endl;
    service ()->transaction().start(pool::ITransaction::READ);
    fObject->isNull ();
    service ()->transaction().commit();
    if (mVerbose) std::cout << "transaction ---> commit" << std::endl;
  }
  catch(const coral::TableNotExistingException& e) {
    std::cerr << "getObject-> coral::TableNotExisting Exception" << std::endl;
  }
  catch (const pool::Exception& e) {
    std::cerr<<"getObject-> POOL error: " << e.what() << std::endl;
  }
  catch(...){
    std::cerr << "getObject-> Funny error" << std::endl;
  }
  if (mVerbose) std::cout << "HcalDbPool::getObject-> end..." << std::endl;
  return !(fObject->isNull ());
}

template <class T>
bool HcalDbPool::getObject_ (T* fObject, const std::string& fTag, int fRun) {
  std::string metadataToken = metadataGetToken (fTag);
  if (metadataToken.empty ()) {
    std::cerr << "HcalDbPool::getObject ERROR-> Can not find metadata for tag " << fTag << std::endl;
    return false;
  }
  if (iovCache.toString () != metadataToken) {
    getObject (metadataToken, &iovCache);
  }
  if (iovCache.isNull ()) {
    std::cerr << "HcalDbPool::getObject ERROR: can not find IOV for token " << metadataToken << std::endl;;
    return false;
  }
  pool::Ref<T> ref;
  if (getObject (iovCache, fRun, &ref)) {
    *fObject = *ref; // make copy
    return true;
  }
  return false;
}

template <class T>
bool HcalDbPool::putObject_ (T* fObject, const std::string& fClassName, const std::string& fTag, int fRun) {
  std::string metadataToken = metadataGetToken (fTag);
  pool::Ref<cond::IOV> iov;
  if (!metadataToken.empty ()) {
    getObject (metadataToken, &iov);
    if (iov.isNull ()) {
      std::cerr << "HcalDbPool::putObject ERROR: can not find IOV for token " << metadataToken << std::endl;;
      return false;
    }
  }
  bool create = iov.isNull ();
  pool::Ref<T> ref;
  if (!storeObject (fObject, fClassName, &ref) ||
      !storeIOV (ref, fRun, &iov)) {
    std::cerr << "ERROR: failed to store object or its IOV" << std::endl;
    return false;
  }
  if (create) {
    std::string token = iov.toString ();
    metadataSetTag (fTag, token);
  }
  return true;
}

HcalDbPool::HcalDbPool (const std::string& fConnect, bool fVerbose)
  : mConnect (fConnect),
    mVerbose (fVerbose) {
  if (mVerbose) std::cout << "HcalDbPool::HcalDbPool started..." << std::endl;
  seal::PluginManager::get()->initialise();
  mContext.reset (new seal::Context);
  seal::Handle<seal::ComponentLoader> loader = new seal::ComponentLoader (&*mContext);
  loader->load( "SEAL/Services/MessageService" );
  loader->load( "CORAL/Services/EnvironmentAuthenticationService" );
  loader->load( "CORAL/Services/RelationalService");
  mTag.clear ();
  if (!::getenv("CORAL_AUTH_USER")) ::putenv("CORAL_AUTH_USER=blah");
  if (!::getenv("CORAL_AUTH_PASSWORD")) ::putenv("CORAL_AUTH_PASSWORD=blah"); 
  service ();
  if (mVerbose) std::cout << "HcalDbPool::HcalDbPool done..." << std::endl;
}

HcalDbPool::~HcalDbPool () {
  if (mVerbose) std::cout << "HcalDbPool::~HcalDbPool started..." << std::endl;
  mService->session().disconnectAll();
  mCatalog->commit ();
  mCatalog->disconnect ();
  if (mVerbose) std::cout << "HcalDbPool::~HcalDbPool done..." << std::endl;
}


pool::IDataSvc* HcalDbPool::service ()
{
  if (!mService.get ()) {
    if (mVerbose) std::cout << "HcalDbPool::service () started..." << std::endl;
    try {
      pool::URIParser parser;
      parser.parse();
      
      mCatalog.reset (new pool::IFileCatalog ());
      if (mVerbose) std::cout << "HcalDbPool::service ()->parser.contactstring() " << parser.contactstring() << std::endl;
      mCatalog->setWriteCatalog(parser.contactstring());
      mCatalog->connect();
      mCatalog->start();
      
      mService.reset (pool::DataSvcFactory::create (&*mCatalog));
      
      pool::DatabaseConnectionPolicy policy;  
      policy.setWriteModeForNonExisting(pool::DatabaseConnectionPolicy::CREATE);
      policy.setWriteModeForExisting(pool::DatabaseConnectionPolicy::UPDATE); 
      mService->session().setDefaultConnectionPolicy(policy);
      mPlacement.reset (new pool::Placement ());
      mPlacement->setDatabase(mConnect, pool::DatabaseSpecification::PFN); 
      mPlacement->setTechnology(pool::POOL_RDBMS_StorageType.type());
    }
    catch (const pool::Exception& e) {
      std::cerr<<"HcalDbPool::service ()-> POOL error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
      std::cerr << "HcalDbPool::service ()->  standard error: "  << e.what() << std::endl;
      return false;
    }
    catch (...) {
      std::cerr << "HcalDbPool::service ()-> General error" << std::endl;
    }
    if (mVerbose) std::cout << "HcalDbPool::service () done..." << std::endl;
  }
  return mService.get ();
}

std::auto_ptr<coral::ISession> HcalDbPool::session () {
  std::auto_ptr<coral::ISession> result;
  service ();
  if (mVerbose) std::cout << "HcalDbPool::session () started..." << std::endl;
  try {
    std::vector< seal::IHandle<coral::IRelationalService> > v_svc;
    mContext->query( v_svc );
    if ( v_svc.empty() ) {
      std::cerr <<  "HcalDbPool::session () -> Could not locate the relational service" << std::endl;
    }
    else {
      seal::IHandle<coral::IRelationalService>& relationalService = v_svc.front();
      coral::IRelationalDomain& domain = relationalService->domainForConnection (mConnect);
      result.reset (domain.newSession (mConnect));
      result->connect ();
      result->startUserSession(); // What is that???
    }
  }
  catch (const pool::Exception& e) {
    std::cerr<<"HcalDbPool::session ()-> POOL error: " << e.what() << std::endl;
  }
  catch (const std::exception& e) {
    std::cerr << "HcalDbPool::session ()->  standard error: "  << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "HcalDbPool::session ()-> General error" << std::endl;
  }
  if (mVerbose) std::cout << "HcalDbPool::session () done..." << std::endl;
  return result;
}

namespace {
  const std::string METADATA_TABLE ("METADATA");
  const std::string METADATA_TAG_COLUMN ("NAME");
  const std::string METADATA_TOKEN_COLUMN ("TOKEN");
}

std::vector<std::string> HcalDbPool::metadataAllTags () {
  std::vector<std::string> result;
  std::auto_ptr<coral::ISession> coralSession;
  try {
    coralSession = session ();
    if (mVerbose) std::cout << "HcalDbPool::metadataAllTags->connecting..." << std::endl;
    coralSession->transaction().start(true);
    if (coralSession->nominalSchema().existsTable(METADATA_TABLE)) {
      coral::ITable& mytable=coralSession->nominalSchema().tableHandle (METADATA_TABLE);
      std::auto_ptr< coral::IQuery > query(mytable.newQuery());
      query->setRowCacheSize( 100 );
      coral::AttributeList emptyBindVariableList;
      query->addToOutputList (METADATA_TAG_COLUMN);
      coral::ICursor& cursor = query->execute();
      while( cursor.next() ) {
	const coral::AttributeList& row = cursor.currentRow();
	result.push_back (row [METADATA_TAG_COLUMN].data<std::string>());
      }
    }
    coralSession->transaction().commit();
    coralSession->disconnect ();
  }
  catch (const pool::Exception& e) {
    coralSession->transaction().rollback();
    coralSession->disconnect ();
    std::cerr<<"HcalDbPool::metadataAllTags-> POOL error: " << e.what() << std::endl;
    mToken.clear ();
  }
  catch (...) {
    coralSession->transaction().rollback();
    coralSession->disconnect ();
    std::cerr << "HcalDbPool::metadataAllTags-> General error" << std::endl;
    mToken.clear ();
  }
  return result;
}

const std::string& HcalDbPool::metadataGetToken (const std::string& fTag) {
  if (mTag != fTag) {
    mTag = fTag;
    mToken.clear ();
    std::auto_ptr<coral::ISession> coralSession;
    try {
      coralSession = session ();
      if (mVerbose) std::cout << "HcalDbPool::metadataGetToken->connecting..." << std::endl;
      if (mVerbose) std::cout << "transaction ---> start" << std::endl;
      if (mVerbose) std::cout << "metadataGetToken->transaction is active1: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
      coralSession->transaction().start(true);
      if (mVerbose) std::cout << "metadataGetToken->transaction is active2: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
      if (coralSession->nominalSchema().existsTable(METADATA_TABLE)) {
	coral::ITable& mytable=coralSession->nominalSchema().tableHandle (METADATA_TABLE);
	std::auto_ptr< coral::IQuery > query(mytable.newQuery());
	query->setRowCacheSize( 100 );
	coral::AttributeList emptyBindVariableList;
	std::string condition = METADATA_TAG_COLUMN + " = '" + fTag + "'";
	query->setCondition( condition, emptyBindVariableList );
	query->addToOutputList (METADATA_TOKEN_COLUMN);
	coral::ICursor& cursor = query->execute();
	while( cursor.next() ) {
	  const coral::AttributeList& row = cursor.currentRow();
	  mToken = row [METADATA_TOKEN_COLUMN].data<std::string>();
	  break; // ignore other tokens
	}
      }
      if (mVerbose) std::cout << "metadataGetToken->transaction is active3: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
      coralSession->transaction().commit();
      if (mVerbose) std::cout << "metadataGetToken->transaction is active4: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
      if (mVerbose) std::cout << "transaction ---> commit" << std::endl;
      coralSession->disconnect ();
      if (mVerbose) std::cout << "HcalDbPool::metadataGetToken->disconnecting..." << std::endl;
    }
    catch (const pool::Exception& e) {
      coralSession->transaction().rollback();
      coralSession->disconnect ();
      std::cerr<<"HcalDbPool::metadataGetToken-> POOL error: " << e.what() << std::endl;
      mToken.clear ();
    }
    catch (...) {
      coralSession->transaction().rollback();
      coralSession->disconnect ();
      std::cerr << "HcalDbPool::metadataGetToken-> General error" << std::endl;
      mToken.clear ();
    }
    if (mToken.empty ()) mTag.clear ();
  }
  if (mVerbose) std::cout << "HcalDbPool::metadataGetToken-> " << fTag << '/' << mToken << std::endl;
  return mToken;
}

bool HcalDbPool::metadataSetTag (const std::string& fTag, const std::string& fToken) {
  if (mVerbose) std::cout << "HcalDbPool::metadataSetTag->begin..." << std::endl;
  bool result = false;
  std::auto_ptr<coral::ISession> coralSession;
  try {
    coralSession = session ();
    if (mVerbose) std::cout << "transaction ---> start" << std::endl;
    if (mVerbose) std::cout << "metadataSetToken->transaction is active1: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
    coralSession->transaction().start(false);
    if (mVerbose) std::cout << "metadataSetToken->transaction is active2: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
    if (!coralSession->nominalSchema().existsTable(METADATA_TABLE)) { // make new table
      coral::ISchema& schema=coralSession->nominalSchema();
      coral::TableDescription description;
      description.setName (METADATA_TABLE);
      description.insertColumn (METADATA_TAG_COLUMN, coral::AttributeSpecification::typeNameForId( typeid(std::string)) );
      description.insertColumn (METADATA_TOKEN_COLUMN, coral::AttributeSpecification::typeNameForId( typeid(std::string)) );
      std::vector<std::string> cols;
      cols.push_back (METADATA_TAG_COLUMN);
      description.setPrimaryKey (cols);
      description.setNotNullConstraint (METADATA_TOKEN_COLUMN);
      coral::ITable& table=schema.createTable(description);
      table.privilegeManager().grantToPublic( coral::ITablePrivilegeManager::Select);
    }
    coral::ITable& mytable=coralSession->nominalSchema().tableHandle (METADATA_TABLE);
    coral::AttributeList rowBuffer;
    coral::ITableDataEditor& dataEditor = mytable.dataEditor();
    dataEditor.rowBuffer( rowBuffer );
    rowBuffer[METADATA_TAG_COLUMN].data<std::string>() = fTag;
    rowBuffer[METADATA_TOKEN_COLUMN].data<std::string>() = fToken;
    dataEditor.insertRow( rowBuffer );
    if (mVerbose) std::cout << "metadataSetToken->transaction is active3: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
    coralSession->transaction().commit() ;
    if (mVerbose) std::cout << "metadataSetToken->transaction is active4: " << coralSession->transaction().isActive() << ", readOnly?" << coralSession->transaction().isReadOnly() << std::endl;
    if (mVerbose) std::cout << "transaction ---> commit" << std::endl;
    coralSession->disconnect ();
    if (mVerbose) std::cout << "HcalDbPool::metadataSetTag->disconnect..." << std::endl;
   result = true;
  }
  catch (const pool::Exception& e) {
    coralSession->transaction().rollback();
    coralSession->disconnect ();
    std::cerr<<"HcalDbPool::metadataSetTag-> POOL error: " << e.what() << std::endl;
    result = false;
  }
  catch (...) {
    coralSession->transaction().rollback();
    coralSession->disconnect ();
    std::cerr << "HcalDbPool::metadataSetTag-> General error" << std::endl;
    result = false;
  }
   if (mVerbose) std::cout << "HcalDbPool::metadataSetTag-> " << fTag << '/' << fToken << std::endl;
  return result;
}

bool HcalDbPool::getObject (cond::IOV* fObject, const std::string& fTag) {
  if (!fObject) return false;
  std::string metadataToken = metadataGetToken (fTag);
  if (metadataToken.empty ()) {
    std::cerr << "HcalDbPool::getObject IOV ERROR-> Can not find metadata for tag " << fTag << std::endl;
    return false;
  }
  if (iovCache.toString () != metadataToken) {
    getObject (metadataToken, &iovCache);
  }
  if (iovCache.isNull ()) {
    std::cerr << "HcalDbPool::getObject ERROR: can not find IOV for token " << metadataToken << std::endl;;
    return false;
  }
  *fObject = *iovCache;
  return true;
}

bool HcalDbPool::getObject (HcalPedestals* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalPedestals* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalPedestals", fTag, fRun);}
bool HcalDbPool::getObject (HcalPedestalWidths* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalPedestalWidths* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalPedestalWidths", fTag, fRun);}
bool HcalDbPool::getObject (HcalGains* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalGains* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalGains", fTag, fRun);}
bool HcalDbPool::getObject (HcalGainWidths* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalGainWidths* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalGainWidths", fTag, fRun);}
bool HcalDbPool::getObject (HcalQIEData* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalQIEData* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalQIEData", fTag, fRun);}
bool HcalDbPool::getObject (HcalCalibrationQIEData* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalCalibrationQIEData* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalQIEData", fTag, fRun);}
bool HcalDbPool::getObject (HcalChannelQuality* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalChannelQuality* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalChannelQuality", fTag, fRun);}
bool HcalDbPool::getObject (HcalElectronicsMap* fObject, const std::string& fTag, int fRun) {return getObject_ (fObject, fTag, fRun);}
bool HcalDbPool::putObject (HcalElectronicsMap* fObject, const std::string& fTag, int fRun) {return putObject_ (fObject, "HcalElectronicsMap", fTag, fRun);}
