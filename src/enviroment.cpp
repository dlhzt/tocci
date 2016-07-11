#include "tocci/tocci.h"
using namespace oracle::occi;

//#include <oci.h>
#include <ociapr.h>
#include <ocidfn.h>
#include <oratypes.h>
#include <ocidem.h>

Environment *Environment::createEnvironment(Mode mode)
{ 
	return new Environment(mode);
}

Connection *Environment::createConnection(const char *login, const char *pass, const char *str)
{
	return new Connection(this, login, pass, str);
}

void Environment::terminateConnection(Connection *c)
{
	delete c;
}

void Environment::terminateEnvironment(Environment *e)
{
	delete e;
}

Map *Environment::getMap() { return &map;}

Environment::Environment(Mode m): mode(m)
{
	if(OCIEnvCreate(&envhp, m, NULL, NULL, NULL, NULL, 0, NULL)!=OCI_SUCCESS) 
	{
		throw SQLException(1, "Cannot alocate Enviroment handler");
	}
	if(OCIHandleAlloc(envhp, (void **)&errhp, (ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0))
	{
		throw SQLException(2, "Cannot create error handler");
	}
}

Environment::~Environment()
{
	OCIHandleFree(errhp, (ub4) OCI_HTYPE_ERROR);
	OCIHandleFree(envhp, (ub4) OCI_HTYPE_ENV);
}

