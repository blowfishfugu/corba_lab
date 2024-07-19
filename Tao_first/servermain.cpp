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
#include <array>

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
class MyPerson : public virtual POA_CompanyModule::Person
{
public:
	std::string _firstname{};
	std::string _name{};
	std::chrono::year_month_day _birthdate{ 1990y/3/12d };
	
	virtual ~MyPerson() {
		std::cout << "deleted MyPerson\n";
	}

	// Inherited via Person
	virtual char* firstname() override
	{
		return _firstname.data();
	}
	virtual void firstname(const char* firstname) override
	{
		_firstname =std::string(firstname);
		std::cout << std::format("firstname set to: {}\n", _firstname);
	}
	virtual char* name() override
	{
		return _name.data();
	}
	virtual void name(const char* name) override
	{
		_name = std::string(name);
	}
	virtual ::CompanyModule::YearMonthDay birthday() override
	{
		return ::CompanyModule::YearMonthDay{
			.year = static_cast<CORBA::Long>( _birthdate.year() ),
			.month = static_cast<CORBA::UShort>( (unsigned int)_birthdate.month() ),
			.day = static_cast<CORBA::UShort>( (unsigned int)_birthdate.day() )
		};
	}
	virtual void birthday(const::CompanyModule::YearMonthDay& birthday) override
	{
		_birthdate = { std::chrono::year{birthday.year} / birthday.month / birthday.day };
	}
	virtual char* FullName() override
	{
		thread_local std::string fullname( std::format("{} {}", _firstname.c_str(), _name.c_str()));
		return fullname.data();
	}
};

class MyEmployee : public virtual POA_CompanyModule::Employee, public virtual MyPerson
{
public:
	virtual ~MyEmployee() {
		std::cout << "deleted MyEmpoyee\n";
	}

	double _salary{};
	// Inherited via Employee
	virtual ::CORBA::Double salary() override
	{
		return _salary;
	}
	virtual void salary(::CORBA::Double salary) override
	{
		_salary = salary;
	}

	virtual void payout() override
	{
		_salary = {};
	}
};

class MyFactory : public virtual POA_CompanyModule::ImpFactory
{
	// Inherited via ImpFactory
	virtual ::CompanyModule::Person_ptr CreatePerson() override
	{
		MyPerson* person = new MyPerson(); //TODO: <- das kann nicht richtig sein
		
		return CompanyModule::Person::_duplicate(person->_this());
	}
	virtual ::CompanyModule::Employee_ptr CreateEmployee() override
	{
		MyEmployee* emp = new MyEmployee(); //TODO: <- das kann nicht richtig sein
		return CompanyModule::Employee::_duplicate(emp->_this());
	}
};

int main(int argc, char** argv)
{
	std::vector<std::string> args = StoreArgs(argc, argv);
	//orb //orbinit
	//poa
	//poa-activate
	//orbdestroy

	std::array<const char*, 2> pArgs{
		argv[0],
		"-ORBInitRef NameService=corbaloc:iiop:localhost:2809/NameService",
	};
	int argSize = static_cast<int>( pArgs.size() );
	CORBA::ORB_var orb = CORBA::ORB_init(argSize, const_cast<char**>(pArgs.data())); //<-achtung, argv hiernach weg?
	
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

	//TODO: try-catch und in eigene funktion auslagern, TryRegister(const char* Name, pObj)
	try 
	{
		//doku: file:///$TAO_ROOT/docs/tutorials/Quoter/Naming_Service/index.html
		// spec: https://www.omg.org/cgi-bin/doc?ptc/00-08-07   -> Nameservice defaultport: 2809
		//"wir" sollten starten mit:
		// servermain.exe -ORBInitRef NameService=corbaloc:iiop:localhost:2809/NameService
		//und der namingservice z.B. mit:
		// $TAO_ROOT/orbsvcs/Naming_Service/tao_cosnaming -m 1 -ORBEndPoint iiop://localhost:2809
		ACE_Time_Value timeout(40ms); //lokales LAN, ping auf 1ms, 40ms warten dürfte reichen
		CORBA::Object_var ns_obj = orb->resolve_initial_references("NameService", &timeout);
		if (CORBA::is_nil(ns_obj))
		{
			poa->destroy(true, true);
			orb->destroy();
			return 1;
		}
		CosNaming::NamingContext_var ns_context = CosNaming::NamingContext::_narrow(ns_obj);
		//ns_context->list(1024, ) //TODO: list_names(ns_context.in()) ala tao/utils/nslist/nslist.cpp
		
		
		//TODO: toName(const char*)->CosNaming::Name
		CosNaming::Name Name(1);
		Name.length(1);
		Name[0].id = CORBA::string_dup("MyFactory");
		ns_context->rebind(Name, pObj);
		
		//TODO: separate Funktion try_resolve(ns_context, const char* name)
		CORBA::Object_ptr tstResolve = ns_context->resolve(Name);
		CompanyModule::ImpFactory_var iFactory=CompanyModule::ImpFactory::_narrow(tstResolve);
		tstResolve = nullptr;

		CompanyModule::Person_var person=iFactory->CreatePerson();
		
		//ns_context->unbind(Name); //<- kann das jeder client?
	}
	catch (CORBA::ORB::InvalidName exc) //TODO: welchen Typ wirft der timeout? InvalidName ist abgehandelt :)
	{
		std::cout << "failed to resolve NameService\n" << std::string(exc._info().c_str()) << "\n";
	}

	std::jthread runFor([&orb]() {
		const int maxwait = 50000;
		for (int i = 0; i < maxwait; ++i)
		{
			std::cout << std::format("shutdown in {} ..\n", maxwait - i);
			std::this_thread::sleep_for(1s);
		}
		orb->shutdown(true);
		});

	//dies hier, wie der windows-messageloop, blocking listener auf events
	orb->run(); //<- ab hier
	
	poa->destroy(true,true);
	orb->destroy();

	return 0;
}