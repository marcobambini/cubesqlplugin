/*
 *  CubeSQLPlugin.h
 *  CubeSQLPlugin
 *
 *  Created by Marco Bambini on 2/17/11.
 *  Copyright 2011 SQLabs. All rights reserved.
 *
 */

#include "rb_plugin.h"
#include "cubesql.h"
#include "csql.h"

#define	PING_FREQUENCY		300 // on the server it is specified as 300
#define DEBUG_WRITE(...)	if (debugFile != NULL) debug_write(__VA_ARGS__)
#define PLUGIN_VERSION		"3.2.0"
#define SSL_NOVERSION		"N/A"
#define MAX_TYPES_COUNT     512

#define kTempError1			100
#define kTempError2			101
#define kTempError3			102
#define kTempError4			103
#define kTempError5			104
#define kTempError6			105
#define kTempError7			106

#if _MSC_VER
#define snprintf _snprintf
#pragma pack(push, 1)
#endif
struct dbDatabase {
	csqldb				*db;					// the CubeSQL database
	int					referenceCount;			// to protect the reference
	int					pingFrequency;			// how often a ping message is sent
	int					timeout;				// optional timeout value
	int					port;					// port property
	int					encryption;				// encryption property
	long				timerID;				// background task
	REALstring			token;					// optional token
	REALstring			sslCipherList;			// optional cipher list
	REALstring			sslCertificatePassword;	// optional password used to decrypt the certificate file
	REALfolderItem		sslCertificate;			// path to SSL certificate file
	REALfolderItem		rootCertificate;		// path to Root certificate file
	Boolean				isConnected;			// flag to check if the db is really connected
	Boolean				autoCommit;				// optional autocommit settings
	Boolean				endChunkReceived;		// flag to check if endChunk has been received
	Boolean				useREALServerProtocol;	// flag to set if you want to use the old REALSQLServer protocol
	char				tempErrorMsg[256];		// temporary error string
	int					tempErrorCode;			// temporary error code
	
	char				filler[3];
	Boolean				traceEnabled;
	void*				traceEvent;
}
#ifdef __GCC__
__attribute__ ((packed));
#elif _MSC_VER
#pragma pack( pop )
#endif
;

struct dbCursor {
	csqlc				*c;
	Boolean				isLocked;
	Boolean				firstRowCalled;
};

struct cubeSQLVM {
	csqlvm              *vm;
};

struct cubeSQLPrepare {
    csqlvm              *vm;
    int                 types[MAX_TYPES_COUNT];
};

// accessories
csqlc			*REALServerBuildFieldSchemaCursor (csqlc *c);
char			*REALbasicBoolean2Integer (char *realvalue);
int				REALbasic2CubeSQLColumnType (int rbtype);
void			debug_write (const char *format, ...);
void			debug_fileopen (REALfolderItem value);
void			debug_fileclose (void);
void			TraceEventSetter (REALobject instance, long param, Boolean value);
void			rb_trace (const char* sql, void *userData);
int             GetVarType (REALobject value);

// implicit methods
void			DatabaseConstructor(REALobject instance);
void			DatabaseDestructor(REALobject instance);
void			DatabaseClose(dbDatabase *database);
long			DatabaseLastErrorCode(dbDatabase *database);
REALstring		DatabaseLastErrorString(dbDatabase *database);
REALdbCursor	DatabaseTableSchema(dbDatabase *database);
REALdbCursor	DatabaseIndexSchema(dbDatabase *database, REALstring tableName);
REALdbCursor	DatabaseFieldSchema(dbDatabase *database, REALstring tableName);
void			DatabaseSQLExecute(dbDatabase *aDatabase, REALstring sql);
REALdbCursor	DatabaseSQLSelect(dbDatabase *aDatabase, REALstring sql);
void			DatabaseAddTableRecord(dbDatabase *database, REALstring tableName, REALcolumnValue *values);
void			DatabaseGetSupportedTypes(int32_t **dataTypes, char **dataNames, size_t *count);
REALobject		DatabasePrepare(REALobject instance, REALstring sql);

// vm class
void			CubeSQLVMConstructor (REALobject instance);
void			CubeSQLVMDestructor (REALobject instance);
void			CubeSQLVMBindBlob (REALobject instance, int index, REALstring blob);
void			CubeSQLVMBindDouble (REALobject instance, int index, double n);
void			CubeSQLVMBindInt (REALobject instance, int index, int n);
void			CubeSQLVMBindInt64 (REALobject instance, int index, int64 n);
void			CubeSQLVMBindNull (REALobject instance, int index);
void			CubeSQLVMBindZeroBlob (REALobject instance, int index, int len);
void			CubeSQLVMBindText (REALobject instance, int index, REALstring str);
void			CubeSQLVMExecute (REALobject instance);
REALdbCursor	CubeSQLVMSelect (REALobject instance);
REALdbCursor    CubeSQLVMSelectRowSet (REALobject instance);
void			CubeSQLVMClose (REALobject instance);

// prepare class
void            CubeSQLPrepareConstructor (REALobject instance);
void            CubeSQLPrepareDestructor (REALobject instance);
void            CubeSQLPrepareBindValue (REALobject instance, int index, REALobject value);
void            CubeSQLPrepareBindValueType (REALobject instance, int index, REALobject value, int type);
void            CubeSQLPrepareBindValues (REALobject instance, REALarray values);
void            CubeSQLPrepareBindType (REALobject instance, int index, int type);
void            CubeSQLPrepareBindTypes (REALobject instance, REALarray types);
void            CubeSQLPrepareExecuteSQL (REALobject instance, REALarray values);
REALdbCursor    CubeSQLPrepareSelectSQL (REALobject instance, REALarray values);
//void            CubeSQLPrepareExecuteSQLNoValues (REALobject instance);
//REALdbCursor    CubeSQLPrepareSelectSQLNoValues (REALobject instance);
void            CubeSQLPrepareSQLExecute (REALobject instance, REALarray params);
REALdbCursor    CubeSQLPrepareSQLSelect (REALobject instance, REALarray params);

// new methods
void			DatabaseSetTempError(dbDatabase *db, const char *errorMsg, int errorCode);
Boolean			DatabaseConnect(REALdbDatabase instance);
Boolean			DatabaseIsConnected(REALobject instance);
long long		DatabaseLastRowID(REALobject instance);
int				DatabaseErrCode (REALobject instance);
REALstring		DatabaseErrMessage (REALobject instance);
Boolean			DatabasePing(REALobject instance);
void			DatabaseSendEndChunk(REALobject instance);
void			DatabaseSendAbortChunk(REALobject instance);
void			DatabaseSendChunk(REALobject instance, REALstring s);
REALstring		DatabaseReceiveChunk(REALobject instance);

// cursor methods
dbCursor		*CursorCreate(csqlc *c);
void			CursorDestroy(dbCursor* cursor);
int				CursorColumnCount(dbCursor *cursor);
int				CursorRowCount(dbCursor *cursor);
REALstring		CursorColumnName(dbCursor *cursor, int column);
int				CursorColumnType(dbCursor *cursor, int index);
void			CursorColumnValue(dbCursor *cursor, int column, void **value, unsigned char *type, int *length);
void			CursorFieldSchemaColumnValue(dbCursor *cursor, int column, void **value, unsigned char *type, int *length);
void			CursorDelete(dbCursor *cursor);
void			CursorUpdate(dbCursor *cursor, REALcursorUpdate *updates);
RBBoolean		CursorNextRow(dbCursor *cursor);
void			CursorPrevRow(dbCursor *cursor);
void			CursorFirstRow(dbCursor *cursor);
void			CursorLastRow(dbCursor *cursor);
void			CursorClose(dbCursor *cursor);
void			CursorEdit(dbCursor *cursor);
void			CursorCheckClearLock(dbCursor *cursor);
REALstring		CursorTableName(REALobject instance, REALdbCursor rs);
Boolean			CursorGoToRow(REALobject instance, REALdbCursor rs, int index);

// properties
REALstring		ServerVersionGetter(REALobject instance, long param);
int				PortGetter(REALobject instance, long param);
void			PortSetter(REALobject instance, long param, int value);
int				TimeoutGetter(REALobject instance, long param);
void			TimeoutSetter(REALobject instance, long param, int value);
Boolean			AutoCommitGetter(REALobject instance, long param);
void			AutoCommitSetter(REALobject instance, long param, Boolean value);
int				EncryptionGetter(REALobject instance, long param);
void			EncryptionSetter(REALobject instance, long param, int value);
void			DebugFileSetter(REALobject instance, long param, REALfolderItem value);
void			TraceFunctionSetter(REALobject instance, long param, void *value);
int				PingFrequencyGetter(REALobject instance, long param);
void			PingFrequencySetter(REALobject instance, long param, int value);
void			PingTimerStart(dbDatabase *database);
void			PingTimerStop(dbDatabase *database);
void			DoPing (void *data);
Boolean			EndChunkGetter(REALobject instance, long param);
Boolean			UseREALServerProtocolGetter(REALobject instance, long param);
void			UseREALServerProtocolSetter(REALobject instance, long param, Boolean value);
void			SSLCertificateSetter(REALobject instance, long param, REALfolderItem value);
void			SSLRootCertificateSetter(REALobject instance, long param, REALfolderItem value);
void			TokenSetter(REALobject instance, long param, REALstring value);
REALstring		TokenGetter(REALobject instance, long param);


REALstring		PluginVersionGetter(void);
REALstring		OpenSSLVersionGetter(void);
int             OpenSSLVersionNumGetter(void);
Boolean			NULLAsStringGetter(void);
void			NULLAsStringSetter(Boolean value);
Boolean			BLOBAsStringGetter(void);
void			BLOBAsStringSetter(Boolean value);
Boolean			BooleanAsIntegerGetter(void);
void			BooleanAsIntegerSetter(Boolean value);
void			SSLLibrarySetter(REALfolderItem value);
void			CryptoLibrarySetter(REALfolderItem value);
REALstring		REALbasicPathFromFolderItem (REALfolderItem value);

// MARK: - NEW 2019 API -
REALdbCursor    DatabaseSelectSQL(dbDatabase *db, REALstring sql, REALarray params);
void            DatabaseExecuteSQL(dbDatabase *db, REALstring sql, REALarray params);
REALobject      DatabasePrepareStatement(dbDatabase *db, REALstring statement);
REALobject      CubeSQLDatabasePrepare(REALdbDatabase dbObject, REALstring statement);
void            DatabaseCommit(dbDatabase *db);
void            DatabaseRollback(dbDatabase *db);
void            BeginTransaction(dbDatabase *db);
    
void			DatabaseLock(dbDatabase *database);
void			DatabaseUnlock(dbDatabase *database);
void			PluginEntry(void);

REALmethodDefinition CubeSQLDatabaseMethods[] = {
	{ (REALproc) DatabaseConnect, REALnoImplementation, "Connect() as Boolean", REALconsoleSafe},
	{ (REALproc) DatabaseIsConnected, REALnoImplementation, "IsConnected() as Boolean", REALconsoleSafe},
	{ (REALproc) DatabaseLastRowID, REALnoImplementation, "LastRowID() as Int64", REALconsoleSafe},
	{ (REALproc) DatabaseErrCode, NULL, "ErrCode() As Integer", REALconsoleSafe},
	{ (REALproc) DatabaseErrMessage, NULL, "ErrMsg() As String", REALconsoleSafe},
	{ (REALproc) DatabaseSendChunk, REALnoImplementation, "SendChunk(chunk as String)", REALconsoleSafe},
	{ (REALproc) DatabaseReceiveChunk, REALnoImplementation, "ReceiveChunk() as String", REALconsoleSafe},
	{ (REALproc) DatabaseSendEndChunk, REALnoImplementation, "SendEndChunk()", REALconsoleSafe},
	{ (REALproc) DatabaseSendAbortChunk, REALnoImplementation, "SendAbortChunk()", REALconsoleSafe},
	{ (REALproc) DatabasePing, REALnoImplementation, "Ping() as Boolean", REALconsoleSafe},
	{ (REALproc) DatabasePrepare, REALnoImplementation, "VMPrepare(sql as String) as CubeSQLVM", REALconsoleSafe},
    { (REALproc) CubeSQLDatabasePrepare, REALnoImplementation, "Prepare(statement As String) as CubeSQLPreparedStatement", REALconsoleSafe},
	{ (REALproc) CursorGoToRow, REALnoImplementation, "GoToRow(rs As RecordSet, index As Integer) As Boolean", REALconsoleSafe},
	{ (REALproc) CursorTableName, REALnoImplementation, "TableName(rs As RecordSet) As String", REALconsoleSafe},
};

REALproperty CubeSQLDatabaseProperties[] = {
	{ NULL, "ServerVersion", "String", REALconsoleSafe, (REALproc) ServerVersionGetter, NULL },
	{ NULL, "Port", "Integer", REALconsoleSafe, (REALproc) PortGetter, (REALproc) PortSetter },
	{ NULL, "Timeout", "Integer", REALconsoleSafe, (REALproc) TimeoutGetter, (REALproc) TimeoutSetter },
	{ NULL, "AutoCommit", "Boolean", REALconsoleSafe, (REALproc) AutoCommitGetter, (REALproc) AutoCommitSetter },
	{ NULL, "Encryption", "Integer", REALconsoleSafe, (REALproc) EncryptionGetter, (REALproc) EncryptionSetter },
	{ NULL, "DebugFile", "FolderItem", REALconsoleSafe, NULL, (REALproc) DebugFileSetter },
	{ NULL, "TraceEnabled", "Boolean", REALconsoleSafe, REALstandardGetter, (REALproc) TraceEventSetter, FieldOffset(dbDatabase, traceEnabled)},
	{ NULL, "EndChunk", "Boolean", REALconsoleSafe, (REALproc) EndChunkGetter, NULL },
	{ NULL, "UseREALServerProtocol", "Boolean", REALconsoleSafe, (REALproc) UseREALServerProtocolGetter, (REALproc) UseREALServerProtocolSetter },
	{ NULL, "IsEndChunk", "Boolean", REALconsoleSafe, (REALproc) EndChunkGetter, NULL },
	{ NULL, "PingFrequency", "Integer", REALconsoleSafe, (REALproc) PingFrequencyGetter, (REALproc) PingFrequencySetter },
	{ NULL, "Token", "String", REALconsoleSafe, (REALproc) TokenGetter, (REALproc) TokenSetter },
	{ NULL, "SSLCertificate", "FolderItem", REALconsoleSafe, REALstandardGetter, (REALproc) SSLCertificateSetter },
	{ NULL, "RootCertificate", "FolderItem", REALconsoleSafe, REALstandardGetter, (REALproc) SSLRootCertificateSetter },
	{ NULL, "SSLCipherList", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter, FieldOffset(dbDatabase, sslCipherList)},
	{ NULL, "SSLCertificatePassword", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter, FieldOffset(dbDatabase, sslCertificatePassword)},
};

REALproperty CubeSQLModuleProperties[] = {
	{ NULL, "NULLAsString", "Boolean", REALconsoleSafe, (REALproc) NULLAsStringGetter, (REALproc) NULLAsStringSetter },
	{ NULL, "BLOBAsString", "Boolean", REALconsoleSafe, (REALproc) BLOBAsStringGetter, (REALproc) BLOBAsStringSetter },
	{ NULL, "BooleanAsInteger", "Boolean", REALconsoleSafe, (REALproc) BooleanAsIntegerGetter, (REALproc) BooleanAsIntegerSetter },
	{ NULL, "SSLLibrary", "FolderItem", REALconsoleSafe, NULL, (REALproc) SSLLibrarySetter },
	{ NULL, "CryptoLibrary", "FolderItem", REALconsoleSafe, NULL, (REALproc) CryptoLibrarySetter },
	{ NULL, "Version", "String", REALconsoleSafe, (REALproc) PluginVersionGetter, NULL },
	{ NULL, "OpenSSLVersion", "String", REALconsoleSafe, (REALproc) OpenSSLVersionGetter, NULL },
    { NULL, "OpenSSLVersionNumber", "Integer", REALconsoleSafe, (REALproc) OpenSSLVersionNumGetter, NULL },
};

REALconstant CubeSQLConstants[] = {
	{"kAESNONE = 0", NULL, 0},
	{"kAES128 = 2", NULL, 0},
	{"kAES192 = 3", NULL, 0},
	{"kAES256 = 4", NULL, 0},
	{"kSSL = 8", NULL, 0},
};

REALconstant CubeSQLPrepareConstants[] = {
    {"CUBESQL_INTEGER = 1", NULL, 0},
    {"CUBESQL_DOUBLE = 2", NULL, 0},
    {"CUBESQL_TEXT = 3", NULL, 0},
    {"CUBESQL_BLOB = 4", NULL, 0},
    {"CUBESQL_NULL = 5", NULL, 0},
    {"CUBESQL_INT64 = 8", NULL, 0},
    {"CUBESQL_ZEROBLOB = 9", NULL, 0},
};

REALevent CubeSQLEvents[] = {
	{ "Trace(sql As String)" }
};

REALmoduleDefinition CubeSQLModule = {
	kCurrentREALControlVersion,
	"CubeSQLPlugin",
	NULL, 0,
	CubeSQLConstants,
    sizeof(CubeSQLConstants) / sizeof(REALconstant),
	CubeSQLModuleProperties,
	sizeof(CubeSQLModuleProperties) / sizeof(REALproperty),
};

REALmethodDefinition CubeSQLVMMethods[] = {
	{ (REALproc) CubeSQLVMBindInt, NULL, "BindInt(index As Integer, value As Integer)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindDouble, NULL, "BindDouble(index As Integer, value As Double)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindText, NULL, "BindText(index As Integer, value As String)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindBlob, NULL, "BindBlob(index As Integer, value As String)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindInt64, NULL, "BindInt64(index As Integer, value As Int64)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindNull, NULL, "BindNull(index As Integer)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMBindZeroBlob, NULL, "BindZeroBlob(index As Integer, length As Integer)", REALconsoleSafe},
	{ (REALproc) CubeSQLVMExecute, NULL, "VMExecute()", REALconsoleSafe},
	{ (REALproc) CubeSQLVMSelect, NULL, "VMSelect() As RecordSet", REALconsoleSafe},
    { (REALproc) CubeSQLVMSelectRowSet, NULL, "VMSelectRowSet() As RowSet", REALconsoleSafe},
};

REALmethodDefinition CubeSQLPrepareMethods[] = {
    { (REALproc) CubeSQLPrepareBindValue, NULL, "Bind(index As Integer, value As Variant)", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareBindValueType, NULL, "Bind(index As Integer, value As Variant, type As Integer)", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareBindValues, NULL, "Bind(values() As Variant)", REALconsoleSafe},
    
    { (REALproc) CubeSQLPrepareBindType, NULL, "BindType(index As Integer, type As Integer)", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareBindTypes, NULL, "BindType(types() As Integer)", REALconsoleSafe},
    
    { (REALproc) CubeSQLPrepareSQLSelect, REALnoImplementation, "SQLSelect(ParamArray params As Variant) As RecordSet", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareSelectSQL, REALnoImplementation, "SelectSQL(ParamArray params As Variant) As RowSet", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareSQLExecute, REALnoImplementation, "SQLExecute(ParamArray params As Variant)", REALconsoleSafe},
    { (REALproc) CubeSQLPrepareExecuteSQL, REALnoImplementation, "ExecuteSQL(ParamArray params As Variant)", REALconsoleSafe},
};

REALclassDefinition CubeSQLVMClass = {
	kCurrentREALControlVersion,
	"CubeSQLVM",
	NULL,
	sizeof(cubeSQLVM),
	0,
	(REALproc) CubeSQLVMConstructor,
    (REALproc) CubeSQLVMDestructor,
	NULL,
	0,
	CubeSQLVMMethods,
    sizeof(CubeSQLVMMethods) / sizeof(REALmethodDefinition),
	NULL,0,
};

REALclassDefinition CubeSQLPrepareClass = {
    kCurrentREALControlVersion,
    "CubeSQLPreparedStatement",
    NULL,
    sizeof(cubeSQLPrepare),
    0,
    (REALproc) CubeSQLPrepareConstructor,
    (REALproc) CubeSQLPrepareDestructor,
    NULL,
    0,
    CubeSQLPrepareMethods,
    sizeof(CubeSQLPrepareMethods) / sizeof(REALmethodDefinition),
    NULL,0, // events
    NULL,0, // eventInstances
    "PreparedSQLStatement",   // interfaces
    NULL,0, // attributes
    CubeSQLPrepareConstants,
    sizeof(CubeSQLPrepareConstants) / sizeof(REALconstant),
    0,      // mFlags
};

REALclassDefinition CubeSQLDatabaseClass = {
	kCurrentREALControlVersion,
	"CubeSQLServer",
	"Database",
	sizeof(dbDatabase),
	0,
	(REALproc) DatabaseConstructor,
	(REALproc) DatabaseDestructor,
	CubeSQLDatabaseProperties,
	sizeof(CubeSQLDatabaseProperties) / sizeof(REALproperty),
	CubeSQLDatabaseMethods,
	sizeof(CubeSQLDatabaseMethods) / sizeof(REALmethodDefinition),
	CubeSQLEvents,
	sizeof(CubeSQLEvents) / sizeof(REALevent),
	NULL,0,
};

REALdbCursorDefinition CubeSQLFieldSchemaCursor = {
	kCurrentREALControlVersion,
	0,
	CursorClose,
	CursorColumnCount,
	CursorColumnName,
	CursorRowCount,
	CursorFieldSchemaColumnValue,
	nil,		// Releasevalue
	CursorNextRow,
	nil,		// CursorDelete
	nil,		// CursorDeleteAll,
	nil,		// FieldKeyFunc
	nil,		// CursorUpdate
	nil,		// CursorEdit
	CursorPrevRow,
	CursorFirstRow,
	CursorLastRow,
	CursorColumnType
};

REALdbEngineDefinition CubeSQLEngine = {
	kCurrentREALControlVersion,     // version
	0,                              // forSystemUse
	dbEnginePrimaryKeySupported | dbEngineDontUseBrackets,  // flags1
    0,  // flags2
    0,  // flags3
    
	DatabaseClose,
	DatabaseTableSchema,
	DatabaseFieldSchema,
	DatabaseSQLSelect,
	DatabaseSQLExecute,
	nil,		// CreateTable
	DatabaseAddTableRecord,
	DatabaseSelectSQL,
	nil,		// UpdateFields
	nil,		// AddTableColumn
	DatabaseIndexSchema,
	DatabaseLastErrorCode,
	DatabaseLastErrorString,
	DatabaseCommit,
	DatabaseRollback,
	BeginTransaction,
	DatabaseGetSupportedTypes,
	NULL,		// dropTable
	NULL,		// dropColumn
	NULL,		// alterTableName
	NULL,		// alterColumnName
	NULL,		// alterColumnType
	NULL,		// alterColumnConstraint
	DatabasePrepareStatement,
    DatabaseExecuteSQL,
    NULL
};

REALdbCursorDefinition CubeSQLCursor = {
	kCurrentREALControlVersion,
	0,
	CursorClose,
	CursorColumnCount,
	CursorColumnName,
	CursorRowCount,
	CursorColumnValue,
	nil,		// ReleaseValue
	CursorNextRow,
	CursorDelete,
	nil,		// CursorDeleteAll,
	nil,		// FieldKeyFunc
	CursorUpdate,
	CursorEdit,
	CursorPrevRow,
	CursorFirstRow,
	CursorLastRow,
	CursorColumnType
};
