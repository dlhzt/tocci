#include "tocci/tocci.h"
using namespace oracle::occi;

#define OASSERT(x) if ((x)!=OCI_SUCCESS && (x)!=OCI_SUCCESS_WITH_INFO) throw SQLException(errhp);

ResultSet::ResultSet():stmt(NULL),errhp(NULL) {}
ResultSet::ResultSet(OCIStmt *st, OCIError *err):
	stmt(st),
	errhp(err) {

	OCIParam *par;
	int numcols, r, i;
	text *aname;
	ub4 alen;

	r=OCIAttrGet(stmt, OCI_HTYPE_STMT, &numcols, 0, OCI_ATTR_PARAM_COUNT, errhp);
	OASSERT(r);
	/* WE NEED TO DefineByPos or something here to recieve data */
	for (i=1; i<=numcols; i++) {
			r=OCIParamGet(stmt, OCI_HTYPE_STMT, errhp, (void **)&par, i);
			alen=0;
			r=OCIAttrGet((dvoid*) par, (ub4) OCI_DTYPE_PARAM, 
					 (dvoid*) &aname, &alen, (ub4) OCI_ATTR_NAME, 
					(OCIError *) errhp );
			OASSERT(r);
			metadata.push_back(MetaData(string((char*)aname, alen)));
			OCIDescriptorFree((void*) par, OCI_DTYPE_PARAM);
	}
}

int ResultSet::next() { 
	if (stmt==NULL) return END_OF_FETCH;
	int r, numcols, datalen;
	OCIParam *par;
	OCIDefine *def;
	int i, errcode;
	short dtype;
	
	char *dptr;

	r=OCIAttrGet(stmt, OCI_HTYPE_STMT, &numcols, 0, OCI_ATTR_PARAM_COUNT, errhp);
	OASSERT(r);
	/* WE NEED TO DefineByPos or something here to recieve data */
	for (i=1; i<=numcols; i++) {
		par=NULL;
		r=OCIParamGet(stmt, OCI_HTYPE_STMT, errhp, (void **)&par, i);
		OASSERT(r);
		datalen=0;
		r=OCIAttrGet((dvoid*) par, (ub4) OCI_DTYPE_PARAM, 
                 (dvoid*) &dtype,(ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE, 
                (OCIError *) errhp );
		OASSERT(r);
		r=OCIAttrGet((dvoid*) par, (ub4) OCI_DTYPE_PARAM, (void*) &datalen, NULL, (ub4) OCI_ATTR_DATA_SIZE, 
        (OCIError *) errhp  );
		OASSERT(r);

		if (dtype!=SQLT_STR) {
			dtype=SQLT_STR;
			datalen=512;
		}
		

		if (data[i]==NULL) {data[i]=new Cell; data[i]->ptr=NULL; }
		dptr=(char *)(data[i]->ptr=realloc(data[i]->ptr, datalen+1));
		memset(data[i]->ptr, 0, datalen+1);
		data[i]->len=datalen;
		data[i]->type=dtype;
		if (data[i]->ptr==NULL) throw SQLException(6, "Not enough memory");
		def=NULL;
		r=OCIDefineByPos(stmt, &def, errhp, i, data[i]->ptr, datalen, dtype, NULL, NULL, NULL, OCI_DEFAULT);
		OASSERT(r);

		OCIDescriptorFree((void*) par, OCI_DTYPE_PARAM);
	}

	r=OCIStmtFetch(stmt, errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
	if (r==OCI_ERROR) {
		OCIErrorGet(errhp, 1, (text *) NULL, &errcode,
                       (ub1*)NULL, (unsigned)-1,  OCI_HTYPE_ERROR);
		if (errcode==1405) r=OCI_SUCCESS;
	}
	if (r==OCI_NO_DATA) return END_OF_FETCH;
	OASSERT(r);
	return DATA_AVAILABLE;
}

unsigned int ResultSet::getUInt(int pos) {
	unsigned int res=(unsigned int)getInt(pos); //i know i know ;)
	return res;
}

int ResultSet::getInt(int pos) {
	if (data[pos]==NULL) throw SQLException(7, "no such column");
	
	int len=data[pos]->len;
	int type=data[pos]->type;
	char *ptr=(char *)data[pos]->ptr;
	int res=0, r;

	switch(type) {
	case SQLT_STR:
		res=atoi(ptr);
		return res;

	case SQLT_NUM: 
		r = OCINumberToInt (errhp, (struct OCINumber*)ptr, sizeof(res), OCI_NUMBER_SIGNED, &res);
		OASSERT(r);
		return res;
		break;

	default: throw SQLException(1, "Cannot cast sql type to int(unknown type.)");
	}
	return 0;
}

const string ResultSet::getString(int pos){
	if (data[pos]==NULL) throw SQLException(7, "no such column");
	char *dptr=(char*)data[pos]->ptr;
	int e=strlen(dptr)-1;
	while(dptr[e]==' ') e--; //trim
	dptr[e+1]=0;
	return dptr;
}

const vector<MetaData>& ResultSet::getColumnListMetaData() const {
	return metadata;
}

ResultSet::~ResultSet() {
	for (map<int, Cell*>::iterator i=data.begin(); i!=data.end(); i++) {
		delete (*i).second;
	}
}
