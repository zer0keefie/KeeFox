/*
  KeeFox - Allows Firefox to communicate with KeePass (via the KeeICE KeePass-plugin)
  Copyright 2008 Chris Tomlinson <keefox@christomlinson.name>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// This is the implemenation of the XPCOM methods described in comp.idl

#include "comp-impl.h"
#include "nsICategoryManager.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsILocalFile.h"
#include "nsILoginInfo.h"
#include "nsIProxyObjectManager.h"

#include <string>
#include <vector>
#include <iostream>
//#include "nsILoginInfo.h"

using namespace std;
using namespace KeeICE::KFlib;

using std::string;
using std::vector;

//NS_IMPL_ISUPPORTS1(CKeePass, IKeePass)
NS_IMPL_ADDREF(CKeeFox)
NS_IMPL_RELEASE(CKeeFox)

NS_INTERFACE_MAP_BEGIN(CKeeFox)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, CKeeFox)
  NS_INTERFACE_MAP_ENTRY(IKeeFox)
NS_INTERFACE_MAP_END

/*
NS_INTERFACE_MAP_BEGIN(CKeeFox)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, CKeeFox)
  NS_INTERFACE_MAP_ENTRY(nsILoginManagerStorage)
  NS_INTERFACE_MAP_ENTRY(IKeeFox)
NS_INTERFACE_MAP_END
*/

KeeFoxObserver *KeeFoxCallBackHelper::_observer = NULL;
bool KeeFoxCallBackHelper::javascriptCallBacksReady = false;


class CallbackReceiverI : public CallbackReceiver
{
public:

    virtual void
    callback(Ice::Int num, const Ice::Current&)
    {
		cout << "received callback ###" << num << endl;
		
		// send an asynchronous callback to the javascript observer. We don't
		// realy care if the observer has gone since there's nothing we can
		// do about that.
		if (KeeFoxCallBackHelper::javascriptCallBacksReady)
			KeeFoxCallBackHelper::_observer->CallBackToKeeFoxJS(num);

    }
};

NS_IMETHODIMP CKeeFox::AddObserver(KeeFoxObserver *observer)
{
	nsresult rv = NS_OK;

	// get the proxy manager object
	nsCOMPtr<nsIProxyObjectManager> pIProxyObjectManager =
	do_GetService("@mozilla.org/xpcomproxy;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	// use proxy manager to create a proxy object for the observer we've been given
	// nsnull -> send proxied calls to the main Firefox UI thread
	rv = pIProxyObjectManager->GetProxyForObject( nsnull,
                                                  KeeFoxObserver::GetIID(),
                                                  observer,
                                                  0x0002 | 0x0004,
                                                  (void**)&KeeFoxCallBackHelper::_observer);
	// 0x0001 = synchronous, 0x0002 = async
	
	//TODO: these lines probably need to be uncommented to prevent memory leaks
    // we do not care about the real object anymore. ie. GetProxyObject
    // refcounts it.
	//NS_RELEASE(observer);
	//KeeFoxCallBackHelper::_observer->Test1(x,y,z);
	//NS_RELEASE(KeeFoxCallBackHelper::_observer);


	//KeeFoxCallBackHelper::_observer->SetJavascriptCallBacksReady((PRBool)true);
	KeeFoxCallBackHelper::javascriptCallBacksReady = true;


	// enum for status thoughts:
	//KF_STATUS_JSCALLBACKS_DISABLED 0
	//KF_STATUS_JSCALLBACKS_SETUP 1 // CALL THIS ONE FROM RIGHT HERE AS A QUICK SETUP SANITY TEST
	//KF_STATUS_ICECALLBACKS_SETUP 2
	//KF_STATUS_DATABASE_OPENING 3
	//KF_STATUS_DATABASE_OPEN 4
	//KF_STATUS_DATABASE_CLOSING 5
	//KF_STATUS_DATABASE_CLOSED 6
	//KF_STATUS_DATABASE_SAVING 7
	//KF_STATUS_DATABASE_SAVED 8
	//KF_STATUS_DATABASE_DELETING 9
	//KF_STATUS_DATABASE_DELETED 10

	KeeFoxCallBackHelper::_observer->CallBackToKeeFoxJS(1);

	return 0;
}

CKeeFox::CKeeFox()
{
	/* member initializers and constructor code */
	//mName.Assign(L"Default Name");
	//int status = 0;
	//string DBName = "nope";

	
    //app.run(0, NULL);
	char* argv[1];
	argv[0] = "no";
KeeICE.main(0, argv);
    
    
}

CKeeFox::~CKeeFox()
{
	/* destructor code */
	if (KeeICE.ic)
        KeeICE.ic->destroy();
}

int KeeICEProxy::run(int, char*[]) {
	return establishICEConnection();
}

//TODO: adjust ICE thread connection size?
int KeeICEProxy::establishICEConnection() {


        int status = 0;
		//string DBName = "nope";
		
		try {

			// Get the initialized property set.
			//
			Ice::PropertiesPtr props = Ice::createProperties();

			// Make sure that network and protocol tracing are off.
			//
			props->setProperty("Ice.ACM.Client", "0");
			props->setProperty("Ice.ThreadPool.Client.Size", "2");
			props->setProperty("Ice.ThreadPool.Server.Size", "2");
			props->setProperty("Ice.ThreadPool.Client.SizeMax", "20");
			props->setProperty("Ice.ThreadPool.Server.SizeMax", "20");

			//props->setProperty("Ice.Trace.Protocol", "0");

			// Initialize a communicator with these properties.
			//
			Ice::InitializationData id;
			id.properties = props;
			ic = Ice::initialize(id);

			//ic = Ice::initialize();
			Ice::ObjectPrx base = ic->stringToProxy(
									"KeeICE:default -p 10000");
			KP = KPPrx::checkedCast(base);
			if (!KP)
				throw "Invalid proxy";

			Ice::ObjectAdapterPtr adapter =	ic->createObjectAdapter("");
			Ice::Identity ident;
			ident.name = IceUtil::generateUUID();
			ident.category = "";
			//CallbackPtr cb = new CallbackI;
			CallbackReceiverPtr cb = new CallbackReceiverI;
			adapter->add(cb, ident);
			adapter->activate();
			KP->ice_getConnection()->setAdapter(adapter);
			KP->addClient(ident);

//    communicator()->waitForShutdown();

			;
		} catch (const Ice::Exception& ex) {
			cerr << ex << endl;
			status = 1;
		} catch (const char* msg) {
			cerr << msg << endl;
			status = 1;
		} catch (...) {
			//cerr << ex << endl;
			status = 1;
		}

        return status;
    }

NS_IMETHODIMP CKeeFox::GetDBName(nsAString &_retval)
{
	int status = 0;
	string DBName = "nope";

    try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		DBName = KeeICE.KP->getDatabaseName();
        ;
    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	// Convert response to XPCOM string
	_retval = NS_ConvertUTF8toUTF16(DBName.c_str());
	
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::CheckVersion(float keeFoxVersion, float keeICEVersion, PRInt32 *result, PRBool *_retval)
{
	int status = 0;
	//string DBName = "nope";

    try {
		if (!KeeICE.KP)
		{
			status = KeeICE.establishICEConnection();
		}
		//throw "Proxy went away";
		if (!status)
			*_retval = KeeICE.KP->checkVersion(keeFoxVersion, keeICEVersion, *result);
        
    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	// Convert response to XPCOM string
	//_retval = NS_ConvertUTF8toUTF16(DBName.c_str());
	
	return NS_OK;
}

/* long add (in long a, in long b); - for early testing */
NS_IMETHODIMP CKeeFox::Add(PRInt32 a, PRInt32 b, PRInt32 *_retval)
{
	//*_retval = a + b;
	//Test *tes = new Test();
	//*_retval = tes->Add(a,b);
	//IKeePassFirefoxWrapper *wrapper = IKeePassFirefoxWrapper::CreateInstance();
    //*_retval = wrapper->Add3(a,b);
	//string temp = wrapper->test();
	*_retval = 222;

    //IKeePassFirefoxWrapper::Terminate(wrapper);

	//int test = KeepassFirefoxToolbarExt::Add3(a,b);
	//*_retval = Add2(a,b);
	return NS_OK;
}

//var theActualObject = arg.wrappedJSObject.object; 

/* attribute AString name; */
NS_IMETHODIMP CKeeFox::GetName(nsAString & aName)
{
	aName.Assign(mName);
	return NS_OK;
}
NS_IMETHODIMP CKeeFox::SetName(const nsAString & aName)
{
	mName.Assign(aName);
	return NS_OK;
}








//TODO: later- implement deleting


NS_IMETHODIMP CKeeFox::AddLogin(kfILoginInfo *aLogin)
{
  NS_ENSURE_ARG_POINTER(aLogin);

#if _DEBUG
  std::cout << "comp-impl.cpp::AddLogin - start" << "\n";
#endif

KeeICE::KFlib::KPEntry entry;
KPFormFieldList *formFieldList = new KPFormFieldList();
entry.formFieldList = *formFieldList;

	nsString hostname;
	(void)aLogin->GetHostname(hostname);
	entry.hostName = NS_ConvertUTF16toUTF8(hostname).get(); 
	
	entry.title = entry.hostName;

	nsString formURL;
	(void)aLogin->GetFormSubmitURL(formURL);
	entry.formURL = NS_ConvertUTF16toUTF8(formURL).get();
	
	nsString httpRealm;
	(void)aLogin->GetHttpRealm(httpRealm);
	entry.HTTPRealm = NS_ConvertUTF16toUTF8(httpRealm).get(); 

	nsString username;
	(void)aLogin->GetUsername(username);
	nsString usernameField;
	(void)aLogin->GetUsernameField(usernameField);

	KPFormField *usernameFF = new KPFormField();
	usernameFF->name = NS_ConvertUTF16toUTF8(usernameField).get(); 
	usernameFF->value = NS_ConvertUTF16toUTF8(username).get();
	usernameFF->type = formFieldType::FFTusername;

	entry.formFieldList.push_back(*usernameFF);

	nsString password;
	(void)aLogin->GetPassword(password);
	nsString passwordField;
	(void)aLogin->GetPasswordField(passwordField);

	KPFormField *passwordFF = new KPFormField();
	passwordFF->name = NS_ConvertUTF16toUTF8(passwordField).get(); 
	passwordFF->value = NS_ConvertUTF16toUTF8(password).get();
	passwordFF->type = formFieldType::FFTpassword;

	entry.formFieldList.push_back(*passwordFF);

	nsCOMPtr<nsIMutableArray> customFields;
	(void)aLogin->GetCustomFields(getter_AddRefs(customFields));

	// get the array
	//nsCOMPtr<nsIArray> array;
	//foo->GetElements(getter_AddRefs(array));
	 

	if (customFields != NULL)
	{
		PRUint32 customCount = 0;
		customFields->GetLength(&customCount);

		if (customCount > 0)
		{
			// make an enumerator
			nsCOMPtr<nsISimpleEnumerator> enumerator;
			customFields->Enumerate(getter_AddRefs(enumerator));

			// now enumerate the elements
			PRBool moreFields;
			enumerator->HasMoreElements(&moreFields);


			while (moreFields)
			{
				nsCOMPtr<kfILoginField> customField;
				enumerator->GetNext(getter_AddRefs(customField));

				nsString fieldValue;
				(void)customField->GetValue(fieldValue);
				nsString fieldName;
				(void)customField->GetName(fieldName);

				KPFormField *customFF = new KPFormField();
				customFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
				customFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
				customFF->type = formFieldType::FFTtext; //TODO: maybe want to record an accurate type here one day?

				entry.formFieldList.push_back(*customFF);

				enumerator->HasMoreElements(&moreFields);
			}
		}
	}


  try {
	if (!KeeICE.KP)
        throw "Proxy went away";

		KeeICE.KP->AddLogin(entry);

    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::AddLogin - finished" << "\n";
#endif

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::RemoveLogin(kfILoginInfo *aLogin)
{

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::ModifyLogin(kfILoginInfo *aOldLogin, kfILoginInfo *aNewLogin)
{
  NS_ENSURE_ARG_POINTER(aOldLogin);
  NS_ENSURE_ARG_POINTER(aNewLogin);
  
  return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetAllLogins(PRUint32 *count, kfILoginInfo ***logins)
{

#if _DEBUG
  std::cout << "comp-impl.cpp::GetAllLogins - started" << "\n";
#endif

  int status = 0;
  nsresult rv;

  nsCOMArray<kfILoginInfo> results;

  KeeICE::KFlib::KPEntryList entries;

  try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		*count = KeeICE.KP->getAllLogins(entries);

    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "we have received " << entries.size() << " KPEntry objects" << "\n";
#endif

	for (unsigned int i = 0; i < entries.size(); i++) {
		KPEntry entry = entries.at(i);

	#if _DEBUG
	  std::cout << "found a new KPEntry" << "\n";
	#endif

		nsString username, password, usernameField, passwordField;

		for (unsigned int j = 0; j < entry.formFieldList.size(); j++) 
		{
			KPFormField kpff = entry.formFieldList.at(j);
	#if _DEBUG
	  std::cout << "found a new field..." << "\n";
	#endif
	  if (kpff.type == formFieldType::FFTusername)
			{
				username = NS_ConvertUTF8toUTF16(kpff.value.c_str());
				usernameField = NS_ConvertUTF8toUTF16(kpff.name.c_str());
	#if _DEBUG
	  std::cout << "found a username" << "\n";
	#endif
			} else if (kpff.type == formFieldType::FFTpassword)
			{
				password = NS_ConvertUTF8toUTF16(kpff.value.c_str());
				passwordField = NS_ConvertUTF8toUTF16(kpff.name.c_str());
	#if _DEBUG
	  std::cout << "found a password" << "\n";
	#endif
			} else
			{
	#if _DEBUG
	  std::cout << "not a username or password - ignoring as per standard FF3 password system spec." << "\n";
	#endif
			}
		}

		nsString NShostname,NSactionURL,NSHTTPRealm;

		NShostname = NS_ConvertUTF8toUTF16(entry.hostName.c_str());
		NSactionURL = NS_ConvertUTF8toUTF16(entry.formURL.c_str());
		NSHTTPRealm = NS_ConvertUTF8toUTF16(entry.HTTPRealm.c_str());

		nsCOMPtr<kfILoginInfo> info = do_CreateInstance("@christomlinson.name/kfLoginInfo;1");
		if (!info) {
		  return NS_ERROR_OUT_OF_MEMORY;
		}
		rv = info->Init(NShostname, NSactionURL, NSHTTPRealm, username, password,
						usernameField, passwordField);

		if (NS_SUCCEEDED(rv))
		  (void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfILoginInfo object" << "\n";
	#endif
	}

  if (0 == results.Count()) {
    *logins = nsnull;
    *count = 0;
    return NS_OK;
  }
  
  kfILoginInfo **retval = (kfILoginInfo **)NS_Alloc(sizeof(kfILoginInfo *) * results.Count());
  for (PRInt32 i = 0; i < results.Count(); i++)
    NS_ADDREF(retval[i] = results[i]);
  *logins = retval;
  *count = results.Count(); //TODO: check matches with C# output?
  
#if _DEBUG
  std::cout << "count: " << *count << "\n";
#endif

#if _DEBUG
  std::cout << "comp-impl.cpp::GetAllLogins - finished" << "\n";
#endif

  return NS_OK;

}

NS_IMETHODIMP CKeeFox::FindLogins(PRUint32 *count, const nsAString &aHostname, const nsAString &aActionURL, const nsAString &aHttpRealm, kfILoginInfo ***logins)
{
 
#if _DEBUG
  std::cout << "comp-impl.cpp::FindLogins - started" << "\n";
#endif

  int status = 0;
  nsresult rv;

  nsCOMArray<kfILoginInfo> results;

  KeeICE::KFlib::KPEntryList entries;

  try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		// null (Void) strings and empty strings are different in Firefox but ICE can't support that so there are
		// various places like this where we need to make some decisions about what KeePass can return before
		// any message is actually sent to KeePass. A bit counter-intuitive but it works and reduces
		// inter-application traffic too as a nice side-effect.
		if (aActionURL.IsVoid() && aHttpRealm.IsVoid())
		{	
			*count = 0;
			#if _DEBUG
			  std::cout << "comp-impl.cpp::FindLogins - finished" << "\n";
			#endif
			return NS_OK;
		}

		string hostname = NS_ConvertUTF16toUTF8(aHostname).get();
		string actionURL = NS_ConvertUTF16toUTF8(aActionURL).get();
		string httpRealm = NS_ConvertUTF16toUTF8(aHttpRealm).get();

		if (aHttpRealm.IsVoid())
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTnoRealms),false, entries);
		else if (aActionURL.IsVoid())
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTnoForms),false, entries);
		else
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTall), false, entries);

    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "we have received " << entries.size() << " KPEntry objects" << "\n";
#endif

	for (unsigned int i = 0; i < entries.size(); i++) {
		KPEntry entry = entries.at(i);

	#if _DEBUG
	  std::cout << "found a new KPEntry" << "\n";
	#endif

		nsString username, password, usernameField, passwordField;

		nsCOMPtr<nsIMutableArray> customFields = do_CreateInstance(NS_ARRAY_CONTRACTID);
				
		for (unsigned int j = 0; j < entry.formFieldList.size(); j++) 
		{
			KPFormField kpff = entry.formFieldList.at(j);
	#if _DEBUG
	  std::cout << "found a new field..." << "\n";
	#endif
	  if (kpff.type == formFieldType::FFTusername)
			{
				username = NS_ConvertUTF8toUTF16(kpff.value.c_str());
				usernameField = NS_ConvertUTF8toUTF16(kpff.name.c_str());
	#if _DEBUG
	  std::cout << "found a username" << "\n";
	#endif
			} else if (kpff.type == formFieldType::FFTpassword)
			{
				password = NS_ConvertUTF8toUTF16(kpff.value.c_str());
				passwordField = NS_ConvertUTF8toUTF16(kpff.name.c_str());
	#if _DEBUG
	  std::cout << "found a password" << "\n";
	#endif
			} else if (kpff.type == formFieldType::FFTtext)
			{
				nsString customValue = NS_ConvertUTF8toUTF16(kpff.value.c_str());
				nsString customName = NS_ConvertUTF8toUTF16(kpff.name.c_str());

				
				nsCOMPtr<kfILoginField> customField = do_CreateInstance("@christomlinson.name/kfLoginField;1");
				if (!customField) {
				  return NS_ERROR_OUT_OF_MEMORY;
				}

				rv = customField->Init(customName, customValue);

				if (NS_SUCCEEDED(rv))
					customFields->AppendElement(customField,false);

				// get the array
				//nsCOMPtr<nsIArray> array;
				//foo->GetElements(getter_AddRefs(array));
				 /*

				if (customFields != NULL)
				{
					PRUint32 customCount = 0;
					customFields->GetLength(&customCount);

					if (customCount > 0)
					{
						// make an enumerator
						nsCOMPtr<nsISimpleEnumerator> enumerator;
						customFields->Enumerate(getter_AddRefs(enumerator));

						// now enumerate the elements
						PRBool moreFields;
						enumerator->HasMoreElements(&moreFields);


						while (moreFields)
						{
							nsCOMPtr<kfILoginField> customField;
							enumerator->GetNext(getter_AddRefs(customField));

							nsString fieldValue;
							(void)customField->GetValue(fieldValue);
							nsString fieldName;
							(void)customField->GetName(fieldName);

							KPFormField *customFF = new KPFormField();
							customFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
							customFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
							customFF->type = formFieldType::FFTtext; //TODO: maybe want to record an accurate type here one day?

							entry.formFieldList.push_back(*customFF);

							enumerator->HasMoreElements(&moreFields);
						}
					}
					
				}*/



	#if _DEBUG
	  std::cout << "found a text/custom field" << "\n";
	#endif
			} else
			{
	#if _DEBUG
	  std::cout << "not a username, password or text/custom field - unsupported." << "\n";
	#endif
			}
		}

		nsString NShostname,NSactionURL,NSHTTPRealm;

		NShostname = NS_ConvertUTF8toUTF16(entry.hostName.c_str());
		NSactionURL = NS_ConvertUTF8toUTF16(entry.formURL.c_str());
		NSHTTPRealm = NS_ConvertUTF8toUTF16(entry.HTTPRealm.c_str());

		nsCOMPtr<kfILoginInfo> info = do_CreateInstance("@christomlinson.name/kfLoginInfo;1");
		if (!info) {
		  return NS_ERROR_OUT_OF_MEMORY;
		}

		PRUint32 customLength = 0;

		customFields->GetLength(&customLength);

		if (customLength > 0)
			rv = info->InitCustom(NShostname, NSactionURL, NSHTTPRealm, username, password,
						usernameField, passwordField, customFields);
		else
			rv = info->Init(NShostname, NSactionURL, NSHTTPRealm, username, password,
						usernameField, passwordField);

		if (NS_SUCCEEDED(rv))
		  (void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfILoginInfo object" << "\n";
	#endif
	}

  if (0 == results.Count()) {
    *logins = nsnull;
    *count = 0;
    return NS_OK;
  }
  
  // does this work with mutable array? mem overflow?
  kfILoginInfo **retval = (kfILoginInfo **)NS_Alloc(sizeof(kfILoginInfo *) * results.Count());
  for (PRInt32 i = 0; i < results.Count(); i++)
    NS_ADDREF(retval[i] = results[i]);
  *logins = retval;
  *count = results.Count(); //TODO: check matches with C# output?
  
#if _DEBUG
  std::cout << "count: " << *count << "\n";
#endif

#if _DEBUG
  std::cout << "comp-impl.cpp::FindLogins - finished" << "\n";
#endif

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::CountLogins(nsAString const &aHostname, nsAString const &aActionURL,
	nsAString const &aHttpRealm, unsigned int *_retval)
{
#if _DEBUG
  std::cout << "comp-impl.cpp::CountLogins - started" << "\n";
#endif

	int status = 0;
	int count = 0;

    try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		// null (Void) strings and empty strings are different in Firefox but ICE can't support that concept
		// so there are various places like this where we need to make some decisions about what KeePass
		// can return before any message is actually sent to KeePass. A bit counter-intuitive but
		// it works and reduces inter-application traffic too as a nice side-effect.
		if (aActionURL.IsVoid() && aHttpRealm.IsVoid())
		{	
			*_retval = 0;
			#if _DEBUG
			  std::cout << "comp-impl.cpp::CountLogins - finished" << "\n";
			#endif
			return NS_OK;
		}

		string hostname = NS_ConvertUTF16toUTF8(aHostname).get();
		string actionURL = NS_ConvertUTF16toUTF8(aActionURL).get();
		string httpRealm = NS_ConvertUTF16toUTF8(aHttpRealm).get();

		if (aHttpRealm.IsVoid())
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTnoRealms), false);
		else if (aActionURL.IsVoid())
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTnoForms), false);
		else
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KFlib::loginSearchType(LSTall), false);

    } catch (KeeICE::KFlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::CountLogins - finished" << "\n";
#endif
//*_retval = 1;
return NS_OK;
}




















































///////////////////////////////////////////////////////////////////////////////
//// nsILoginManagerStorage

/*
bool CKeeFox::IsLocationInCache(string url)
{
return true;

// TODO: read from memory (linked list / tree?) for speed
// temp fix - read from disk?

}

int CKeeFox::CountLocationsInCache(string url)
{
return 1;

// TODO: read from memory (linked list / tree?) for speed
// temp fix - read from disk?

}

void CKeeFox::DeleteLocationFromCache(string url)
{
return;

// TODO: write to memory (linked list / tree?) for speed
// TODO: write to disk

//TODO: Need to think what happens when more than one entry for a URL/realm -
// do we need a deleteALL? or some way to identify individual entries?

}

void CKeeFox::WriteLocationToCache(string url)
{
return;

// TODO: write to memory (linked list / tree?)

/* TODO: Make this file writing example work
nsCOMPtr<nsILocalFile> lf =
      do_GetService("@mozilla.org/file/local;1");
  //NS_ENSURE_STATE(cat);

	lf->initWithPath("c:\\temp\\foo.dat"); //TODO: use XP standard location (e.g. extension folder)

  if (!lf.exists())
    lf->create(lf.NORMAL_FILE_TYPE, 0644);

  nsCOMPtr<nsIIOService> ioService = do_GetService("@mozilla.org/network/io-service;1");

  string uri = ioService.newFileURI(lf);
  string channel = ioService.newChannelFromURI(uri);

  nsCOMPtr<nsIFileOutputStream> outputStream = do_GetService("@mozilla.org/network/file-output-stream;1");

  outputStream.init(lf, 0x20|0x02, 00004, null);
  string buffer = "This is a test";

  outputStream.write(buffer, buffer.length);
  outputStream.flush();
  outputStream.close();
*/

//}

/*

NS_IMETHODIMP CKeeFox::Init()
{
  return NS_OK;
}

NS_IMETHODIMP CKeeFox::InitWithFile(nsIFile *aInputFile, nsIFile *aOutputFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP CKeeFox::RemoveAllLogins()
{
	//TODO: not sure how we can do this. FF3 spec says this should be called without
	// having to enter master password. That is impossible but do we inform
	// the firefox user or silently ignore?
	// do we allow this if the master password is entered? or is this just too
	// drastic on the keepass database and too open to confusion about what should be
	// deleted and what left alone. lots of room for horrible mistakes unless 
	// explained to user very clearly!

	// maybe just clear the firefox URL cache, keepass database name, etc.?
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP CKeeFox::GetAllDisabledHosts(PRUint32 *count, PRUnichar ***hostnames)
{
  // TODO implement me
  *count = 0;
  *hostnames = nsnull;

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetLoginSavingEnabled(const nsAString &aHost, PRBool *_retval)
{
  // TODO implement me
  *_retval = PR_TRUE;

  return NS_OK;;
}

NS_IMETHODIMP CKeeFox::SetLoginSavingEnabled(const nsAString &aHost, PRBool aEnabled)
{
  // TODO implement me

  return NS_OK;
}




*/



//TODO?: comp_impl converts unmanaged object to nsILoginInfo
	// unmanaged object can be represented by its own nsIDL - with close to a 1-1 mapping of fields, methods, etc.
	// 2nd stage javascript will understand the objects returned in that custom IDL format and be able to
	// extract the info needed to fill in forms, etc.
	// nsILoginInfo conversion may be more complex but there will be less fields overall that need to be marshalled so not sure which will end up fastest
