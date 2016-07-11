#include "tocci/tocci.h"

using namespace oracle::occi;

SQLException::SQLException(int code, char *msg) {
	errcode=code;
	errmsg=msg;
}

SQLException::SQLException(OCIError *err) {
	char msgbuf[512];
    memset((void *) msgbuf, 0, sizeof(msgbuf));

	OCIErrorGet(err, 1, (text *) NULL, &errcode,
                       (ub1*)msgbuf, sizeof(msgbuf),  OCI_HTYPE_ERROR);
	errmsg=msgbuf;
}
void SQLException::setErrorCtx(void *c) {ctx=c;}
int SQLException::getErrorCode() {return errcode;}
const string& SQLException::getMessage() const {
	return errmsg;
}
