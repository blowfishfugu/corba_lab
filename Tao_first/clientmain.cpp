#define ACE_OS_MAIN_H

#include <iostream>
#include "clientmain.h"
#include "CompanyC.h"

#include <iostream>
#include <format>
#include <chrono>
#include <thread>
#include <array>

#include <tao/corba.h>
#include <orbsvcs/orbsvcs/CosNamingC.h>

using namespace std::chrono_literals;

int main(int argc, char** argv)
{
	std::array<const char*, 2> pArgs{
		argv[0],
		"-ORBInitRef NameService=corbaloc:iiop:localhost:2809/NameService",
	};
	int argSize = static_cast<int>(pArgs.size());
	CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);
	
	ACE_Time_Value timeout(40ms); //lokales LAN, ping auf 1ms, 40ms warten dürfte reichen
	CORBA::Object_var ns_obj = orb->resolve_initial_references("NameService", &timeout);
	if (CORBA::is_nil(ns_obj))
	{
		orb->destroy();
		return 1;
	}
	CosNaming::NamingContext_var ns_context = CosNaming::NamingContext::_narrow(ns_obj);

	CosNaming::Name Name(1);
	Name.length(1);
	Name[0].id = CORBA::string_dup("MyFactory");
	CORBA::Object_ptr srvFactoryObj= ns_context->resolve(Name);
	CompanyModule::ImpFactory_var srvFactory = CompanyModule::ImpFactory::_narrow(srvFactoryObj);
	CompanyModule::Person_var person=srvFactory->CreatePerson();
	person->firstname("tester1");

	orb->destroy(); //TODO: orb_raii, std::unique_ptr mit destructor, der destroy aufruft?

  return 0;
}