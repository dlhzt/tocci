#include "tocci/tocci.h"
#include <stdio.h>

using namespace oracle::occi;

#define OASSERT(x) if ((x)!=OCI_SUCCESS && (x)!=OCI_SUCCESS_WITH_INFO) throw SQLException(errhp);

Statement::Statement(Connection *con,const char *q)
	: envhp(con->getEnvironment()->getOCIEnviroment()),
	errhp(con->getEnvironment()->getOCIError()),
	svchp(con->getOCIServiceContext()),
	query(q),autoCommit(false) {

	int r=0;
	r=OCIHandleAlloc(envhp, (void **)&stmt, OCI_HTYPE_STMT, (CONST size_t) 0, (dvoid **) 0);

	if (r!=OCI_SUCCESS && r!=OCI_SUCCESS_WITH_INFO) throw SQLException(errhp);
	r=OCIStmtPrepare (stmt, errhp,
		(unsigned char *)query.c_str(), query.length(), OCI_NTV_SYNTAX, OCI_DEFAULT);
	if (r!=OCI_SUCCESS && r!=OCI_SUCCESS_WITH_INFO) {
		throw SQLException(errhp);
	}
}

Statement::~Statement() {
	OCIHandleFree((void **)stmt, OCI_HTYPE_STMT);
}


void Statement::setPrefetchRowCount(int pfc) {
  if (OCIAttrSet (stmt,
                  OCI_HTYPE_STMT,
                  &pfc, 
                  0,
                  OCI_ATTR_PREFETCH_ROWS,
                  errhp)) throw SQLException(errhp);
}

void Statement::registerOutParam(int pos, int type) {
	int r;
	if (pos>=MAX_BINDS) throw SQLException(3, "Cannot bind such index (too big)" );
	var[pos].type=type;
	switch(type) {
	case OCCIUNSIGNED_INT:
		var[pos].data_len=sizeof(unsigned int);
		var[pos].data=realloc(var[pos].data, var[pos].data_len);
		break;
	case OCCICURSOR:
		var[pos].data_len=0;
		var[pos].data=realloc(var[pos].data, sizeof(OCIStmt *));
		*((OCIStmt **)var[pos].data)=NULL;
		r=OCIHandleAlloc(envhp, (void **)var[pos].data, OCI_HTYPE_STMT, (CONST size_t) 0, (dvoid **) 0);
		OASSERT(r);
		break;

	default: throw SQLException(4, "Unknown type. TOCCI doesnt support this type of OUT parameter");
	}
}

int Statement::executeUpdate() {  
	int i, r;
	char bname[8];
	ub2 type;

	int iter;//must be zero only for select ! OCI_ATTR_STMT_TYPE OCI_STMT_SELECT
	r=OCIAttrGet(stmt, OCI_HTYPE_STMT, &type, 0, OCI_ATTR_STMT_TYPE, errhp);
	OASSERT(r);
	iter=(type==OCI_STMT_SELECT)?0:1;

	for(i=1; i<MAX_BINDS; i++) if (var[i].data_len!=-1) {
		sprintf(bname, ":v%d", i);
		r=OCIBindByName(stmt, &var[i].bindhp, errhp, (unsigned char *)bname, strlen(bname), 
			var[i].data, var[i].data_len, var[i].type, 
			NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
		OASSERT(r);

		if (var[i].type==SQLT_NTY || var[i].type==SQLT_NCO) {
			r=OCIBindObject(var[i].bindhp, errhp, var[i].oci_type, 
				(void**)var[i].data, NULL, NULL, NULL);
			OASSERT(r);
		}
	}

	r=OCIStmtExecute(svchp, stmt, errhp, iter, 0, NULL, NULL, (autoCommit)?OCI_COMMIT_ON_SUCCESS:OCI_DEFAULT);
	OASSERT(r);

	return 0;
}


ResultSet *Statement::executeQuery() {
	int i, r;
	char bname[8];
	ub2 type;

	int iter;//must be zero only for select ! OCI_ATTR_STMT_TYPE OCI_STMT_SELECT
	r=OCIAttrGet(stmt, OCI_HTYPE_STMT, &type, 0, OCI_ATTR_STMT_TYPE, errhp);
	OASSERT(r);
	iter=(type==OCI_STMT_SELECT)?0:1;

	for(i=1; i<MAX_BINDS; i++) if (var[i].data_len!=-1) {
//		printf("Bind var: %d\n", i);
		sprintf(bname, ":v%d", i);
		r=OCIBindByName(stmt, &var[i].bindhp, errhp, (unsigned char *)bname, strlen(bname), 
			var[i].data, var[i].data_len, var[i].type, 
			NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
		OASSERT(r);

		if (var[i].type==SQLT_NTY || var[i].type==SQLT_NCO) {
			//indpp=NULL;
			r=OCIBindObject(var[i].bindhp, errhp, var[i].oci_type, 
				(void**)var[i].data, NULL, NULL, NULL);
			OASSERT(r);
		}
	}

	r=OCIStmtExecute(svchp, stmt, errhp, iter, 0, NULL, NULL, (autoCommit)?OCI_COMMIT_ON_SUCCESS:OCI_DEFAULT);
	OASSERT(r);
	return (type==OCI_STMT_SELECT)?new ResultSet(stmt, errhp):new ResultSet();
}

unsigned int Statement::getUInt(int pos) {
	if (var[pos].data_len==-1) throw SQLException(10, "You havent bind that index");
	return *((unsigned int*)var[pos].data);
}

void Statement::setUInt(int pos, unsigned int val) {
	var[pos].type=OCCIUNSIGNED_INT;
	var[pos].data=realloc(var[pos].data, sizeof(val));
	memcpy(var[pos].data, &val, sizeof(val));
	var[pos].data_len=sizeof(val);
}

void Statement::setString(int pos, const string &str) {
	var[pos].type=SQLT_STR;
	//var[pos].data=(void*)strdup(str.c_str());
	var[pos].data=realloc(var[pos].data, str.length()+1);
	strcpy((char*)var[pos].data, str.c_str());
	var[pos].data_len=str.length()+1;
}

ResultSet * Statement::getCursor(int pos) {
	OCIStmt *st=*((OCIStmt **)var[pos].data);
	return new ResultSet(st, errhp);
}

bool Statement::getAutoCommit() const {
	return autoCommit;
}

void Statement::setAutoCommit(bool ac) {
	autoCommit=ac;
}


void Statement::closeResultSet(ResultSet *rs) {delete rs;}


void Statement::setVector(int pos, const vector<PObject*> &v, const char *type) {
	int r;

	OCIType *t;
	OCIType *coll_type;
	OCIColl **coll;
	void *obj_inst;

	var[pos].type=SQLT_NTY;
	var[pos].data_len=sizeof(OCIColl*);
	var[pos].data=realloc(var[pos].data, sizeof(OCIColl*));
	coll=(OCIColl**)var[pos].data;

	r=OCITypeByName(envhp, errhp, svchp, 
			NULL, (unsigned)-1, (unsigned char*)type, strlen(type), 
			NULL, (unsigned)-1, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &coll_type);
	OASSERT(r);
	r=OCIObjectNew(envhp, errhp, svchp, OCI_TYPECODE_NAMEDCOLLECTION, coll_type, 
		NULL, OCI_DURATION_SESSION, TRUE, (dvoid**)coll);
	OASSERT(r);
	var[pos].oci_type=coll_type;

	for (vector<PObject*>::const_iterator i=v.begin(); i!=v.end(); i++) {
		PObject * obj=(*i);
		r=OCITypeByName(envhp, errhp, svchp, 
			NULL, (unsigned)-1, (unsigned char*)obj->getSQLTypeName().c_str(), obj->getSQLTypeName().length(), 
			NULL, (unsigned)-1, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &t);
		OASSERT(r);
		r=OCIObjectNew(envhp, errhp, svchp, SQLT_NTY, t, NULL, OCI_DURATION_SESSION, TRUE, (dvoid**)&obj_inst);
		OASSERT(r);
		/*
		OBJECT ATTR SET !
		*/

		vector<AnyDataElement> obj_meta;
		AnyData obj_data(&obj_meta);
		obj->writeSQL(obj_data);
		for(vector<AnyDataElement>::iterator ae=obj_meta.begin(); ae!=obj_meta.end(); ae++) {
			AnyDataElement e=(*ae);

			text *names[1];
			ub4 lengths[1];
			
			names[0]=(text*)e.name.c_str();
			lengths[0]=e.name.length();
			OCINumber val;
			r=OCINumberFromInt(errhp, &e.value, sizeof(e.value), OCI_NUMBER_SIGNED, &val);
			OASSERT(r);

			//#ifdef WIN32
			//r=OCIObjectSetAttr(envhp, errhp, obj_inst, NULL, t, (ortext**)names, lengths, 1, 0, 0, 0, NULL, &val);
			//#else
			r=OCIObjectSetAttr(envhp, errhp, obj_inst, NULL, t, (const oratext**)names, lengths, 1, 0, 0, 0, NULL, &val);
			//#endif
			OASSERT(r);
		}

		r=OCICollAppend(envhp, errhp, obj_inst, NULL, *coll);
		OASSERT(r);
	}

//	memcpy(var[pos].data, &coll, sizeof(coll));
}
/*	string sum(type);
	string sql;
	AnyData  data(&sql);
	var[pos].data=strdup("\"TAB_KEYWORDS\"()");
	var[pos].type=SQLT_STR;
	var[pos].data_len=0;

	sum.append("(");

	for (vector<PObject*>::const_iterator i=v.begin(); i!=v.end(); i++) {
		sql="KEYWORD(";
		PObject * obj=(*i);
		obj->writeSQL(data);
		sql.resize(sql.length()-2);
		sql.append(")");

		sum.append(sql);
		sum.append("  ");
	}
	sum.resize(sum.length()-2);
	sum.append(" )");
	//printf("%s", sum.c_str());
	var[pos].data=strdup(sum.c_str());
	var[pos].data_len=sum.length()+1;
	var[pos].type=SQLT_STR;
*/
/*		
		text *names[4];
		ub4 lengths[4];

		names[0]=(text*)"WORD_ID";
		lengths[0]=strlen((char*)names[0]);
		names[1]=(text*)"CASE_MASK";
		lengths[1]=strlen((char*)names[1]);
		names[2]=(text*)"TYPE";
		lengths[2]=strlen((char*)names[2]);
		names[3]=(text*)"LENGTH";
		lengths[3]=strlen((char*)names[3]);
		text * val=(text*)"1";

		r=OCIObjectSetAttr(envhp, errhp, ref, NULL, t, (text**)names, lengths, 
			4, 0, 0,	NULL, NULL, val);
		OASSERT(r);
*/
/*
		keyword_t k;
		int c=10;
		r=OCINumberFromInt(errhp, &c, sizeof(c), OCI_NUMBER_SIGNED, &k.a);
		OASSERT(r);
		c=1;
		r=OCINumberFromInt(errhp, &c, sizeof(c), OCI_NUMBER_SIGNED, &k.b);
		OASSERT(r);
		c=0;
		r=OCINumberFromInt(errhp, &c, sizeof(c), OCI_NUMBER_SIGNED, &k.c);
		OASSERT(r);
		r=OCINumberFromInt(errhp, &c, sizeof(c), OCI_NUMBER_SIGNED, &k.d);
		OASSERT(r);
*/
