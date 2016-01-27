////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
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
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "LineEditor.h"

#include "Utilities/ShellBase.h"

using namespace std;
using namespace arangodb;
using namespace arangodb;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructs a new editor
////////////////////////////////////////////////////////////////////////////////

LineEditor::LineEditor() : _shell(nullptr), _signalFunc(nullptr) {}

LineEditor::~LineEditor() {
  if (_shell != nullptr) {
    close();
    delete _shell;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not the shell implementation supports colors
////////////////////////////////////////////////////////////////////////////////

bool LineEditor::supportsColors() const { return _shell->supportsColors(); }

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor open
////////////////////////////////////////////////////////////////////////////////

bool LineEditor::open(bool autoComplete) { return _shell->open(autoComplete); }

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor shutdown
////////////////////////////////////////////////////////////////////////////////

bool LineEditor::close() { return _shell->close(); }

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor prompt
////////////////////////////////////////////////////////////////////////////////

std::string LineEditor::prompt(std::string const& prompt,
                               std::string const& begin, bool& eof) {
  return _shell->prompt(prompt, begin, eof);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief add to history
////////////////////////////////////////////////////////////////////////////////

void LineEditor::addHistory(std::string const& line) {
  return _shell->addHistory(line);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief send a signal to the shell implementation
////////////////////////////////////////////////////////////////////////////////

void LineEditor::signal() {
  if (_signalFunc != nullptr) {
    _signalFunc();
  }
  _shell->signal();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a callback function to be executed on signal receipt
////////////////////////////////////////////////////////////////////////////////

void LineEditor::setSignalFunction(std::function<void()> const& func) {
  _signalFunc = func;
}
