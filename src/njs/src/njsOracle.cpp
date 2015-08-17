/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved. */

/******************************************************************************
 *
 * You may not use the identified files except in compliance with the Apache
 * License, Version 2.0 (the "License.")
 *
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file uses NAN:
 *
 * Copyright (c) 2015 NAN contributors
 * 
 * NAN contributors listed at https://github.com/rvagg/nan#contributors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * 
 * NAME
 *   njsOracle.cpp
 *
 * DESCRIPTION
 *   Oracledb class implementation.
 *
 *****************************************************************************/

#include "node.h"

#include "njsOracle.h"
#include "njsConnection.h"
#include "njsPool.h"
#include "njsResultSet.h"
#include "njsMessages.h"
#include "njsIntLob.h"
                                        //peristent Oracledb class handle
Nan::Persistent<FunctionTemplate> Oracledb::oracledbTemplate_s;

#define NJS_MAX_ROWS            100
#define NJS_STMT_CACHE_SIZE      30
#define NJS_POOL_MIN              0
#define NJS_POOL_MAX              4
#define NJS_POOL_INCR             1
#define NJS_POOL_TIMEOUT         60
#define NJS_PREFETCH_ROWS       100
#define NJS_LOB_PREFETCH_SIZE 16384

/*****************************************************************************/
/*
   DESCRIPTION
     Constructor for the Oracledb class.
 */
Oracledb::Oracledb()
{
  dpienv_             = dpi::Env::createEnv();
  outFormat_          = ROWS_ARRAY;
  maxRows_            = NJS_MAX_ROWS;
  autoCommit_         = false;
  stmtCacheSize_      = NJS_STMT_CACHE_SIZE;
  poolMax_            = NJS_POOL_MAX;
  poolMin_            = NJS_POOL_MIN;
  poolIncrement_      = NJS_POOL_INCR;
  poolTimeout_        = NJS_POOL_TIMEOUT;
  prefetchRows_       = NJS_PREFETCH_ROWS;
  connClass_          = "";
  externalAuth_       = false;
  fetchAsStringTypes_ = NULL;
  lobPrefetchSize_ = NJS_LOB_PREFETCH_SIZE;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Destructor for the Oracledb class.
 */
Oracledb::~Oracledb()
{
  if ( fetchAsStringTypes_ )
  {
    free ( fetchAsStringTypes_ );
  }

  if (this->dpienv_)
  {
    dpienv_->terminate();
  }

  //NanDisposePersistent(jsOracledb);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Init function of the Oracledb class.
     Initiates and maps the functions and properties of Oracledb class.
*/
void Oracledb::Init(Handle<Object> target)
{
  Nan::HandleScope scope;

  Local<FunctionTemplate> temp = Nan::New<FunctionTemplate>(New);
  temp->InstanceTemplate()->SetInternalFieldCount(1);
  temp->SetClassName(Nan::New<v8::String>("Oracledb").ToLocalChecked());

  Nan::SetPrototypeMethod(temp, "getConnection", GetConnection);
  Nan::SetPrototypeMethod(temp, "createPool", CreatePool);

  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("poolMax").ToLocalChecked(),
    Oracledb::GetPoolMax,
    Oracledb::SetPoolMax );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("poolMin").ToLocalChecked(),
    Oracledb::GetPoolMin,
    Oracledb::SetPoolMin );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("poolIncrement").ToLocalChecked(),
    Oracledb::GetPoolIncrement,
    Oracledb::SetPoolIncrement );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("poolTimeout").ToLocalChecked(),
    Oracledb::GetPoolTimeout,
    Oracledb::SetPoolTimeout );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("stmtCacheSize").ToLocalChecked(),
    Oracledb::GetStmtCacheSize,
    Oracledb::SetStmtCacheSize );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("prefetchRows").ToLocalChecked(),
    Oracledb::GetPrefetchRows,
    Oracledb::SetPrefetchRows );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("autoCommit").ToLocalChecked(),
    Oracledb::GetAutoCommit,
    Oracledb::SetAutoCommit );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("maxRows").ToLocalChecked(),
    Oracledb::GetMaxRows,
    Oracledb::SetMaxRows );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("outFormat").ToLocalChecked(),
    Oracledb::GetOutFormat,
    Oracledb::SetOutFormat );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("version").ToLocalChecked(),
    Oracledb::GetVersion,
    Oracledb::SetVersion );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("connectionClass").ToLocalChecked(),
    Oracledb::GetConnectionClass,
    Oracledb::SetConnectionClass );
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("externalAuth").ToLocalChecked(),
    Oracledb::GetExternalAuth,
    Oracledb::SetExternalAuth );
  Nan::SetAccessor(
    temp->InstanceTemplate(), 
    Nan::New<v8::String>("fetchAsString").ToLocalChecked(),
    Oracledb::GetFetchAsString,
    Oracledb::SetFetchAsString);
  Nan::SetAccessor(
    temp->InstanceTemplate(),
    Nan::New<v8::String>("lobPrefetchSize").ToLocalChecked(),
    Oracledb::GetLobPrefetchSize,
    Oracledb::SetLobPrefetchSize);

  oracledbTemplate_s.Reset(temp);
  Nan::Set(target, Nan::New<v8::String>("Oracledb").ToLocalChecked(),temp->GetFunction());
}

/*****************************************************************************/
/*
   DESCRIPTION
     Invoked when new of oracledb is called from JS

*/
NAN_METHOD(Oracledb::New)
{

  Oracledb *oracledb = new Oracledb();
  oracledb->Wrap(info.This());
  oracledb->jsOracledb.Reset( info.This() );
  info.GetReturnValue().Set(info.This());
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of poolMin Property
*/
NAN_PROPERTY_GETTER(Oracledb::GetPoolMin)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->poolMin_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of poolMin Property
*/
NAN_SETTER(Oracledb::SetPoolMin)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->poolMin_, value, "poolMin");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of poolMax Property
*/
NAN_PROPERTY_GETTER(Oracledb::GetPoolMax)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->poolMax_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of poolMax Property
*/
NAN_SETTER(Oracledb::SetPoolMax)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->poolMax_, value, "poolMax");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of poolIncrement Property
*/
NAN_PROPERTY_GETTER(Oracledb::GetPoolIncrement)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->poolIncrement_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of poolIncrement Property
*/
NAN_SETTER(Oracledb::SetPoolIncrement)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->poolIncrement_, value, "poolIncrement");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of poolTimeout Property
*/
NAN_PROPERTY_GETTER(Oracledb::GetPoolTimeout)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->poolTimeout_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of poolTimeout Property
*/
NAN_SETTER(Oracledb::SetPoolTimeout)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->poolTimeout_ , value, "poolTimeout");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of maxRows property
*/
NAN_PROPERTY_GETTER(Oracledb::GetMaxRows)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->maxRows_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of maxRows property
*/
NAN_SETTER(Oracledb::SetMaxRows)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->maxRows_, value, "maxRows");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of outFormat property
*/
NAN_PROPERTY_GETTER(Oracledb::GetOutFormat)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  info.GetReturnValue().Set(Nan::New<v8::Integer>(oracledb->outFormat_));
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of outFormat property
*/
NAN_SETTER(Oracledb::SetOutFormat)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->outFormat_, value, "outFormat");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of stmtCacheSize property
*/
NAN_PROPERTY_GETTER(Oracledb::GetStmtCacheSize)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  info.GetReturnValue().Set(Nan::New<v8::Integer>(oracledb->stmtCacheSize_));
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of stmtCacheSize property
*/
NAN_SETTER(Oracledb::SetStmtCacheSize)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->stmtCacheSize_, value, "stmtCacheSize");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of prefetchRows property
*/
NAN_PROPERTY_GETTER(Oracledb::GetPrefetchRows)
{
  Oracledb* oracledb   = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  info.GetReturnValue().Set(Nan::New<v8::Integer>(oracledb->prefetchRows_));
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of prefetchRows property
*/
NAN_SETTER(Oracledb::SetPrefetchRows)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->prefetchRows_, value, "prefetchRows");
}


/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of autoCommit property
*/
NAN_PROPERTY_GETTER(Oracledb::GetAutoCommit)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  info.GetReturnValue().Set(Nan::New<v8::Boolean>(oracledb->autoCommit_));
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of autoCommit property
*/
NAN_SETTER(Oracledb::SetAutoCommit)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  oracledb->autoCommit_ = value->ToBoolean()->Value();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of version property
*/
NAN_PROPERTY_GETTER(Oracledb::GetVersion)
{
  int version = NJS_NODE_ORACLEDB_VERSION;
  info.GetReturnValue().Set(Nan::New<v8::Integer>(version));
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of version property
*/
NAN_SETTER(Oracledb::SetVersion)
{
  std::string msg;
  msg = NJSMessages::getErrorMsg(errReadOnly, "version");
  NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length());
}


/*****************************************************************************/
/*
  DESCRIPTION
    Get Accessor of connectionClass property
*/
NAN_PROPERTY_GETTER(Oracledb::GetConnectionClass)
{

  Oracledb *oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<String> value = Nan::New<v8::String>(oracledb->connClass_.c_str(),
                                          (int)oracledb->connClass_.length ()).ToLocalChecked();
  info.GetReturnValue().Set(value);
}


/*****************************************************************************/
/*
  DESCRIPTION
    Set Accessor of connectionClass property
*/
NAN_SETTER(Oracledb::SetConnectionClass)
{

  Oracledb *oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  v8::String::Utf8Value utfstr ( value->ToString () );

  oracledb->connClass_ = std::string ( *utfstr, utfstr.length() );
}


/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of externalAuth property
*/
NAN_PROPERTY_GETTER(Oracledb::GetExternalAuth)
{

  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Boolean> value = Nan::New<v8::Boolean>(oracledb->externalAuth_);

  info.GetReturnValue().Set(value);
}


/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of externalAuth property
*/
NAN_SETTER(Oracledb::SetExternalAuth)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  oracledb->externalAuth_ = value->ToBoolean()->Value();
}


/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of lobPrefetchSize property
*/
NAN_PROPERTY_GETTER(Oracledb::GetLobPrefetchSize)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Integer> value = Nan::New<v8::Integer>(oracledb->lobPrefetchSize_);
  info.GetReturnValue().Set(value);
}

/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of lobPrefetchSize property
*/
NAN_SETTER(Oracledb::SetLobPrefetchSize)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  NJS_SET_PROP_UINT(oracledb->lobPrefetchSize_, value, "lobPrefetchSize");
}

/*****************************************************************************/
/*
   DESCRIPTION
     Get Accessor of FetchAsString property
*/
NAN_PROPERTY_GETTER(Oracledb::GetFetchAsString)
{

  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Array> typeArray = Nan::New <v8::Array>(0);
  
  if ( oracledb->fetchAsStringTypes_ )
  {
    unsigned int nCount = sizeof ( oracledb->fetchAsStringTypes_ ) / 
                   sizeof ( oracledb->fetchAsStringTypes_[0] ) ;
    
    typeArray = Nan::New<v8::Array>( nCount );
    for ( unsigned int t = 0; t < nCount ; t ++ )
    {
      typeArray->Set (t, Nan::New<v8::Integer>(oracledb->fetchAsStringTypes_[t]));
    }
  }

  info.GetReturnValue().Set(typeArray);
}


/*****************************************************************************/
/*
   DESCRIPTION
     Set Accessor of FetchAsString property
*/
NAN_SETTER(Oracledb::SetFetchAsString)
{
  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb>(info.Holder());
  Local<Array> array;
  string msg;

  if ( !value->IsArray () )
  {
    msg = NJSMessages::getErrorMsg ( errEmptyArrayForFetchAs );
    NJS_SET_EXCEPTION(msg.c_str(), (int) msg.length () );
  }
      
  array = value.As<v8::Array> ();
  if ( array->Length () == 0 )
  {
    if ( oracledb->fetchAsStringTypes_ )
    {
      free ( oracledb->fetchAsStringTypes_ ) ;
      oracledb->fetchAsStringTypesCount_ = 0 ;
      return;
    }
  }
  
  // If already defined, clear the array.
  if ( oracledb->fetchAsStringTypes_ )
  {
    free ( oracledb->fetchAsStringTypes_ );
  }

  oracledb->fetchAsStringTypesCount_ = array->Length ();
  
  oracledb->fetchAsStringTypes_ = (DataType *)malloc ( 
                                      array->Length() * sizeof ( DataType ) );
  for ( unsigned int t = 0 ; t < array->Length () ; t ++ )
  {
    DataType type = (DataType) 
                    array->Get(t).As<v8::Integer>()->ToInt32()->Value ();
    if ( ( type == DATA_STR  ) || ( type == DATA_DEFAULT ) )
    {
      msg = NJSMessages::getErrorMsg ( errInvalidTypeForConversion );
      NJS_SET_EXCEPTION(msg.c_str(), (int)msg.length () );
    }
    oracledb->fetchAsStringTypes_[t] = type;
  }
}



/*****************************************************************************/
                                                                             /*
   DESCRIPTION
     Get Connection method on Oracledb class.

   PARAMETERS:
     Arguments - Connection attributes as JSON object,
                 Callback
*/
NAN_METHOD(Oracledb::GetConnection)
{

  Local<Function> callback;
  Local<Object> connProps;
  NJS_GET_CALLBACK ( callback, info );

  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb> ( info.This() );
  connectionBaton *connBaton = new connectionBaton ();
  connBaton->cb.Reset( callback );

  NJS_CHECK_NUMBER_OF_ARGS ( connBaton->error, info, 2, 2, exitGetConnection );
  NJS_GET_ARG_V8OBJECT ( connProps, connBaton->error, info, 0,
                         exitGetConnection );
  NJS_GET_STRING_FROM_JSON ( connBaton->user, connBaton->error,
                             connProps, "user", 0, exitGetConnection );
  NJS_GET_STRING_FROM_JSON ( connBaton->pswrd, connBaton->error,
                             connProps, "password", 0, exitGetConnection );
  NJS_GET_STRING_FROM_JSON ( connBaton->connStr, connBaton->error,
                             connProps, "connectString", 0, exitGetConnection );

  connBaton->connClass      = oracledb->connClass_;

  // the following properties will be overriden if provided as call parameters

  connBaton->stmtCacheSize  = oracledb->stmtCacheSize_;
  connBaton->externalAuth = oracledb->externalAuth_;

  NJS_GET_UINT_FROM_JSON   ( connBaton->stmtCacheSize, connBaton->error,
                             connProps, "stmtCacheSize", 0, exitGetConnection );
  NJS_GET_BOOL_FROM_JSON   ( connBaton->externalAuth, connBaton->error,
                             connProps, "externalAuth", 0, exitGetConnection );

  connBaton->oracledb   =  oracledb;
  connBaton->dpienv     =  oracledb->dpienv_;
  connBaton->lobPrefetchSize =  oracledb->lobPrefetchSize_;

exitGetConnection :
  connBaton->req.data  =  (void*) connBaton;
  // This needs to be called even in error case to make the control
  // fall through uv_after_work_cb. In case of error being present in
  // baton, the worker thread anyway returns
  uv_queue_work( uv_default_loop(), &connBaton->req, Async_GetConnection,
                 (uv_after_work_cb) Async_AfterGetConnection );
  info.GetReturnValue().SetUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker function of Get Connection method

   PARAMETERS:
     UV queue work block

   NOTES:
     DPI call execution.
*/
void Oracledb::Async_GetConnection (uv_work_t *req)
{
  connectionBaton *connBaton = (connectionBaton*)req->data;

  if(!(connBaton->error).empty()) goto exitAsync_GetConnection;
  try
  {
    connBaton->dpiconn = connBaton-> dpienv ->
                               getConnection( connBaton->user,
                                              connBaton->pswrd,
                                              connBaton->connStr,
                                              connBaton->stmtCacheSize,
                                              connBaton->connClass,
                                              connBaton->externalAuth );
    
    connBaton->dpiconn->lobPrefetchSize(connBaton->lobPrefetchSize);
  }
  catch (dpi::Exception& e)
  {
    connBaton->error = std::string(e.what());
  }
  exitAsync_GetConnection:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Callback function of Get Connection method

   PARAMETERS:
     UV queue work block

   NOTES:
     Connection handle is formed and handed over to JS.

*/
void Oracledb::Async_AfterGetConnection (uv_work_t *req)
{
  Nan::HandleScope scope;
  connectionBaton *connBaton = (connectionBaton*)req->data;

  v8::TryCatch tc;
  Local<Value> argv[2];
  if( !(connBaton->error).empty() )
  {
    argv[0] = v8::Exception::Error(Nan::New<v8::String>( (connBaton->error).c_str() ).ToLocalChecked());
    argv[1] = Nan::Null();
  } 
  else
  {
    argv[0] = Nan::Undefined();
    Local<FunctionTemplate> lft = Nan::New(Connection::connectionTemplate_s);
    Handle<Object> connection = lft->GetFunction()-> NewInstance();
    (ObjectWrap::Unwrap<Connection> (connection))->
                                setConnection( connBaton->dpiconn,
                                               connBaton->oracledb );
    argv[1] = Nan::New<Value>(connection);
  }
  Local<Function> callback = Nan::New(connBaton->cb);
  delete connBaton;
  Nan::MakeCallback( 
    Nan::GetCurrentContext()->Global(),
    callback, 
    2, 
    argv );
  if(tc.HasCaught())
    node::FatalException(tc);
}

/*****************************************************************************/
/*
   DESCRIPTION
     CreatePool method on Oracledb class.

   PARAMETERS:
     Arguments - Pool attributes as JSON object,
                 Callback
*/
NAN_METHOD(Oracledb::CreatePool)
{

  Local<Function> callback;
  Local<Object> poolProps;
  NJS_GET_CALLBACK ( callback, info );

  Oracledb* oracledb = ObjectWrap::Unwrap<Oracledb> ( info.This() );
  connectionBaton *poolBaton = new connectionBaton ();

  poolBaton->cb.Reset( callback );

  NJS_CHECK_NUMBER_OF_ARGS ( poolBaton->error, info, 2, 2, exitCreatePool );
  NJS_GET_ARG_V8OBJECT ( poolProps, poolBaton->error, info, 0,
                         exitCreatePool );
  NJS_GET_STRING_FROM_JSON ( poolBaton->user, poolBaton->error,
                             poolProps, "user", 0, exitCreatePool );
  NJS_GET_STRING_FROM_JSON ( poolBaton->pswrd, poolBaton->error,
                             poolProps, "password", 0, exitCreatePool );
  NJS_GET_STRING_FROM_JSON ( poolBaton->connStr, poolBaton->error,
                             poolProps, "connectString", 0, exitCreatePool );

  poolBaton->poolMax       =  oracledb->poolMax_;
  poolBaton->poolMin       =  oracledb->poolMin_;
  poolBaton->poolIncrement =  oracledb->poolIncrement_;
  poolBaton->poolTimeout   =  oracledb->poolTimeout_;
  poolBaton->stmtCacheSize =  oracledb->stmtCacheSize_;
  poolBaton->externalAuth = oracledb->externalAuth_;

  NJS_GET_UINT_FROM_JSON   ( poolBaton->poolMax, poolBaton->error,
                             poolProps, "poolMax", 0, exitCreatePool );
  NJS_GET_UINT_FROM_JSON   ( poolBaton->poolMin, poolBaton->error,
                             poolProps, "poolMin", 0, exitCreatePool );
  NJS_GET_UINT_FROM_JSON   ( poolBaton->poolIncrement, poolBaton->error,
                             poolProps, "poolIncrement", 0, exitCreatePool );
  NJS_GET_UINT_FROM_JSON   ( poolBaton->poolTimeout, poolBaton->error,
                             poolProps, "poolTimeout", 0, exitCreatePool );
  NJS_GET_UINT_FROM_JSON   ( poolBaton->stmtCacheSize, poolBaton->error,
                             poolProps, "stmtCacheSize", 0, exitCreatePool );
  NJS_GET_BOOL_FROM_JSON   ( poolBaton->externalAuth, poolBaton->error,
                             poolProps, "externalAuth", 0, exitCreatePool );

  poolBaton->oracledb  =  oracledb;
  poolBaton->dpienv    =  oracledb->dpienv_;
  poolBaton->lobPrefetchSize =  oracledb->lobPrefetchSize_;

exitCreatePool:
  poolBaton->req.data = (void *)poolBaton;

  uv_queue_work(uv_default_loop(),
                &poolBaton->req,
                Async_CreatePool,
                (uv_after_work_cb) Async_AfterCreatePool);

  info.GetReturnValue().SetUndefined();
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker Function of CreatePool.

   PARAMETERS:
     UV queue work block

   NOTES:
     DPI call execution.
*/
void Oracledb::Async_CreatePool (uv_work_t *req)
{
  connectionBaton *poolBaton = (connectionBaton *)req->data;

  if(!(poolBaton->error).empty()) goto exitAsyncCreatePool;

  try
  {
    poolBaton->dpipool = poolBaton-> dpienv ->
                                     createPool ( poolBaton->user,
                                                  poolBaton->pswrd,
                                                  poolBaton->connStr,
                                                  poolBaton->poolMax,
                                                  poolBaton->poolMin,
                                                  poolBaton->poolIncrement,
                                                  poolBaton->poolTimeout,
                                                  poolBaton->stmtCacheSize,
                                                  poolBaton->externalAuth );
  }
  catch (dpi::Exception &e)
  {
    poolBaton->error = std::string (e.what() );
  }
  exitAsyncCreatePool:
  ;
}

/*****************************************************************************/
/*
   DESCRIPTION
     Worker Function of CreatePool.

   PARAMETERS:
     UV queue work block

   NOTES:
     Pool handle is created and handed over to JS.
*/
void Oracledb::Async_AfterCreatePool (uv_work_t *req)
{
  Nan::HandleScope scope;
  connectionBaton *poolBaton = (connectionBaton *)req->data;

  v8::TryCatch tc;
  Local<Value> argv[2];

  if (!poolBaton->error.empty())
  {
    argv[0] = v8::Exception::Error(Nan::New<v8::String>(( poolBaton->error).c_str() ).ToLocalChecked());
    argv[1] = Nan::Undefined();
  }
  else
  {
    argv[0] = Nan::Undefined();
    Local<Object> njsPool = Nan::New(Pool::poolTemplate_s)->
                             GetFunction() ->NewInstance();
    (ObjectWrap::Unwrap<Pool> (njsPool))-> setPool ( poolBaton->dpipool,
                                                     poolBaton->oracledb,
                                                     poolBaton->poolMax,
                                                     poolBaton->poolMin,
                                                     poolBaton->poolIncrement,
                                                     poolBaton->poolTimeout,
                                                     poolBaton->stmtCacheSize,
                                                     poolBaton->lobPrefetchSize);
    argv[1] = njsPool;
  }
  Local<Function> callback = Nan::New(poolBaton->cb);
  delete poolBaton;
  Nan::MakeCallback ( Nan::GetCurrentContext()->Global(),
                       callback, 2, argv);
  if(tc.HasCaught())
  {
    node::FatalException (tc);
  }
}

/*****************************************************************************/
/*
   DESCRIPTION
     Invoked when require on oracledb is called

   PARAMETERS:
     Target Object
*/
extern "C"
{
   static void init(Handle<Object> target)
   {
      Oracledb::Init(target);
      Connection::Init(target);
      Pool::Init(target);
      ResultSet::Init(target);
      ILob::Init(target);
   }

   NODE_MODULE(oracledb, init)
}


/*****************************************************************************/
/*
  DESCRIPTION
    To obtain Fetch-As-String-Types, a new array is allocated and types are
    copied and retured and expected to be freed at the end of execution

  PARAMETERS
    -NONE-

  RETURNS
    array of DataType element to Fetch As String
*/
const DataType * Oracledb::getFetchAsStringTypes () const
{
  DataType *types = NULL;

  if ( fetchAsStringTypes_ )
  {
    unsigned int count = fetchAsStringTypesCount_;

    types = (DataType * )malloc ( sizeof ( DataType ) * count ) ;

    for ( unsigned int i = 0 ; i < count ; i ++ )
    {
      types[i] = fetchAsStringTypes_[i];
    }
  }
  
  return types;
}



/* end of file njsOracle.cpp */

