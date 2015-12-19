////////////////////////////////////////////////////////////////////////////////
/// @brief Aql, query parser
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Basics/logging.h"
#include "Aql/Parser.h"
#include "Aql/AstNode.h"
#include "Aql/QueryResult.h"

using namespace triagens::aql;

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief create the parser
////////////////////////////////////////////////////////////////////////////////

Parser::Parser (Query* query) 
  : _query(query),
    _ast(query->ast()),
    _scanner(nullptr),
    _buffer(query->queryString()),
    _remainingLength(query->queryLength()),
    _offset(0),
    _marker(nullptr),
    _stack(),
    _isModificationQuery(false) {
  
  _stack.reserve(4);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy the parser
////////////////////////////////////////////////////////////////////////////////

Parser::~Parser () {
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief set data for write queries
////////////////////////////////////////////////////////////////////////////////

bool Parser::configureWriteQuery (AstNode const* collectionNode,
                                  AstNode* optionNode) {

  // now track which collection is going to be modified
  _ast->addWriteCollection(collectionNode);

  if (optionNode != nullptr && ! optionNode->isConstant()) {
    _query->registerError(TRI_ERROR_QUERY_COMPILE_TIME_OPTIONS);
  }

  // register that we have seen a modification operation
  _isModificationQuery = true;

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parse the query
////////////////////////////////////////////////////////////////////////////////

QueryResult Parser::parse (bool withDetails) {
  char const* q = queryString();
  LOG_TRACE("parser parse %s", q);

  if (q == nullptr || *q == '\0') {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_QUERY_EMPTY);
  }
  LOG_TRACE("1");
  // start main scope
  auto scopes = _ast->scopes();
  LOG_TRACE("2");
  scopes->start(AQL_SCOPE_MAIN);
  LOG_TRACE("3");
  Aqllex_init(&_scanner);
  LOG_TRACE("4");
  Aqlset_extra(this, _scanner);
  LOG_TRACE("5");

  try {
    // parse the query string
    LOG_TRACE("5.1");
    if (Aqlparse(this)) {
      LOG_TRACE("5.2");

      LOG_TRACE("parsing AQL query failed");

      // lexing/parsing failed
      THROW_ARANGO_EXCEPTION(TRI_ERROR_QUERY_PARSE);
    }
    LOG_TRACE("5.3");

    Aqllex_destroy(_scanner);
  }
  catch (...) {
    Aqllex_destroy(_scanner);
    throw;
  }
  LOG_TRACE("6");
 
  // end main scope
  scopes->endCurrent();
  LOG_TRACE("7");

  TRI_ASSERT(scopes->numActive() == 0);
  LOG_TRACE("8");

  QueryResult result;

  if (withDetails) {
    result.collectionNames = _query->collectionNames();
    result.bindParameters  = _ast->bindParameters();
    result.json            = _ast->toJson(TRI_UNKNOWN_MEM_ZONE, false);
  }
  LOG_TRACE("9");

  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a parse error, position is specified as line / column
////////////////////////////////////////////////////////////////////////////////

void Parser::registerParseError (int errorCode,
                                 char const* format,
                                 char const* data,
                                 int line,
                                 int column) {
  LOG_TRACE("registerParseError %d", errorCode);
  char buffer[512];
  snprintf(buffer,
           sizeof(buffer),
           format,
           data);

  return registerParseError(errorCode, buffer, line, column);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a parse error, position is specified as line / column
////////////////////////////////////////////////////////////////////////////////

void Parser::registerParseError (int errorCode,
                                 char const* data,
                                 int line,
                                 int column) {
  TRI_ASSERT(errorCode != TRI_ERROR_NO_ERROR);
  TRI_ASSERT(data != nullptr);
  LOG_TRACE("registerParseError %d", errorCode);

  // extract the query string part where the error happened
  std::string const region(_query->extractRegion(line, column));

  // note: line numbers reported by bison/flex start at 1, columns start at 0
  std::stringstream errorMessage;
  errorMessage
    << data
    << std::string(" near '")
    << region 
    << std::string("' at position ")
    << line
    << std::string(":")
    << (column + 1);
  
  if (_query->verboseErrors()) {
    errorMessage 
      << std::endl
      << _query->queryString()
      << std::endl;

    // create a neat pointer to the location of the error.
    size_t i;
    for (i = 0; i + 1 < (size_t) column; i++) {
      errorMessage << ' ';
    }
    if (i > 0) {
      errorMessage << '^';
    }
    errorMessage << '^'
                 << '^'
                 << std::endl;
  }

  registerError(errorCode, errorMessage.str().c_str());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a non-parse error
////////////////////////////////////////////////////////////////////////////////

void Parser::registerError (int errorCode,
                            char const* data) {
  LOG_TRACE("parser register error %d", errorCode);
  _query->registerError(errorCode, data);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a warning
////////////////////////////////////////////////////////////////////////////////

void Parser::registerWarning (int errorCode,
                              char const* data,
                              int line,
                              int column) {
  // ignore line and column for now
  _query->registerWarning(errorCode, data);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief push an AstNode into the array element on top of the stack
////////////////////////////////////////////////////////////////////////////////

void Parser::pushArrayElement (AstNode* node) {
  auto array = static_cast<AstNode*>(peekStack());
  TRI_ASSERT(array->type == NODE_TYPE_ARRAY);
  array->addMember(node);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief push an AstNode into the object element on top of the stack
////////////////////////////////////////////////////////////////////////////////

void Parser::pushObjectElement (char const* attributeName,
                                size_t nameLength,
                                AstNode* node) {
  auto object = static_cast<AstNode*>(peekStack());
  TRI_ASSERT(object->type == NODE_TYPE_OBJECT);
  auto element = _ast->createNodeObjectElement(attributeName, nameLength, node);
  object->addMember(element);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief push an AstNode into the object element on top of the stack
////////////////////////////////////////////////////////////////////////////////

void Parser::pushObjectElement (AstNode* attributeName,
                                AstNode* node) {
  auto object = static_cast<AstNode*>(peekStack());
  TRI_ASSERT(object->type == NODE_TYPE_OBJECT);
  auto element = _ast->createNodeCalculatedObjectElement(attributeName, node);
  object->addMember(element);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief push a temporary value on the parser's stack
////////////////////////////////////////////////////////////////////////////////

void Parser::pushStack (void* value) {
  _stack.emplace_back(value);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief pop a temporary value from the parser's stack
////////////////////////////////////////////////////////////////////////////////
        
void* Parser::popStack () {
  TRI_ASSERT(! _stack.empty());

  void* result = _stack.back();
  _stack.pop_back();
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief peek at a temporary value from the parser's stack
////////////////////////////////////////////////////////////////////////////////
        
void* Parser::peekStack () {
  TRI_ASSERT(! _stack.empty());

  return _stack.back();
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
