#define ACE_OS_MAIN_H

#include "CompanyC.h" //generiert von tao_idl
#include "CompanyS.h" //generiert von tao_idl

#include "servermain.h"

//includes: ACE_ROOT TAO_ROOT
//libs: ACE TAO TAO_AnyTypeCode TAO_PortableServer TAO_CosNaming
#include <tao/corba.h>
#include <tao/PortableServer/PortableServer.h>
#include <orbsvcs/orbsvcs/CosNamingC.h>
// #include <ace/streams.h> //nicht verwendet, nur als beispiel

#include <vector>
#include <string_view>
#include <span>
#include <ranges>
#include <algorithm>
#include <iostream>
#include <chrono>
using namespace std::chrono_literals;

static auto StoreArgs = [](int argc, char** argv) {
	std::vector<std::string> args;
	args.reserve(argc);
	auto args_span =
		std::span(argv, argc) | std::views::transform([](char const* val) { return std::string_view(val); });
	std::transform(args_span.begin(), args_span.end(), std::back_inserter(args), [](const std::string_view& v) {
		return std::string(v.data(), v.size());
		});
	return args;
};

class MyFactory : public virtual POA_CompanyModule::ImpFactory
{
	// Inherited via ImpFactory
	virtual ::CompanyModule::Person_ptr CreatePerson() override
	{
		return ::CompanyModule::Person_ptr();
	}
	virtual ::CompanyModule::Employee_ptr CreateEmployee() override
	{
		return ::CompanyModule::Employee_ptr();
	}
};

int main(int argc, char** argv)
{
	std::vector<std::string> args = StoreArgs(argc, argv);
	//orb //orbinit
	//poa
	//poa-activate
	//orbdestroy
	CORBA::ORB_var orb = CORBA::ORB_init(argc, argv); //<-achtung, argv hiernach weg!
	
	CORBA::Object_var poa_object = orb->resolve_initial_references("RootPOA"); //<- hier der hit auf firewall-abfrage
	PortableServer::POA_var poa = PortableServer::POA::_narrow(poa_object /*.in() */);
	
	PortableServer::POAManager_var poa_manager = poa->the_POAManager();
	poa_manager->activate();

	/// TODO: Implementierung der Factory
	/// + anhängen an den manager
	//std::cout << ior << "\n";

	MyFactory mainFactory;
	PortableServer::Servant_var<MyFactory> activatable_servant = &mainFactory; //cast to Servant
	PortableServer::ObjectId_var oid=poa->activate_object(activatable_servant.in());
	
	//Ding bekannt? 
	CORBA::Object_ptr pObj = poa->id_to_reference(oid.in());
	std::string iorStr( orb->object_to_string(pObj) );
	std::cout << iorStr << "\n";

	//TODO: try-catch und in eigene funktion auslagern
	try 
	{
		//doku: file:///$TAO_ROOT/docs/tutorials/Quoter/Naming_Service/index.html
		//"wir" sollten starten mit:
		// servermain.exe -ORBInitRef NameService=corbaloc:iiop:localhost:12345/NameService
		//und der namingservice z.B. mit:
		// $TAO_ROOT/orbsvcs/Naming_Service/tao_cosnaming -ORBEndPoint iiop://localhost:12345
		ACE_Time_Value timeout(40ms); //lokales LAN
		CORBA::Object_var ns_obj = orb->resolve_initial_references("NameService", &timeout);
		if (CORBA::is_nil(ns_obj))
		{
			poa->destroy(true, true);
			orb->destroy();
			return 1;
		}
		CosNaming::NamingContext_var ns_context = CosNaming::NamingContext::_narrow(ns_obj);
	}
	catch (CORBA::ORB::InvalidName exc) //TODO: welchen Typ wirft der timeout?
	{
		std::cout << "failed to resolve NameService\n" << std::string(exc._info().c_str()) << "\n";
	}
	//dies hier, wie der windows-messageloop, blocking listener auf events
	orb->run(); //<- ab hier
	
	poa->destroy(true,true);
	orb->destroy();

	return 0;
}