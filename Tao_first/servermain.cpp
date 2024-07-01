#define ACE_OS_MAIN_H

#include "CompanyC.h" //generiert von tao_idl
#include "CompanyS.h" //generiert von tao_idl

#include "servermain.h"

//includes: ACE_ROOT TAO_ROOT
//libs: ACE TAO TAO_AnyTypeCode TAO_PortableServer TAO_CosNaming
#include <tao/corba.h>
#include <tao/PortableServer/PortableServer.h>
// #include <ace/streams.h> //nicht verwendet, nur als beispiel

#include <vector>
#include <string_view>
#include <span>
#include <ranges>
#include <algorithm>
#include <iostream>

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

	/// TODO: Implementierung der Factory
	/// + anhängen an den manager
	/// 
	
	//std::cout << ior.in() << "\n";

	poa_manager->activate();

	//dies hier, wie der windows-messageloop, blocking listener auf events
	orb->run(); //<- ab hier
	
	poa->destroy(true,true);
	orb->destroy();

	return 0;
}