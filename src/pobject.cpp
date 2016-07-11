#include "tocci/tocci.h"

using namespace oracle::occi;

//const char *PObject::SQLTypeName="qwe";

PObject::PObject() : ctx(NULL) {
}
PObject::PObject(void *ctxOCCI_):ctx(ctxOCCI_) {
}
bool PObject::isNull() {return (ctx==NULL);}
void PObject::setNull() {ctx=NULL;}

void *PObject::operator new(size_t size) { 
	return (void *)new char[size]; 
}

void *PObject::operator new(size_t size, const Connection * sess, const string& table, const string& name) {	
	return (void *)new char[size];
}
