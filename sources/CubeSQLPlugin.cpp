/*
 *  CubeSQLPlugin.cpp
 *  CubeSQLPlugin
 *
 *  Created by Marco Bambini on 2/17/11.
 *  Copyright 2011 SQLabs. All rights reserved.
 *
 */

#include "CubeSQLPlugin.h"
#include "rb_plugin.h"
#include <stdarg.h>

#include <string>
#include <sstream>
#include <iostream>
using namespace std;


#if !TARGET_COCOA
#define REALBuildStringWithEncoding	    REALBuildString
#define REALGetPropValueString		    REALGetPropValue
#define REALnewInstanceWithClass	    REALnewInstance
#endif

// https://www.mail-archive.com/realbasic-plugins@lists.realsoftware.com/msg01029.html
#define REALGetCString(_s)              (const char *)REALGetStringContents(_s, NULL)

// Data types and their names when running in versions of REALbasic 2006.04 or higher
static long sDataTypes[]		= {	dbTypeText, dbTypeText, dbTypeLong, dbTypeInt64, dbTypeDouble, 
									dbTypeDouble, dbTypeBoolean, dbTypeDate, dbTypeTime,
									dbTypeTimeStamp, dbTypeBinary, dbTypeBinary, dbTypeCurrency };

static char sDataTypeNames[]	= { "Text,Varchar,Smallint,Integer,Float,"
									"Double,Boolean,Date,Time,"
									"Timestamp,Binary,Blob,Currency"};

static long sDataTypesCount		= sizeof(sDataTypes);

static FILE	*debugFile			= NULL;

// Global settings
static Boolean nullAsString	= true;
static Boolean blobAsString = true;
static Boolean booleanAsInteger = true;

// MARK: - Database API -

void DatabaseConstructor(REALobject instance) {
	DEBUG_WRITE("DatabaseConstructor");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	memset((void *)data, 0, sizeof(dbDatabase));
	data->db = NULL;
	data->pingFrequency = PING_FREQUENCY;
	data->timeout = CUBESQL_DEFAULT_TIMEOUT;
	data->port = CUBESQL_DEFAULT_PORT;
	data->encryption = CUBESQL_ENCRYPTION_NONE;
	data->isConnected = false;
	data->autoCommit = false;
	data->timerID = 0;
	data->referenceCount = 0;
	data->endChunkReceived = false;
	data->useREALServerProtocol = false;
	data->traceEnabled = false;
	data->traceEvent = NULL;
	data->sslCertificate = NULL;
	data->rootCertificate = NULL;
	data->token = NULL;
	
	REALConstructDBDatabase((REALdbDatabase) instance, data, &CubeSQLEngine);
}

void DatabaseDestructor(REALobject instance) {
	DEBUG_WRITE("DatabaseDestructor");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	DatabaseClose(data);
}

void DatabaseClose(dbDatabase *database) {
	DEBUG_WRITE("DatabaseClose");
	if (database->db == NULL) return; 
	DatabaseUnlock(database);
}

Boolean DatabaseConnect(REALdbDatabase instance) {
	DEBUG_WRITE("DatabaseConnect");
	dbDatabase *database = REALGetDBFromREALdbDatabase(instance);
	if (database == NULL) return false;
	if (database->isConnected) return true;
	
	// get implicit properties
	REALstring dbHost = REALGetDBHost(instance);
	REALstring dbUser = REALGetDBUserName(instance);
	REALstring dbPassword = REALGetDBPassword(instance);
	REALstring dbDatabaseName = REALGetDBDatabaseName(instance);
	
	// sanity check
	if ((dbHost == NULL) || (dbUser == NULL) || (dbPassword == NULL)) {
		DatabaseSetTempError(database, "Hostname, username and password cannot be NULL", kTempError1);
		return false;
	}
	
	// sanity check again on length
    if ((REALStringLength(dbHost) == 0) || (REALStringLength(dbUser) == 0) || (REALStringLength(dbPassword) == 0)) {
        DatabaseSetTempError(database, "Hostname, username or password cannot be empty", kTempError2);
        return false;
    }
    
	const char *s1 = REALGetCString(dbHost);
	const char *s2 = REALGetCString(dbUser);
	const char *s3 = REALGetCString(dbPassword);
	
	char *token = NULL;
	const char *sslCertificate = NULL;
	const char *rootCertificate = NULL;
	const char *sslCertificatePassword = NULL;
	const char *sslCipherList = NULL;
	
	int	useREALServerProtocol = kFALSE;
	REALstring path1 = NULL;
	REALstring path2 = NULL;
	
	if (database->token != nil) token = (char *)REALGetCString(database->token);
	if (database->useREALServerProtocol) useREALServerProtocol = kTRUE;
	
	if (database->sslCertificate != NULL) {
		path1 = REALbasicPathFromFolderItem(database->sslCertificate);
		if (path1) sslCertificate = (const char*) REALGetCString(path1);
	}
	
	if (database->rootCertificate != NULL) {
		path2 = REALbasicPathFromFolderItem(database->rootCertificate);
		if (path2) rootCertificate = (const char*) REALGetCString(path2);
	}
	
	if (database->sslCertificatePassword)
		sslCertificatePassword = REALGetCString(database->sslCertificatePassword);
	
	if (database->sslCipherList)
		sslCipherList = REALGetCString(database->sslCipherList);
	
	// try to connecto to the server (sslCertificate can be NULL even if encryption is set to kSSL)
	int err = cubesql_connect_token(&database->db, s1, database->port, s2, s3, database->timeout, database->encryption, token,
									useREALServerProtocol, sslCertificate, rootCertificate, sslCertificatePassword, sslCipherList);
	if (err != CUBESQL_NOERR) {
		if (err == CUBESQL_SSL_ERROR) DatabaseSetTempError(database, "SSL Library Not Found", kTempError4);
		else if (err == CUBESQL_PARAMETER_ERROR) DatabaseSetTempError(database, "Parameters Error", kTempError3);
		else if (err == CUBESQL_MEMORY_ERROR) DatabaseSetTempError(database, "Memory Error", kTempError5);
		else if (err == CUBESQL_SSL_CERT_ERROR) DatabaseSetTempError(database, "SSL error while loading certificate file", kTempError7);
		return false;
	}
		
	// try to execute the USE DATABASE command if the user set the DatabaseName property
	char cmd[512];
	if (dbDatabaseName != NULL) {
		const char *s4 = REALGetCString(dbDatabaseName);
		if (strlen(s4) != 0) {
			snprintf(cmd, sizeof(cmd), "USE DATABASE '%s';", s4);
			if (cubesql_execute(database->db, cmd) != CUBESQL_NOERR) return false;
		}
	}
	
	if (database->useREALServerProtocol == false) {
		// try to set client type
		snprintf(cmd, sizeof(cmd), "SET CLIENT TYPE TO 'Xojo %f';", REALGetRBVersion());
		cubesql_execute(database->db, cmd);
		
		// check PingFrequency
		if (database->pingFrequency != PING_FREQUENCY) {
			snprintf(cmd, sizeof(cmd), "SET PING TIMEOUT TO %d;", database->pingFrequency);
			cubesql_execute(database->db, cmd);
		}
	}
	 
	if (database->pingFrequency != 0) {
		PingTimerStart(database);
	}
	
	// tell the REALbasic framework we are connected
	if (path1) REALUnlockString(path1);
	if (path2) REALUnlockString(path2);
	REALSetDBIsConnected(instance, true);
	database->isConnected = true;
	DatabaseLock(database);
	return true;
}

void DatabaseSetTempError(dbDatabase *db, const char *errorMsg, int errorCode) {
	if (db == NULL) return;
	if (errorMsg == NULL) return;
	
	DEBUG_WRITE(errorMsg);
	snprintf(db->tempErrorMsg, sizeof(db->tempErrorMsg), "%s", errorMsg);
	db->tempErrorCode = errorCode;
}

void DatabaseLock(dbDatabase *database) {
	database->referenceCount++;
}

void DatabaseUnlock(dbDatabase *database) {
	database->referenceCount--;
	if (database->referenceCount > 0) return;
	
	PingTimerStop(database);
	if ((database->db) && (database->isConnected))
		cubesql_disconnect(database->db, kFALSE);
	
	if (database->token) REALUnlockString(database->token);
	database->token = nil;
	database->db = NULL;
	database->isConnected = false;
}


long DatabaseLastErrorCode(dbDatabase *database) {
	if (database == NULL) return 0;
	
	if (database->db == NULL) return database->tempErrorCode;
	return cubesql_errcode(database->db);
}

REALstring DatabaseLastErrorString(dbDatabase *database) {
	if (database == NULL) return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	
	REALstring	s;
	char		*err = NULL;
	
	if (database->db == NULL) {
		err = (char *)database->tempErrorMsg;
	} else {
		err = cubesql_errmsg(database->db);
	}
	
	if (err) s = REALBuildStringWithEncoding(err, (int)strlen(err), kREALTextEncodingUTF8);
	else s = REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	
	return s;
}

int DatabaseErrCode (REALobject instance) {
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	return (int)DatabaseLastErrorCode(data);
}

REALstring DatabaseErrMessage (REALobject instance) {
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	return DatabaseLastErrorString(data);
}

REALdbCursor DatabaseTableSchema(dbDatabase *database) {
	DEBUG_WRITE("DatabaseTableSchema");
	if (database->isConnected == false) return NULL;
	csqlc *c = cubesql_select(database->db, "SELECT name as TableName FROM sqlite_master WHERE type='table' ORDER BY TableName;");
	if (c == NULL) return NULL;
	return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLFieldSchemaCursor);
}

REALdbCursor DatabaseIndexSchema(dbDatabase *database, REALstring tableName) {
	DEBUG_WRITE("DatabaseIndexSchema");
	if (database->isConnected == false) return NULL;
	char sql[512];
	snprintf(sql, sizeof(sql), "SELECT name as IndexName FROM sqlite_master WHERE type='index' AND tbl_name='%s';", REALGetCString(tableName));
	csqlc *c = cubesql_select(database->db, sql);
	if (c == NULL) return NULL;
	return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLFieldSchemaCursor);
}

REALdbCursor DatabaseFieldSchema(dbDatabase *database, REALstring tableName) {
	DEBUG_WRITE("DatabaseFieldSchema");
	if (database->isConnected == false) return NULL;
	char sql[512];
	
	if (database->useREALServerProtocol)
		snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", REALGetCString(tableName));
	else
		snprintf(sql, sizeof(sql), "SHOW TABLE INFO REALBASIC '%s';", REALGetCString(tableName));
	
	csqlc *c = cubesql_select(database->db, sql);
	if (c == NULL) return NULL;
	
	if (database->useREALServerProtocol) c = REALServerBuildFieldSchemaCursor(c);
	return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLFieldSchemaCursor);
}

void DatabaseSQLExecute(dbDatabase *database, REALstring sql) {
	DEBUG_WRITE("DatabaseSQLExecute %s", REALGetCString(sql));
	if (database->isConnected == false) return;
	database->endChunkReceived = false;
	cubesql_execute(database->db, REALGetCString(sql));
}

REALdbCursor DatabaseSQLSelect(dbDatabase *database, REALstring sql) {
	DEBUG_WRITE("DatabaseSQLSelect");
	if (database->isConnected == false) return NULL;
	database->endChunkReceived = false;
	csqlc *c = cubesql_select(database->db, REALGetCString(sql));
	if (c == NULL) return NULL;
	return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLCursor);
}

void DatabaseAddTableRecord(dbDatabase *database, REALstring tableName, REALcolumnValue *values) {
	DEBUG_WRITE("DatabaseAddTableRecord %s", REALGetCString(tableName));
	
	// retrieve information about the DatabaseRecord
	int		ncols = 0;
	int	   	index = 1;
	string 	cols = "";
 	string  vals = "";
	
	for (REALcolumnValue *value = values; value != NULL; value = value->nextColumn) {
		cols += REALGetCString(value->columnName);
		stringstream temp;
		temp<<index;
		vals += "?" + temp.str();
		
		if (value->nextColumn) {
			cols += ",";
			vals += ",";
		}
		
		index++;
		ncols++;
	}
	
	// build sql bind string
	std::string sql = "INSERT INTO ";
	sql += REALGetCString(tableName);
	sql += " (";
	sql += cols;
	sql += ") VALUES (";
	sql += vals;
	sql += ");";
	DEBUG_WRITE("Bind string is %s", sql.c_str());
	
	// build arrays needed by the cubesql_bind function
	char	**colvalue = NULL;
	int		*colsize = NULL;
	int		*coltype = NULL;
	char	*realvalue;
	
	colvalue = (char **) malloc(sizeof(char*)*ncols);
	colsize = (int *) malloc(sizeof(int)*ncols);
	coltype = (int *) malloc(sizeof(int)*ncols);
	if ((colvalue == NULL) || (colsize == NULL) || (coltype == NULL)) return;
	
	index = 0;
	for (REALcolumnValue *value = values; value != NULL; value = value->nextColumn) {
		realvalue = (char *)REALGetCString(value->columnValue);
		if ((value->columnType == dbTypeBoolean) && (booleanAsInteger)) realvalue = REALbasicBoolean2Integer(realvalue);
		colvalue[index] = realvalue;
		colsize[index] = (int)REALStringLength(value->columnValue);
		coltype[index] = REALbasic2CubeSQLColumnType(value->columnType);
		index++;
	}
	
	cubesql_bind (database->db, sql.c_str(), colvalue, colsize, coltype, ncols);
	
	free((void *)colvalue);
	free((void *)colsize);
	free((void *)coltype);
}

void DatabaseGetSupportedTypes(int32_t **dataTypes, char **dataNames, size_t *count) {
	DEBUG_WRITE("DatabaseGetSupportedTypes");
	*dataTypes = (int32_t *) sDataTypes;
	*dataNames = (char *) sDataTypeNames;
	*count = sDataTypesCount;
}

Boolean DatabaseIsConnected(REALobject instance) {
	DEBUG_WRITE("DatabaseIsConnected");
	return DatabasePing(instance);
}

long long DatabaseLastRowID(REALobject instance) {
	DEBUG_WRITE("DatabaseLastRowID");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return 0;
	if (data->isConnected == false) return 0;
	
	csqlc *c = cubesql_select(data->db, "SHOW LASTROWID;");
	if (c == NULL) return 0;
	
	int64 value = cubesql_cursor_int64 (c, 1, 1, 0);
	cubesql_cursor_free(c);
	
	return (long long)value;
}

Boolean DatabasePing(REALobject instance) {
	DEBUG_WRITE("DatabasePing");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return false;
	if (data->isConnected == false) return false;
	
	// execute PING command
	if (cubesql_execute(data->db, "PING;") != CUBESQL_NOERR)
		data->isConnected = false;
	else
		data->isConnected = true;
	
	return data->isConnected;
}

void DatabaseSendEndChunk(REALobject instance) {
	DEBUG_WRITE("DatabaseSendStopChunk");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return;
	if (data->isConnected == false) return;
	
	csql_ack(data->db, kCOMMAND_ENDCHUNK);
}

void DatabaseSendAbortChunk(REALobject instance) {
	DEBUG_WRITE("DatabaseSendStopChunk");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return;
	if (data->isConnected == false) return;
	
	cubesql_execute(data->db, "ABORT CHUNK;");
}

void DatabaseSendChunk(REALobject instance, REALstring s) {
	DEBUG_WRITE("DatabaseSendChunk");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return;
	if (data->isConnected == false) return;
	REALstringData sdata;
	if (!REALGetStringData(s, kREALTextEncodingUnknown, &sdata)) return;
	
	data->endChunkReceived = false;
	csql_sendchunk(data->db, (char *)sdata.data, (int)sdata.length, 0, kFALSE);
	csql_netread(data->db, -1, -1, kTRUE, NULL, NO_TIMEOUT);
    REALDisposeStringData(&sdata);
}

REALstring DatabaseReceiveChunk(REALobject instance) {
	REALstring 	s = NULL;
	int chunk_code = 0;
	
	DEBUG_WRITE("DatabaseReceiveChunk");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return NULL;
	if (data->isConnected == false) return NULL;
	data->endChunkReceived = false;
	
	int len = 0;
	int is_end_chunk = kFALSE;
	char *buf = csql_receivechunk(data->db, &len, &is_end_chunk);
	if (buf != NULL)
		s = REALBuildStringWithEncoding(buf, len, kREALTextEncodingUTF8);
	else
		s = REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	
	if (is_end_chunk == kTRUE) data->endChunkReceived = true;
	if (data->useREALServerProtocol) chunk_code = kCHUNK_OK;
	csql_ack(data->db, chunk_code);
	return s;
}

// MARK: - New DB API 2.0 -

REALstring ConvertObjectToMemoryBlockString(REALobject obj) {
    // check if respond to StringValue first
    REALstring value = nullptr;
    if (REALGetPropValueString(obj, "StringValue", &value)) return value;
    
    static REALclassRef memblockRef = NULL;
    static REALclassRef pictureRef = NULL;
    if (!memblockRef) memblockRef = REALGetClassRef("MemoryBlock");
    if (!pictureRef ) pictureRef = REALGetClassRef("Picture");
    
    // MemoryBlock case
    REALmemoryBlock mb = NULL;
    if (REALObjectIsA(obj, memblockRef)) {
        mb = (REALmemoryBlock)obj;
    } else {
        // Picture case
        if (REALObjectIsA(obj, pictureRef)) {
            // load ToData function
            REALmemoryBlock (*toDataFunc)(REALobject,RBInteger,RBInteger) = NULL;
            toDataFunc = (REALmemoryBlock (*)(REALobject,RBInteger,RBInteger))REALLoadObjectMethod(obj, "ToData(format As Picture.Formats, quality As Integer) As MemoryBlock");
            RBInteger quality = -1; // QualityDefault
            RBInteger format = 150; // PNG
            mb = (toDataFunc) ? toDataFunc(obj, format, quality) : NULL;
        }
    }
    
    if (mb && REALGetPropValueString(mb, "StringValue", &value)) return value;
    return NULL;
}

void BindVariantObjectToVM(REALobject vm, int index, REALobject item) {
    RBInteger varType = GetVarType(item);
    if (varType == -1) { // was (!REALGetPropValueInteger(item, "Type", &varType)) {
        CubeSQLVMBindNull(vm, index);
        return;
    }
    
    // https://docs.xojo.com/VarType
    // https://docs.xojo.com/Variant
    
    switch (varType) {
        case 0: { // TypeNil
            CubeSQLVMBindNull(vm, index);
        } break;
            
        case 2: { // TypeInt32
            int32_t value = 0;
            if (!REALGetPropValueInt32(item, "Int32Value", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindInt(vm, index, (int)value);
        } break;
            
        case 3: { // TypeInt64
            RBInt64 value = 0;
            if (!REALGetPropValueInt64(item, "Int64Value", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindInt64(vm, index, (int64)value);
        } break;
            
        case 4: { // TypeSingle
            float value = 0.0;
            if (!REALGetPropValueSingle(item, "SingleValue", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindDouble(vm, index, (double)value);
        } break;
            
        case 5: { // TypeDouble
            double value = 0.0;
            if (!REALGetPropValueDouble(item, "DoubleValue", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindDouble(vm, index, (double)value);
        } break;
            
//        case 6: { // TypeCurrency
//            REALcurrency value = 0;
//            if (!REALGetPropValueCurrency(item, "CurrencyValue", &value)) CubeSQLVMBindNull(vm, index);
//            else CubeSQLVMBindInt64(vm, index, (int64)value);
//        } break;
            
        case 7:
        case 38: { // TypeDate and TypeDateTime
            REALobject value = nullptr;;
            if (!REALGetPropValueObject(item, "DateTimeValue", &value)) CubeSQLVMBindNull(vm, index);
            else {
                REALstring svalue = nullptr;;
                if (!REALGetPropValueString(value, "SQLDateTime", &svalue)) CubeSQLVMBindNull(vm, index);
                else CubeSQLVMBindText(vm, index, svalue);
            }
        } break;
            
        case 6: // TypeCurrency
        case 8:
        case 18:
        case 19:
        case 20:
        case 21:
        case 37: { // TypeString, CString, WString, PString, CFStringRef and TypeText
            REALstring value = nullptr;;
            if (!REALGetPropValueString(item, "StringValue", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindText(vm, index, value);
        } break;
            
        case 9: { // TypeObject
            // https://docs.xojo.com/MemoryBlock.StringValue
            // https://forum.xojo.com/25966-realcallfunctionwithexceptionhandler-example/0#p214758
            // file:///Users/marco/GitHub/CubeSQLPlugin/Xojo_SDK/Documentation/Plugin%20SDK%20Dynamic%20Access%20Reference.html
            // file:///Users/marco/GitHub/CubeSQLPlugin/Xojo_SDK/Documentation/Plugin%20SDK%20String%20Reference.html
            
            // try to convert object to a buffer
            REALstring svalue = ConvertObjectToMemoryBlockString(item);
            if (svalue) CubeSQLVMBindBlob(vm, index, svalue);
            else CubeSQLVMBindNull(vm, index);
        } break;
            
        case 11:  { // TypeBoolean
            bool value = false;
            if (!REALGetPropValueBoolean(item, "BooleanValue", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindInt(vm, index, value ? 1 : 0);
        } break;
            
        case 16:  { // TypeColor
            RBColor value = 0;
            if (!REALGetPropValueColor(item, "ColorValue", &value)) CubeSQLVMBindNull(vm, index);
            else CubeSQLVMBindInt(vm, index, (int)value);
        } break;
            
//            case 18: {
//                // TypeCString
//                const char *value;
//                if (!REALGetPropValueCString(item, "CStringValue", &value)) CubeSQLVMBindNull(vm, index);
//                else {
//                    ClassData(CubeSQLVMClass, vm, cubeSQLVM, data);
//                    int len = (int)strlen(value);
//                    cubesql_vmbind_text(data->vm, index, (char *)value, len);
//                }
//            } break;
//
//            case 19: {
//                // TypeWString
//                const wchar_t *value;
//                if (!REALGetPropValueWString(item, "WStringValue", &value)) CubeSQLVMBindNull(vm, index);
//                else {
//                    ClassData(CubeSQLVMClass, vm, cubeSQLVM, data);
//                    int len = (int)sizeof(value);
//                    cubesql_vmbind_text(data->vm, index, (char *)value, len);
//                }
//            } break;
//
//            case 20: {
//                // TypePString (used only with Mac OS 9)
//                CubeSQLVMBindNull(vm, index);
//                #if 0
//                const unsigned char *value;
//                if (!REALGetPropValuePString(item, "PStringValue", &value)) CubeSQLVMBindNull(vm, index);
//                else {
//                    ClassData(CubeSQLVMClass, vm, cubeSQLVM, data);
//                    int len = (int)strlen(value);
//                    cubesql_vmbind_text(data->vm, index, (char *)value, len);
//                }
//                #endif
//            } break;
//
//            case 21: {
//                // TypeCString
//                #if TARGET_CARBON || TARGET_COCOA
//                CFStringRef value;
//                if (!REALGetPropValueCFStringRef(item, "CFStringRefValue", &value)) CubeSQLVMBindNull(vm, index);
//                else {
//                    ClassData(CubeSQLVMClass, vm, cubeSQLVM, data);
//                    int len = (int)CFStringGetLength(value);
//                    const char *s = CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
//                    cubesql_vmbind_text(data->vm, index, (char *)s, len);
//                }
//                #endif
//            } break;
        
        case 22:
        case 23:
        case 26:
        case 36: {
            // TypeWindowPtr
            // TypeOSType
            // TypePtr
            // TypeStructure
            CubeSQLVMBindNull(vm, index);
        } break;
            
//            case 37: {
//                // TypeText
//                REALtext value;
//                if (!REALGetPropValueText(item, "TextValue", &value)) CubeSQLVMBindNull(vm, index);
//                else {
//                    ClassData(CubeSQLVMClass, vm, cubeSQLVM, data);
//                    REALtextData *textData = REALGetTextData(value, "utf-8", false);
//                    cubesql_vmbind_text(data->vm, index, (char *)textData->data, (int)textData->size);
//                    REALDisposeTextData(textData);
//                }
//
//            } break;
            
        default: {
            // TypeArray (4096, logically OR'ed with the element type)
            CubeSQLVMBindNull(vm, index);
        } break;
    }
}

void BindVariantArrayToVM(REALobject vm, REALarray params) {
    RBInteger count = REALGetArrayUBound(params);
    
    // loop REALarray
    for (RBInteger i=0; i<=count; ++i) {
        REALobject item = nullptr;
        REALGetArrayValueObject(params, i, &item);
        BindVariantObjectToVM(vm, (int)(i+1), item);
    }
}

// Runs the SQL select statement with the supplied parameters
// sql -- the sql select statement to execute
// params -- variant array of parameters, can be empty or null
REALdbCursor DatabaseSelectSQL(dbDatabase *instance, REALstring sql, REALarray params) {
    DEBUG_WRITE("DatabaseSelect: %s", REALGetCString(sql));
    if (instance->isConnected == false) return NULL;
    instance->endChunkReceived = false;
    
    RBInteger count = REALGetArrayUBound(params);
    Boolean isParamsEmpty = (!params || count == -1);
    
    if (isParamsEmpty) {
        // simpler case without params
        csqlc *c = cubesql_select(instance->db, REALGetCString(sql));
        if (c == NULL) return NULL;
        return REALNewRowSetFromDBCursor(CursorCreate(c), &CubeSQLCursor);
    }
    
    // more complex case with params so a VM is required
    csqlvm *_vm = cubesql_vmprepare(instance->db, REALGetCString(sql));
    if (!_vm) return NULL;
    
    REALobject result = REALnewInstanceWithClass(REALGetClassRef("CubeSQLVM"));
    ClassData(CubeSQLVMClass, result, cubeSQLVM, vm);
    if (!vm || !_vm) return NULL;
    vm->vm = _vm;
    
    BindVariantArrayToVM(result, params);
    return CubeSQLVMSelectRowSet(result);
}

// Executes the SQL statement with the supplied parameters
// sql -- the sql statement to execute
// params -- variant array of parameters, can be empty
void DatabaseExecuteSQL(dbDatabase *instance, REALstring sql, REALarray params) {
    DEBUG_WRITE("DatabaseExecute: %s", REALGetCString(sql));
    if (instance->isConnected == false) return;
    instance->endChunkReceived = false;
    
    RBInteger count = REALGetArrayUBound(params); // COUNT ARRAY
    Boolean isParamsEmpty = (params == nil || count == -1);
    
    if (isParamsEmpty) {
        cubesql_execute(instance->db, REALGetCString(sql));
        return;
    }
    
    // more complex case with params so a VM is required
    csqlvm *_vm = cubesql_vmprepare(instance->db, REALGetCString(sql));
    if (!_vm) return;
    
    REALobject result = REALnewInstanceWithClass(REALGetClassRef("CubeSQLVM"));
    ClassData(CubeSQLVMClass, result, cubeSQLVM, vm);
    if (!vm || !_vm) return;
    vm->vm = _vm;
    
    BindVariantArrayToVM(result, params);
    CubeSQLVMExecute(result);
}

void DatabaseCommit(dbDatabase *instance) {
    cubesql_commit(instance->db);
}

void DatabaseRollback(dbDatabase *instance) {
    cubesql_rollback(instance->db);
}

void BeginTransaction(dbDatabase *instance) {
    cubesql_begintransaction(instance->db);
}

// MARK: - Prepare Statement (DB API 2.0) -

REALobject DatabasePrepareStatement(dbDatabase *database, REALstring statement) {
    // https://docs.xojo.com/Database.Prepare
    // https://docs.xojo.com/PreparedSQLStatement
    
    DEBUG_WRITE("DatabasePrepareStatement");
    if (database->isConnected == false) return NULL;
    
    csqlvm *cvm = cubesql_vmprepare(database->db, REALGetCString(statement));
    if (cvm == NULL) return NULL;
    
    REALobject result = REALnewInstanceWithClass(REALGetClassRef("CubeSQLPreparedStatement"));
    ClassData(CubeSQLPrepareClass, result, cubeSQLPrepare, vm);
    vm->vm = cvm;
    
    // return a PreparedSQLStatement
    return result;
}

REALobject CubeSQLDatabasePrepare(REALdbDatabase dbObject, REALstring statement) {
    dbDatabase *db = (dbDatabase *)REALGetDBFromREALdbDatabase(dbObject);
    if (db->isConnected == false) return NULL;

    return DatabasePrepareStatement(db, statement);
}

void CubeSQLPrepareConstructor (REALobject instance) {
    DEBUG_WRITE("CubeSQLPrepareConstructor");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    memset((void *)data, 0, sizeof(cubeSQLPrepare));
}

void CubeSQLPrepareDestructor (REALobject instance) {
    DEBUG_WRITE("CubeSQLPrepareDestructor");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    cubesql_vmclose(data->vm);
}

void CubeSQLPrepareBindValue (REALobject instance, int index, REALobject value) {
    DEBUG_WRITE("CubeSQLPrepareBindValue");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    
    int type = CUBESQL_BIND_TEXT; // default CUBESQL_TEXT
    if (index < MAX_TYPES_COUNT && data->types[index] != 0) type = data->types[index];
    CubeSQLPrepareBindValueType(instance, index, value, type);
}

void CubeSQLPrepareBindValueType (REALobject instance, int index, REALobject object, int type) {
    DEBUG_WRITE("CubeSQLPrepareBindValueType");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    
    csqlvm *vm = data->vm;
    if (!vm) return;
    
    /*
     {"CUBESQL_INTEGER = 1", NULL, 0},
     {"CUBESQL_DOUBLE = 2", NULL, 0},
     {"CUBESQL_TEXT = 3", NULL, 0},
     {"CUBESQL_BLOB = 4", NULL, 0},
     {"CUBESQL_NULL = 5", NULL, 0},
     {"CUBESQL_INT64 = 8", NULL, 0},
     {"CUBESQL_ZEROBLOB = 9", NULL, 0},
     */
    
    // sqlite3 on server side expects a 1-based index (while Xojo requires a 0-based index)
    ++index;
    
    switch (type) {
        case CUBESQL_BIND_INTEGER: { // CUBESQL_INTEGER
            int32_t value = 0;
            if (!REALGetPropValueInt32(object, "Int32Value", &value)) cubesql_vmbind_null(vm, index);
            else cubesql_vmbind_int(vm, index, (int)value);
        }
        break;
            
        case CUBESQL_BIND_DOUBLE: { // CUBESQL_DOUBLE
            double value = 0.0;
            if (!REALGetPropValueDouble(object, "DoubleValue", &value)) cubesql_vmbind_null(vm, index);
            else cubesql_vmbind_double(vm, index, (double)value);
        }
        break;
            
        case CUBESQL_BIND_BLOB: { // CUBESQL_BLOB
            REALstring svalue = ConvertObjectToMemoryBlockString(object);
            if (!svalue) cubesql_vmbind_null(vm, index);
            else {
                REALstringData sdata;
                if (!REALGetStringData(svalue, kREALTextEncodingUnknown, &sdata)) return;
                cubesql_vmbind_blob(vm, index, (void*)sdata.data, (int)sdata.length);
                REALDisposeStringData(&sdata);
            }
        }
        break;
        
        case CUBESQL_BIND_NULL: { // CUBESQL_NULL
            cubesql_vmbind_null(vm, index);
        }
        break;
        
        case CUBESQL_BIND_INT64: { // CUBESQL_INT64
            RBInt64 value = 0;
            if (!REALGetPropValueInt64(object, "Int64Value", &value)) cubesql_vmbind_null(vm, index);
            else cubesql_vmbind_int64(vm, index, (int64)value);
        }
        break;
            
        case CUBESQL_BIND_ZEROBLOB: { // CUBESQL_ZEROBLOB
            int32_t value = 0;
            if (!REALGetPropValueInt32(object, "Int32Value", &value)) cubesql_vmbind_null(vm, index);
            else cubesql_vmbind_zeroblob(vm, index, (int)value); // value is length
        }
        break;
            
        default: { // CUBESQL_TEXT
            REALstring value = nullptr;
            if (!REALGetPropValueString(object, "StringValue", &value)) cubesql_vmbind_null(vm, index);
            else {
                REALstringData sdata;
                if (!REALGetStringData(value, kREALTextEncodingUTF8, &sdata)) return;
                cubesql_vmbind_text(vm, index, (char *)sdata.data, (int)sdata.length);
                REALDisposeStringData(&sdata);
            }
        }
        break;
    }
}

void CubeSQLPrepareBindValues (REALobject instance, REALarray values) {
    DEBUG_WRITE("CubeSQLPrepareBindValues");
    if (values == nil) return;
    
    RBInteger count = REALGetArrayUBound(values);
    for (int i=0; i<=count; ++i) {
        if (i >= MAX_TYPES_COUNT) break;
        REALobject value = nullptr;
        REALGetArrayValueObject(values, i, &value);
        CubeSQLPrepareBindValue(instance, i, value);
    }
}

void CubeSQLPrepareBindType (REALobject instance, int index, int type) {
    DEBUG_WRITE("CubeSQLPrepareBindType");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    if (index < MAX_TYPES_COUNT) data->types[index] = type;
}

void CubeSQLPrepareBindTypes (REALobject instance, REALarray types) {
    DEBUG_WRITE("CubeSQLPrepareBindTypes");
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    
    RBInteger count = REALGetArrayUBound(types); // COUNT ARRAY
    Boolean isTypesEmpty = (types == nil || count == -1);
    if (!isTypesEmpty) {
        for (RBInteger i=0; i<=count; ++i) {
            if (i >= MAX_TYPES_COUNT) break;
            int32_t type = 0;
            REALGetArrayValueInt32(types, i, &type);
            data->types[i] = type;
        }
    }
}

void CubeSQLPrepareExecuteSQL (REALobject instance, REALarray values) {
    DEBUG_WRITE("CubeSQLPrepareExecuteSQL");
    if (values && (REALGetArrayUBound(values) >= 0)) CubeSQLPrepareBindValues(instance, values);
        
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    cubesql_vmexecute(data->vm);
}

void CubeSQLPrepareSQLExecute (REALobject instance, REALarray params) {
    DEBUG_WRITE("CubeSQLPrepareSQLExecute");
    CubeSQLPrepareExecuteSQL(instance, params);
}

REALdbCursor CubeSQLPrepareSelectSQL (REALobject instance, REALarray values) {
    DEBUG_WRITE("CubeSQLPrepareSelectSQL");
    if (values && (REALGetArrayUBound(values) >= 0)) CubeSQLPrepareBindValues(instance, values);
    
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    csqlc *c = cubesql_vmselect(data->vm);
    if (c == NULL) return NULL;
    return REALNewRowSetFromDBCursor(CursorCreate(c), &CubeSQLCursor);
}

REALdbCursor CubeSQLPrepareSQLSelect (REALobject instance, REALarray params) {
    DEBUG_WRITE("CubeSQLPrepareSQLSelect");
    if (params && (REALGetArrayUBound(params) >= 0)) CubeSQLPrepareBindValues(instance, params);
    
    ClassData(CubeSQLPrepareClass, instance, cubeSQLPrepare, data);
    csqlc *c = cubesql_vmselect(data->vm);
    if (c == NULL) return NULL;
    return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLCursor);
}

// MARK: - Cursor API -

dbCursor *CursorCreate(csqlc *c) {
	DEBUG_WRITE("CursorCreate");
	if (c == NULL) return NULL;
	
	dbCursor *cursor = (dbCursor*) malloc (sizeof(dbCursor));
	if (cursor == NULL) return NULL;
	memset((void *)cursor, 0,  sizeof(dbCursor));
	cursor->c = c;
	cursor->isLocked = false;
	cursor->firstRowCalled = false;
	return cursor;
}

void CursorDestroy(dbCursor* cursor) {
	DEBUG_WRITE("CursorDestroy");
	if (cursor == NULL) return;
	
	cubesql_cursor_free(cursor->c);
	free((void *) cursor);
}

int CursorColumnCount(dbCursor *cursor) {
	DEBUG_WRITE("CursorColumnCount");
	return cubesql_cursor_numcolumns(cursor->c);
}

int CursorRowCount(dbCursor *cursor) {
	DEBUG_WRITE("CursorRowCount");
	return cubesql_cursor_numrows(cursor->c);
}

REALstring CursorColumnName(dbCursor *cursor, int column) {
	DEBUG_WRITE("CursorColumnName");
	int  len = 0;
	char *s = cubesql_cursor_field(cursor->c, CUBESQL_COLNAME, column+1, &len);
	
	REALstring r;
	if ((len == 0) || (s == NULL))
		r = REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	else
		r = REALBuildStringWithEncoding(s, len, kREALTextEncodingUTF8);
	return r;
}

int CursorColumnType(dbCursor *cursor, int index) {
	DEBUG_WRITE("CursorColumnType");
	int t = cubesql_cursor_columntype(cursor->c, index+1);
    switch (t) {
        case CUBESQL_Type_Integer: return dbTypeInt64;
        case CUBESQL_Type_Float: return dbTypeDouble;
        case CUBESQL_Type_Text: return dbTypeText;
        case CUBESQL_Type_Blob: return dbTypeBinary;
        case CUBESQL_Type_Boolean: return dbTypeBoolean;
        case CUBESQL_Type_Date: return dbTypeDate;
        case CUBESQL_Type_Time: return dbTypeTime;
        case CUBESQL_Type_Timestamp: return dbTypeTimeStamp;
        case CUBESQL_Type_Currency: return dbTypeCurrency;
    }
	return dbTypeText;
}

void CursorColumnValue(dbCursor *cursor, int column, void **value, unsigned char *type, int *length) {
	static REALstring strVal = NULL;
	static char sBooleanValueChar;
	int vlen, coltype;
	char *v;
	
	column++;
	cursor->firstRowCalled = true;
	DEBUG_WRITE("CursorColumnValue column %d", column);
	
	vlen = 0;
	v = cubesql_cursor_field (cursor->c, CUBESQL_CURROW, column, &vlen);
	coltype = cubesql_cursor_columntype(cursor->c, column);
	
	// special NULL case (when vlen = -1 that means it's truly NULL and not just an empty string)
	if (vlen == -1) {
		*type = dbTypeNull;
		*length = 0;
		*value = NULL;
		return;
	}
	
	// workaround for a REALbasic bug
	// don't return dbTypeNull but an empty string
	// added a safeNULL flag to turn this workaround ON or OFF
	if ((v == NULL) || (vlen <= 0)) {
		if (nullAsString) {
			if (strVal != NULL) REALUnlockString(strVal);
			strVal = REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
			*type = dbTypeREALstring;
			*length = sizeof(REALstring);
			*value = (Ptr) &strVal;
		} else {
			*type = dbTypeNull;
			*length = 0;
			*value = NULL;
		}
		return;
	}
	 
	// special BLOB case
	if ((blobAsString == false) && (coltype == CUBESQL_Type_Blob)) {
		if (strVal != NULL) REALUnlockString(strVal);
		strVal = REALBuildStringWithEncoding(v, vlen, kREALTextEncodingUnknown);
		*type = dbTypeBinary;
		*length = sizeof(REALstring);
		*value = (Ptr) &strVal;
		return;
	}
	
	// special BOOLEAN case
	if (coltype == CUBESQL_Type_Boolean) {
		sBooleanValueChar = false;
		if ((v[0] == '1') || (v[0] == 't')  || (v[0] == 'T')) sBooleanValueChar = true;
		*type = dbTypeBoolean;
		*value = (Ptr) &sBooleanValueChar;
		*length = 1;
		return;
	}
	
	// everything else is a string
	if (strVal != NULL) REALUnlockString(strVal);
	strVal = REALBuildStringWithEncoding(v, vlen, kREALTextEncodingUTF8);
	*type = dbTypeREALstring;
	*length = sizeof(REALstring);
	*value = (Ptr) &strVal;
	
	return;
}

void CursorFieldSchemaColumnValue(dbCursor *cursor, int column, void **value, unsigned char *type, int *length) {
	DEBUG_WRITE("CursorFieldSchemaColumnValue");
	CursorColumnValue(cursor, column, value, type, length);
}

void CursorDelete(dbCursor *cursor) {
	DEBUG_WRITE("CursorDelete");
    
	int64 rowid = cubesql_cursor_rowid(cursor->c, CUBESQL_CURROW);
	if (rowid == 0) return;
    
	char *table = cubesql_cursor_field(cursor->c, CUBESQL_COLTABLE, 1, NULL);
	if (table == NULL) return;
	
	// cursor is editable
	char sql[1024];
	snprintf(sql, sizeof(sql), "DELETE FROM [%s] WHERE rowid=%lld;", table, rowid);
	cubesql_execute(cubesql_cursor_db(cursor->c), sql);
	
	CursorCheckClearLock(cursor);
}

void CursorUpdate(dbCursor *cursor, REALcursorUpdate *updates) {
	DEBUG_WRITE("CursorUpdate");
	int64 rowid = cubesql_cursor_rowid(cursor->c, CUBESQL_CURROW);
	if (rowid == 0) return;
    
	char *table = cubesql_cursor_field(cursor->c, CUBESQL_COLTABLE, 1, NULL);
	if (table == NULL) return;
	
	// cursor is editable
	// first build the list of columns names for bindings
	std::string 	cols = "";
	int				ncols=0, colindex, index = 1;
	char			*colname;
	for (REALcursorUpdate *update = updates; update; update = update->next) {
		colindex = update->fieldIndex;
		colindex++; // because cubesql functions are all 1 based
		colname = cubesql_cursor_field(cursor->c, CUBESQL_COLNAME, colindex, NULL);
		if (colname == NULL) return;
		stringstream temp;
		temp<<index;
		cols += colname; cols += "=?"; cols += temp.str();
		if (update->next) cols += ", ";
		index++;
		ncols++;
	}
	
	// build sql bind string
	stringstream temp;
	temp<<rowid;
	std::string sql = "UPDATE ";
	sql += table;
	sql += " SET ";
	sql += cols;
	sql += " WHERE rowid=";
	sql += temp.str();
	sql += ";";
	DEBUG_WRITE("Bind string is %s", sql.c_str());
	
	// build arrays needed by the cubesql_bind function
	char	**colvalue = NULL;
	int		*colsize = NULL;
	int		*coltype = NULL;
	int		bindtype;
	char	*realvalue;
	
	colvalue = (char **) malloc(sizeof(char*)*ncols);
	colsize = (int *) malloc(sizeof(int)*ncols);
	coltype = (int *) malloc(sizeof(int)*ncols);
	if ((colvalue == NULL) || (colsize == NULL) || (coltype == NULL)) return;
	
	index = 0;
	for (REALcursorUpdate *update = updates; update; update = update->next) {
		realvalue = (char *)REALGetCString(update->columnValue);
		bindtype = cubesql_cursor_columntypebind(cursor->c, (update->fieldIndex+1));
		if ((cubesql_cursor_columntype(cursor->c, (update->fieldIndex+1)) == CUBESQL_Type_Boolean) && (booleanAsInteger)) {
			realvalue = REALbasicBoolean2Integer(realvalue);
			bindtype = CUBESQL_BIND_INTEGER;
		}
        
        int vlength = (int)REALStringLength(update->columnValue);
		colvalue[index] = realvalue;
        colsize[index] = vlength;
        // sanity check for NULL values
        if ((realvalue == NULL) || (vlength == 0)) bindtype = CUBESQL_BIND_NULL;
		coltype[index] = bindtype;
        
		index++;
	}
	
	cubesql_bind (cubesql_cursor_db(cursor->c), sql.c_str(), colvalue, colsize, coltype, ncols);
	
	free((void *)colvalue);
	free((void *)colsize);
	free((void *)coltype);
	
	CursorCheckClearLock(cursor);
}

RBBoolean CursorNextRow(dbCursor *cursor) {
	DEBUG_WRITE("CursorNextRow");
	if (cubesql_cursor_numrows(cursor->c) == 0)
		return false;
		
	CursorCheckClearLock(cursor);
	
	// workaround for a REALStudio issue
	// basically in a for loop, rs.MoveFirst is not called
	// but there is only a rs.MoveNext
	if (cursor->firstRowCalled == false) {
		CursorFirstRow(cursor);
		return true;
	}
	
	if (cubesql_cursor_seek(cursor->c, CUBESQL_SEEKNEXT) == kTRUE)
		return true;
	return false;
}

void CursorPrevRow(dbCursor *cursor) {
	DEBUG_WRITE("CursorPrevRow");
	CursorCheckClearLock(cursor);
	
	cubesql_cursor_seek(cursor->c, CUBESQL_SEEKPREV);
}

void CursorFirstRow(dbCursor *cursor) {
	DEBUG_WRITE("CursorFirstRow");
	CursorCheckClearLock(cursor);
	
	cursor->firstRowCalled = true;
	cubesql_cursor_seek(cursor->c, CUBESQL_SEEKFIRST);
}

void CursorLastRow(dbCursor *cursor) {
	DEBUG_WRITE("CursorLastRow");
	CursorCheckClearLock(cursor);
	
	cubesql_cursor_seek(cursor->c, CUBESQL_SEEKLAST);
}

void CursorClose(dbCursor *cursor) {
	DEBUG_WRITE("CursorClose");
	CursorCheckClearLock(cursor);
	CursorDestroy(cursor);
}

void CursorEdit(dbCursor *cursor) {
	DEBUG_WRITE("CursorEdit");
	CursorCheckClearLock(cursor);
	
	int64 rowid = cubesql_cursor_rowid(cursor->c, CUBESQL_CURROW);
	if (rowid == 0) return;
	char *table = cubesql_cursor_field(cursor->c, CUBESQL_COLTABLE, 1, NULL);
	if (table == NULL) return;
	
	// cursor is editable
	char sql[1024];
	snprintf(sql, sizeof(sql), "LOCK RECORD %lld ON TABLE %s;", rowid, table);
	if (cubesql_execute(cubesql_cursor_db(cursor->c), sql) == CUBESQL_NOERR)
		cursor->isLocked = true;
}

void CursorCheckClearLock(dbCursor *cursor) {
	DEBUG_WRITE("CursorCheckClearLock");
	if (cursor->isLocked == false) return;
	
	int64 rowid = cubesql_cursor_rowid(cursor->c, CUBESQL_CURROW);
	if (rowid == 0) return;
	char *table = cubesql_cursor_field(cursor->c, CUBESQL_COLTABLE, 1, NULL);
	if (table == NULL) return;
	
	// cursor is editable
	char sql[1024];
	snprintf(sql, sizeof(sql), "UNLOCK RECORD %lld ON TABLE %s;", rowid, table);
	if (cubesql_execute(cubesql_cursor_db(cursor->c), sql) == CUBESQL_NOERR)
		cursor->isLocked = false;
}

Boolean CursorGoToRow(REALobject instance, REALdbCursor rs, int index) {
	dbCursor *cursor = NULL;
	
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return false;
	
	cursor = REALGetCursorFromREALdbCursor(rs);
	if (cursor == NULL) return false;
	if (cursor->c == NULL) return false;
	
	if (cubesql_cursor_seek(cursor->c, index) == kTRUE) return true;
	return false;
}

REALstring CursorTableName(REALobject instance, REALdbCursor rs) {
	dbCursor *cursor = NULL;
	char *s = NULL, *tname = NULL;
	int i = 0, len = 0, ncols = 0;
	
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) goto emptyResult;
	
	cursor = REALGetCursorFromREALdbCursor(rs);
	if (cursor == NULL) goto emptyResult;
	
	ncols = cubesql_cursor_numcolumns(cursor->c);
	for (i=1; i<=ncols; i++) {
		s = cubesql_cursor_field(cursor->c, CUBESQL_COLTABLE, i, &len);
		if (tname == NULL) tname = s;
		else if (strcmp(tname, s) != 0) goto emptyResult;
	}
	
	if (tname == NULL) goto emptyResult;
	return REALBuildStringWithEncoding(tname, len, kREALTextEncodingUTF8);

emptyResult:
	return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
}

// MARK: - VM API -

REALobject DatabasePrepare (REALobject instance, REALstring sql) {
	REALobject result = NULL;
	csqlvm *cvm = NULL;
	
	DEBUG_WRITE("DatabasePrepare");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	if (data == NULL) return NULL;
	if (data->isConnected == false) return NULL;
	
	cvm = cubesql_vmprepare(data->db, REALGetCString(sql));
	if (cvm == NULL) return NULL;
	
	result = REALnewInstanceWithClass(REALGetClassRef("CubeSQLVM"));
	ClassData(CubeSQLVMClass, result, cubeSQLVM, vm);
	vm->vm = cvm;
	
	return result;
}

void CubeSQLVMConstructor (REALobject instance) {
	DEBUG_WRITE("CubeSQLVMConstructor");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	memset((void *)data, 0, sizeof(cubeSQLVM));
}

void CubeSQLVMDestructor (REALobject instance) {
	DEBUG_WRITE("CubeSQLVMDestructor");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmclose(data->vm);
}

void CubeSQLVMBindBlob (REALobject instance, int index, REALstring blob) {
	DEBUG_WRITE("SQLite3VMBindBlob");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	
	REALstringData sdata;
	if (!REALGetStringData(blob, kREALTextEncodingUnknown, &sdata)) return;
	cubesql_vmbind_blob(data->vm, index, (void*)sdata.data, (int)sdata.length);
    REALDisposeStringData(&sdata);
}

void CubeSQLVMBindDouble (REALobject instance, int index, double n) {
	DEBUG_WRITE("CubeSQLVMBindDouble");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmbind_double(data->vm, index, n);
}

void CubeSQLVMBindInt (REALobject instance, int index, int n) {
	DEBUG_WRITE("CubeSQLVMBindInt");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmbind_int(data->vm, index, n);
}

void CubeSQLVMBindInt64 (REALobject instance, int index, int64 n) {
	DEBUG_WRITE("CubeSQLVMBindInt64");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmbind_int64(data->vm, index, n);
}

void CubeSQLVMBindNull (REALobject instance, int index) {
	DEBUG_WRITE("CubeSQLVMBindNull");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmbind_null(data->vm, index);
}

void CubeSQLVMBindZeroBlob (REALobject instance, int index, int len) {
	DEBUG_WRITE("CubeSQLVMBindZeroBlob");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmbind_zeroblob(data->vm, index, len);
}

void CubeSQLVMBindText (REALobject instance, int index, REALstring str) {
	DEBUG_WRITE("CubeSQLVMBindText");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	
	REALstringData sdata;
	if (!REALGetStringData(str, kREALTextEncodingUTF8, &sdata)) return;
	cubesql_vmbind_text(data->vm, index, (char *)sdata.data, (int)sdata.length);
    REALDisposeStringData(&sdata);
}

void CubeSQLVMExecute (REALobject instance) {
	DEBUG_WRITE("CubeSQLVMExecute");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	cubesql_vmexecute(data->vm);
}

REALdbCursor CubeSQLVMSelect (REALobject instance) {
	DEBUG_WRITE("CubeSQLVMSelect");
	ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
	csqlc *c = cubesql_vmselect(data->vm);
	if (c == NULL) return NULL;
	return REALdbCursorFromDBCursor(CursorCreate(c), &CubeSQLCursor);
}

REALdbCursor CubeSQLVMSelectRowSet (REALobject instance) {
    DEBUG_WRITE("CubeSQLVMSelectRowSet");
    ClassData(CubeSQLVMClass, instance, cubeSQLVM, data);
    csqlc *c = cubesql_vmselect(data->vm);
    if (c == NULL) return NULL;
    return REALNewRowSetFromDBCursor(CursorCreate(c), &CubeSQLCursor);
}

// MARK: - Properties -

REALstring ServerVersionGetter(REALobject instance, long param) {
	DEBUG_WRITE("ServerVersionGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	if (data->isConnected == false) return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	
	csqlc *c = cubesql_select(data->db, "SHOW PREFERENCE SERVER_RELEASE;");
	if (c == NULL) return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
	char *p, s[128];
	p = cubesql_cursor_cstring_static(c, 1, 2, s, sizeof(s));
	if (p != NULL) {
		REALstring r = REALBuildStringWithEncoding(p, (int)strlen(p), kREALTextEncodingUTF8);
		cubesql_cursor_free(c);
		return r;
	}
	
	cubesql_cursor_free(c);
	return REALBuildStringWithEncoding("", 0, kREALTextEncodingUTF8);
}

int PortGetter(REALobject instance, long param) {
	DEBUG_WRITE("PortGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return -1;
	
	return data->port;
}

void PortSetter(REALobject instance, long param, int value) {
	DEBUG_WRITE("PortSetter %d", value);
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	data->port = value;
}

int TimeoutGetter(REALobject instance, long param) {
	DEBUG_WRITE("TimeoutGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return -1;
	
	return data->timeout;
}

void TimeoutSetter(REALobject instance, long param, int value) {
	DEBUG_WRITE("TimeoutSetter %d", value);
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	if (value < 0) return;
	
	data->timeout = value;
	if (data->db) data->db->timeout = value;
}

Boolean AutoCommitGetter(REALobject instance, long param) {
	DEBUG_WRITE("AutoCommitGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return false;
	
	return data->autoCommit;
}

void AutoCommitSetter(REALobject instance, long param, Boolean value) {
	DEBUG_WRITE("AutoCommitSetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	if (data->autoCommit == value) return;
	
	// send command to the server
	if (value == true)
		cubesql_execute(data->db, "SET AUTOCOMMIT TO ON;");
	else
		cubesql_execute(data->db, "SET AUTOCOMMIT TO OFF;");
	
	data->autoCommit = value;
}

int EncryptionGetter(REALobject instance, long param) {
	DEBUG_WRITE("EncryptionGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return -1;
	
	return data->encryption;
}

void EncryptionSetter(REALobject instance, long param, int value) {
	DEBUG_WRITE("AutoCommitSetter %d", value);
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	data->encryption = value;
}

void DebugFileSetter(REALobject instance, long param, REALfolderItem value) {
	DEBUG_WRITE("DebugFileSetter");
	
	if ((value == NULL) && (debugFile == NULL))
		return;
	
	if (debugFile != NULL) {
		debug_fileclose();
		if (value == NULL) return;
	}
	
	debug_fileopen(value);
}



int PingFrequencyGetter(REALobject instance, long param) {
	DEBUG_WRITE("PingFrequencyGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return -1;
	
	return data->pingFrequency;
}

void DoPing (void *data) {
	// check time
	// struct timeval t;
	// gettimeofday(&t, NULL);
	// printf("%d\n", t.tv_sec);
	
	DEBUG_WRITE("DoPing");
	dbDatabase *database = (dbDatabase *)data;
	if (database == NULL) return;
	if (database->isConnected == false) return;
	
	// execute PING command
	if (cubesql_execute(database->db, "PING;") != CUBESQL_NOERR)
		database->isConnected = false;
	else
		database->isConnected = true;
}

void PingFrequencySetter(REALobject instance, long param, int value) {
	DEBUG_WRITE("PingFrequencySetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	if (value < 0) value = 0;
	
	if (data->isConnected == false) {
		data->pingFrequency = value;
		return;
	}
	
	// send command to the server
	if ((value >= 0) && (value != data->pingFrequency)) {
		char cmd[512];
		snprintf(cmd, sizeof(cmd), "SET PING TIMEOUT TO %d;", value);
		cubesql_execute(data->db, cmd);
	}
		
	// set property
	data->pingFrequency = value;
	
	PingTimerStop(data);
	if (value > 0) PingTimerStart(data);
}

void PingTimerStart(dbDatabase *database) {
	// TIMER is in ticks
	// time in ticks - 40%
	int ticks = database->pingFrequency * 60;
	ticks = ticks - (int)(ticks * 0.4);
	if (ticks == 0) return;
	
	DEBUG_WRITE("PingTimerStart ticks %d", ticks);
	database->timerID = REALRegisterBackgroundTask(DoPing, ticks, (void *)database);
}

void PingTimerStop(dbDatabase *database) {
	DEBUG_WRITE("PingTimerStop");
	if (database->timerID == 0) return;
	REALUnregisterBackgroundTask((int32_t)database->timerID);
	database->timerID = 0;
}

Boolean EndChunkGetter(REALobject instance, long param) {
	DEBUG_WRITE("EndChunkGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return false;
	
	return data->endChunkReceived;
}

Boolean UseREALServerProtocolGetter(REALobject instance, long param) {
	DEBUG_WRITE("UseREALServerProtocolGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return false;
	return data->useREALServerProtocol;
}

void UseREALServerProtocolSetter(REALobject instance, long param, Boolean value) {
	DEBUG_WRITE("UseREALServerProtocolSetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	data->useREALServerProtocol = value;
}

void SSLCertificateSetter(REALobject instance, long param, REALfolderItem value) {
	DEBUG_WRITE("SSLCertificateSetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	if (data->sslCertificate) REALUnlockObject((REALobject)data->sslCertificate);
	REALLockObject((REALobject)value);
	data->sslCertificate = value;
}

void SSLRootCertificateSetter(REALobject instance, long param, REALfolderItem value) {
	DEBUG_WRITE("SSLRootCertificateSetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	if (data->rootCertificate) REALUnlockObject((REALobject)data->rootCertificate);
	REALLockObject((REALobject)value);
	data->rootCertificate = value;
}

void TokenSetter(REALobject instance, long param, REALstring value) {
	DEBUG_WRITE("TokenSetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	if (data->token) REALUnlockString(data->token);
	REALLockString(value);
	data->token = value;
}

REALstring TokenGetter(REALobject instance, long param) {
	DEBUG_WRITE("TokenGetter");
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return nil;
	return data->token;	
}

// MARK: - Plugin Module API -

REALstring PluginVersionGetter(void) {
	DEBUG_WRITE("PluginVersionGetter");
	REALstring r = REALBuildStringWithEncoding(PLUGIN_VERSION, strlen(PLUGIN_VERSION), kREALTextEncodingUTF8);
	return r;
}

REALstring OpenSSLVersionGetter(void) {
	DEBUG_WRITE("OpenSSLVersionGetter");
	const char *version = cubesql_sslversion();
	if (version == NULL) version = SSL_NOVERSION;
		
	return REALBuildStringWithEncoding(version, (int)strlen(version), kREALTextEncodingUTF8);
}

int OpenSSLVersionNumGetter(void) {
    DEBUG_WRITE("OpenSSLVersionNumGetter");
    return (int)cubesql_sslversion_num();
}

Boolean BLOBAsStringGetter(void) {
	DEBUG_WRITE("BLOBAsStringGetter");
	return blobAsString;
}

void BLOBAsStringSetter(Boolean value) {
	DEBUG_WRITE("BLOBAsStringSetter");
	blobAsString = value;
}	

Boolean NULLAsStringGetter(void) {
	DEBUG_WRITE("NULLAsStringGetter");
	return nullAsString;
}

void NULLAsStringSetter(Boolean value) {
	DEBUG_WRITE("NULLAsStringSetter");
	nullAsString = value;
}

void SSLLibrarySetter(REALfolderItem value) {
	DEBUG_WRITE("SSLLibrarySetter");
	
	REALstring path = REALbasicPathFromFolderItem(value);
    if (!path) return;
	
    cubesql_setpath(CUBESQL_SSL_LIBRARY_PATH, (char*)REALGetCString(path));
	REALUnlockString(path);
}

void CryptoLibrarySetter(REALfolderItem value) {
	DEBUG_WRITE("SSLLibrarySetter");
	
	REALstring path = REALbasicPathFromFolderItem(value);
    if (!path) return;
    
	cubesql_setpath(CUBESQL_CRYPTO_LIBRARY_PATH, (char*)REALGetCString(path));
	REALUnlockString(path);
}

Boolean BooleanAsIntegerGetter(void) {
	DEBUG_WRITE("BooleanAsIntegerGetter");
	return booleanAsInteger;
}

void BooleanAsIntegerSetter(Boolean value) {
	DEBUG_WRITE("BooleanAsIntegerSetter");
	booleanAsInteger = value;
}

// MARK: - Utils -
typedef int (*vartype_callback) (void *);
int GetVarType (REALobject value) {
    static vartype_callback vartype = NULL;
    if (!vartype) vartype = (vartype_callback) REALLoadFrameworkMethod("VarType (value As Variant) As Integer");
    return (vartype) ? vartype(value) : -1;
}

csqlc *REALServerBuildFieldSchemaCursor (csqlc *pragmac) {
	// c is: cid  name  type  notnull  dflt_value  pk (6 columns)
	// we need only: name type pk notnull 0 (5 columns)
	DEBUG_WRITE("REALServerBuildFieldSchemaCursor");
	if (pragmac == NULL) return NULL;
	
	int		cols = 5;
	int		rows = cubesql_cursor_numrows(pragmac);
	int		types[] = {CUBESQL_Type_Text, CUBESQL_Type_Integer, CUBESQL_Type_Boolean, CUBESQL_Type_Boolean, CUBESQL_Type_Integer};
	const char	*names[] = {"ColumnName", "FieldType", "IsPrimary", "NotNull", "Length"};
	char	*row[5];
	int		len[5];
	
	// create a new cursor
	csqlc *c = cubesql_cursor_create(cubesql_cursor_db(pragmac), rows, cols, types, (char **)names);
	if (c == NULL) goto return_result;
	
	// iterate over old cursor and fill-in the new one
	while (cubesql_cursor_iseof(pragmac) != kTRUE) {
		row[0] = cubesql_cursor_field(pragmac, CUBESQL_CURROW, 2, &len[0]);
		row[1] = cubesql_cursor_field(pragmac, CUBESQL_CURROW, 3, &len[1]);
		row[2] = cubesql_cursor_field(pragmac, CUBESQL_CURROW, 6, &len[2]);
		row[3] = cubesql_cursor_field(pragmac, CUBESQL_CURROW, 4, &len[3]);
		row[4] = (char *)"0"; len[4] = 1;
		cubesql_cursor_addrow(c, row, len);
		cubesql_cursor_seek(pragmac, CUBESQL_SEEKNEXT);
	}
	
return_result:
	cubesql_cursor_free(pragmac);
	return c;
}

char *REALbasicBoolean2Integer (char *realvalue) {
	// FIX for BOOLEAN fields passed as Strings
	if ((realvalue) && (realvalue[0])) {
		if (realvalue[0] == 't') return (char*)"1";
		if (realvalue[0] == 'T') return (char*)"1";
		if (realvalue[0] == '1') return (char*)"1";
	}
	return (char*)"0";
}

int REALbasic2CubeSQLColumnType (int rbtype) {
	if ((rbtype == dbTypeBoolean) && (booleanAsInteger == false))
		return CUBESQL_BIND_TEXT;
	
	if (rbtype == dbTypeNull)
		return CUBESQL_BIND_NULL;
	
	if ((rbtype == dbTypeFloat) || (rbtype == dbTypeDouble) || (rbtype == dbTypeDecimal))
		return CUBESQL_BIND_DOUBLE;
	
	if ((rbtype == dbTypeInt64) || (rbtype == dbTypeByte) ||
		(rbtype == dbTypeShort) || (rbtype == dbTypeLong) || (rbtype == dbTypeBoolean))
		return CUBESQL_BIND_INTEGER;
	
	if ((rbtype == dbTypeBinary) || (rbtype == dbTypeLongText) ||
		(rbtype == dbTypeLongBinary) || (rbtype == dbTypeMacPICT))
		return CUBESQL_BIND_BLOB;
	
	return CUBESQL_BIND_TEXT; // default
}

REALstring CheckFixEscapedStringPath (REALstring s) {
	const char *path = REALGetCString(s);
	int len = (int)strlen(path);
	int flag = 0;
	
	// check if the path has been escaped
	for (int i=0; i<len; i++) if (path[i]=='\\') ++flag;
	
	// return original path because it was not escaped
	if (flag == 0) return s;
	
	// build fixed path
	char *fixed_path = (char *)malloc(len+1);
	if (fixed_path == NULL) return s; // not return NULL in order to avoid a memory leak
	
	memset((void *)fixed_path, 0, len+1);
	for (int i=0, j=0; i<len; i++) {
		if (i+1<len) if ((path[i] == '\\') && (path[i+1] == '\\')) fixed_path[j++] = '\\';
		if (path[i] == '\\') continue;
		fixed_path[j++] = path[i];
	}
	
	REALstring res = REALBuildStringWithEncoding(fixed_path, (int)strlen(fixed_path), kREALTextEncodingUTF8);
	if (res == NULL) return s;
	free((void *)fixed_path);
	
	REALUnlockString(s);
	return res;
}

REALstring REALbasicPathFromFolderItem (REALfolderItem value) {
	REALstring path = NULL;
	DEBUG_WRITE("REALbasicPathFromFolderItem");
    
    double v = REALGetRBVersion();
    if (v <= 2012.021) { // 2012R2.1 is the latest Real Studio edition
        #if WIN32
        if (REALGetPropValueString((REALobject)value, "AbsolutePath", &path) == false) return NULL;
        #else
        if (REALGetPropValueString((REALobject)value, "ShellPath", &path) == false) return NULL;
        path = CheckFixEscapedStringPath(path);
        #endif
    } else {
        // NativePath property is supported only on Xojo and it is the recommended way to get path from FolderItem
        if (REALGetPropValueString((REALobject)value, "NativePath", &path) == false) return NULL;
    }
	
	REALLockString(path);
	return path;
}

void debug_write (const char *format, ...) {
	va_list arg;
	
	if (format == NULL) return;
	
	va_start (arg, format);
	vfprintf(debugFile, format, arg);
	va_end (arg);
	fprintf(debugFile, "\n");
	fflush(debugFile);
}

void debug_fileopen (REALfolderItem value) {
	REALstring path = REALbasicPathFromFolderItem(value);
    if (!path) return;
	
	debugFile = fopen (REALGetCString(path), "ab+");
	if (debugFile == NULL) {
		printf("Unable to open debug file!");
		return;
	}
	
	// disable I/O buffer
	setbuf(debugFile, NULL);
}

void debug_fileclose (void) {
	if (debugFile != NULL) fclose(debugFile);
	debugFile = NULL;
}

void TraceEventSetter (REALobject instance, long param, Boolean value) {
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	if (data == NULL) return;
	
	if (value == false) {
		cubesql_set_trace_callback(data->db, NULL, NULL);
		return;
	}
	
	// set trace function
	data->traceEvent = REALGetEventInstance((REALcontrolInstance)instance, &CubeSQLEvents[0]);
	cubesql_set_trace_callback(data->db, rb_trace, (void *)instance);
}

void rb_trace (const char* sql, void *userData) {	
	if (!sql) return;
	REALstring s = REALBuildStringWithEncoding(sql, (int)strlen(sql), kREALTextEncodingUTF8);
	if (!s) return;
	
	REALobject instance = (REALobject) userData;
	ClassData(CubeSQLDatabaseClass, instance, dbDatabase, data);
	
	// invoke the event handler
	void (*fp)(REALobject instance, REALstring);
	fp = (void (*)(REALobject instance, REALstring)) data->traceEvent;
	if (fp) fp(instance, s);
	
	// free memory
	REALUnlockString(s);
}

#pragma mark -
void PluginEntry() {
	
	// register the database and cursor
	REALRegisterDBEngine(&CubeSQLEngine);
	REALRegisterDBCursor(&CubeSQLCursor);
	REALRegisterDBCursor(&CubeSQLFieldSchemaCursor);
	
	// register the CubeSQLDatabase class
	SetClassConsoleSafe(&CubeSQLDatabaseClass);
	REALRegisterClass(&CubeSQLDatabaseClass);
	
	// register the CubeSQLVM class
	SetClassConsoleSafe(&CubeSQLVMClass);
	REALRegisterClass(&CubeSQLVMClass);
    
    // register the CubeSQLVM class
    SetClassConsoleSafe(&CubeSQLPrepareClass);
    REALRegisterClass(&CubeSQLPrepareClass);
	
	REALRegisterModule(&CubeSQLModule);
}
