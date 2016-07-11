#include "tocci/tocci.h"
using namespace oracle::occi;

#define OASSERT(x) if ((x)!=OCI_SUCCESS && (x)!=OCI_SUCCESS_WITH_INFO) throw SQLException(errhp);

void Connection::commit()
{
	int r=OCITransCommit(svchp, errhp,  OCI_DEFAULT);
	OASSERT(r);
}

void Connection::rollback()
{
	int r=OCITransRollback(svchp, errhp,  OCI_DEFAULT);
	OASSERT(r);
}

Statement * Connection::createStatement(const char *query)
{
	return new Statement(this, query);
}
void Connection::terminateStatement(Statement *st)
{
	delete st;
}

Connection::~Connection()
{
	OCILogoff(svchp, errhp);	
}

Connection::Connection(Environment *env, const char *login, const char *pass, const char *cstr): 
		environment(env), envhp(env->getOCIEnviroment()),errhp(env->getOCIError())
{

		int r=OCILogon(envhp, errhp, &svchp, (unsigned char*)login, strlen(login), 
			(unsigned char*)pass, strlen(pass), 
			(unsigned char*)cstr, strlen(cstr));
		if (r!=OCI_SUCCESS && r!=OCI_SUCCESS_WITH_INFO) 
			throw SQLException(errhp);
}
