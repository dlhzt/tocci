#ifndef __CC_TOCCI_H__
#define __CC_TOCCI_H__

#define TOCCI 0x00000301L

extern "C"
{
#include <orid.h>
#include <oci.h>
};

#if defined (_MSC_VER)
#	pragma warning (disable: 4786) // id truncated to '255' chars in the browser info
#endif

#include <string>
#include <map>
#include <vector>
using namespace std;

#define OCCICURSOR SQLT_RSET
#define OCCIUNSIGNED_INT SQLT_UIN

namespace oracle {
	namespace occi {
		//typedef string SQLTypeName;
		class Connection;
		class AnyData;
		
		class PObject {
		protected:
			//static char * SQLTypeName;
			void *ctx;
		public:
//			static const char* SQLTypeName;
			virtual string getSQLTypeName() const {throw "fixme ;)";};
			PObject();
			PObject(void *ctxOCCI_);

			bool isNull();
			void setNull();

			virtual void writeSQL(oracle::occi::AnyData& streamOCCI_){};
			virtual void readSQL(AnyData& streamOCCI_){};

			void *operator new(size_t size);
			void *operator new(size_t size, const Connection * sess, const string& table, const string& name);
		};
		
		class SQLException : public PObject {
		public:
			SQLException(OCIError *err);
			SQLException(int code, char *msg);
			virtual ~SQLException(){};
			void setErrorCtx(void *c);
			int getErrorCode();
			const string& getMessage() const;
		private:
			string errmsg;
			int errcode;
		};


		class MetaData {
		public: 
			enum {Z, ATTR_NAME};
			string getString(int type);
			MetaData(const string &str);
		private:
			string name;
		};

		class Cell {
		public:
			void *ptr;
			int len;
			short type;
			Cell();
			~Cell();
		};

		class Statement;
		class ResultSet :public PObject {
		public:
			enum {END_OF_FETCH, DATA_AVAILABLE};
			int next();
			unsigned int getUInt(int pos);
			int getInt(int pos);
			const string getString(int pos);
			const vector<MetaData>& getColumnListMetaData() const;
			ResultSet();
			ResultSet(OCIStmt *st, OCIError *err);
			virtual ~ResultSet();
		private:
			OCIStmt *stmt;
			OCIError *errhp;
			map<int, Cell*> data;
			vector<MetaData> metadata;
		};

	class BindVariable {
	public: 
		OCIBind *bindhp;
		int type;
		void *data;
		int data_len;
		OCIType *oci_type;
		BindVariable(): bindhp(NULL),data(NULL),data_len(-1),oci_type(NULL) {}
		~BindVariable() {
			if (data!=NULL) free(data);
		}
	};

#define MAX_BINDS 32

	class Statement :public PObject{
		public:
			void setPrefetchRowCount(int pfc);
			void registerOutParam(int pos, int type);
			int executeUpdate();
			ResultSet *executeQuery();
			unsigned int getUInt(int pos);
			void setUInt(int pos, unsigned int val);
			void setString(int pos, const string &str);
			ResultSet * getCursor(int pos);
			void setVector(int pos, const vector<PObject*> &v, const char *type);
			bool getAutoCommit() const;
			void setAutoCommit(bool ac);

			Statement(Connection *con, const char *q);
			virtual ~Statement();
			void closeResultSet(ResultSet *rs);
			inline OCIStmt *getOCIStatement() const {return stmt;}
			inline OCIError *getOCIError() const {return errhp;}

		private: 
			OCIEnv *envhp;
			OCIError *errhp;
			OCIStmt *stmt;
			OCISvcCtx *svchp;
			string query;
			bool autoCommit;
			BindVariable var[MAX_BINDS];
		protected: 
		};

		class f_rw{
		public:
			void *fr(void *);
			void fw(void *, void *);
		};

		class Map {
		public:
			void put(const string& key, void *fr(void *), void fw(void *, void *)) {
			}
			Map() {}
		protected:
		private : 
		//	map<string, f_rw> m;
		};

		class Environment;
		class Connection  :public PObject{
		public: 
			void commit();
			void rollback();
			Statement * createStatement(const char *query) ;
			void terminateStatement(Statement *st);
			Connection(Environment *env, const char *login, const char *pass, const char *cstr);
			
			inline OCISvcCtx * getOCIServiceContext() const {return svchp;}
			inline Environment *getEnvironment() const {return environment;}
			virtual ~Connection();
		private:
			Environment *environment;
			OCIEnv *envhp;
			OCIError *errhp;
			OCISvcCtx *svchp;
		};


		class Environment :public PObject {
		public: 
			friend class SQLException;
			typedef int Mode;
			enum {
				DEFAULT = OCI_DEFAULT, 
				OBJECT=OCI_OBJECT, 
				SHARED=OCI_SHARED,
				NO_USERCALLBACKS=OCI_NO_UCB,
				THREADED_MUTEXED=OCI_THREADED, 
				THREADED_UNMUTEXED=OCI_THREADED | OCI_ENV_NO_MUTEX
			};

			static Environment *createEnvironment(Mode mode);
			Connection *createConnection(const char *login, const char *pass, const char *str) ;
			void terminateConnection(Connection *c);
			static void terminateEnvironment(Environment *e);

			Map *getMap();
			Environment(Mode m);
			virtual ~Environment();
			inline OCIEnv *getOCIEnviroment() const {return envhp;}
			inline OCIError *getOCIError() const {return errhp;}
		private:
			Mode mode;
			OCIEnv *envhp;
			OCIError *errhp;
			Map map;

		protected: 
		};
		

		/*add variant-like class implementation*/
		class AnyDataElement{
		public: 
			string name;
			int value;
			AnyDataElement(const string &n, const int v):name(n), value(v) {}
		};

		class AnyData : public PObject {
		public:
			AnyData(void *ctxOCCI_);
			void setNumber(int n);
			void setNumber(const string& name, int n);
		private:
			void *ctx;
		};

		void setVector(Statement *str, int pos, const vector<PObject*> &v, const char *type);
	} 

}

#endif
