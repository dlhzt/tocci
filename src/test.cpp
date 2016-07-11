/*

Copyright (c) 2003, Confident Outsourcing
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer. 

Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution. 

Neither the name of the Confident Outsourcing nor the names of its 
contributors may be used to endorse or promote products derived from 
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "tocci.h"
#include <stdio.h>

using namespace oracle::occi;


class CKeyword : public PObject
{
public:
    static const char* SQLTypeName ;
	/*HERE TOCCI MISBEHAVES FROM ORIGINAL OCCI*/
private:
    unsigned long word_id;
    unsigned long case_mask;
    unsigned short type;
    unsigned short length;
    
public:

    void setData(  const unsigned long &iword_id,
		 const unsigned long &icase_mask,
		 const unsigned short &itype,
		 const unsigned short &ilength )
    {
	word_id = iword_id;
	case_mask = icase_mask;
	type = itype;
	length = ilength;
    } 
    CKeyword(){};

    void *operator new(size_t size)
    {
	return PObject::operator new(size);
    };

/*    void operator delete(size_t size)
    {
	PObject::operator delete(size);
    };
  */  
    void *operator new(size_t size, const Connection * sess,
			    const string& table)
    {
	return PObject::operator new(size, sess, table, SQLTypeName );
    };
    
    virtual string getSQLTypeName() const
    {
	return string(SQLTypeName);
    };
    
    CKeyword(void *ctxOCCI_) : PObject (ctxOCCI_) {};
    static void *s_readSQL(void *ctxOCCI_){ return NULL; };
    virtual void readSQL(AnyData& streamOCCI_){};
    static void s_writeSQL(void *objectOCCI_, void *ctxOCCI_)
    {
	CKeyword *objOCCI_ = (CKeyword *) objectOCCI_;
	AnyData streamOCCI_(ctxOCCI_);
	try
	{
	    if (objOCCI_->isNull())
		streamOCCI_.setNull();
	    else
	    objOCCI_->writeSQL(streamOCCI_);
	}
	catch (SQLException& e)
	{
	    e.setErrorCtx(ctxOCCI_);
	}
	return;
    };
    virtual void writeSQL(oracle::occi::AnyData& streamOCCI_)
    {
	streamOCCI_.setNumber("WORD_ID", word_id);
	streamOCCI_.setNumber("CASE_MASK", case_mask);
	streamOCCI_.setNumber("TYPE", type);
	streamOCCI_.setNumber("LENGTH", length);
    };
};
const char *CKeyword::SQLTypeName = "KEYWORD";


int main() {
    char connstr[256];
	vector<PObject *> v;
	
    sprintf( connstr, "(DESCRIPTION =\n(ADDRESS_LIST =\n(ADDRESS = (PROTOCOL = TCP)(HOST = %s)(PORT = %u))\n)\n(CONNECT_DATA =\n(SERVICE_NAME = %s)\n)\n)", "hell.wintech.prj", 1521, "HDIS" );
    try{
		Environment *env = Environment::createEnvironment( (Environment::Mode)(Environment::OBJECT | Environment::THREADED_MUTEXED ));
		Connection *con = env->createConnection ("HDIS", "HDIS", connstr);
		
		CKeyword *pkey = new CKeyword();
		pkey->setData( 5220, 1, 1, 4 ); 
		v.push_back( (PObject*)pkey );

		Statement *stmt = con->createStatement("BEGIN :v1 :=hdis.find_matches( :v2, :v3, :v4, :v5, :v6, :v7, :v8 ); END;");
		stmt->setAutoCommit(true);
		stmt->registerOutParam (1, OCCICURSOR );
		stmt->setUInt( 2, 1047 );
		stmt->setUInt( 3, 31 );
		setVector( stmt, 4, v, "TAB_KEYWORDS" );
		stmt->registerOutParam (5, OCCIUNSIGNED_INT );
		stmt->setUInt( 5, 10 );
		stmt->registerOutParam (6, OCCIUNSIGNED_INT );
		stmt->setUInt( 6, 10 );
		stmt->setString( 7, string("+test") );
		stmt->setUInt(8, 31);
	//	MessageBox(NULL, "i am here", "qw", 0);
		stmt->executeUpdate ();
//		ResultSet *rs=stmt->getCursor(1);
//		int n=0;
//		while(rs->next())  n++;
//		printf("%d\n", n);

//		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
		env->terminateConnection(con);
		Environment::terminateEnvironment(env);
		delete pkey;

	} catch (SQLException &e) {
		printf("%s\n", e.getMessage().c_str());
	} catch(...) {
		printf("UNHANDLED EXCEPTION. BUG\n");
	}
//	_CrtDumpMemoryLeaks();
	exit(0);
	return 0;
}
