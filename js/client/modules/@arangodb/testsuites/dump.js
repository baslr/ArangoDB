/* jshint strict: false, sub: true */
/* global print */
'use strict';

// //////////////////////////////////////////////////////////////////////////////
// / DISCLAIMER
// /
// / Copyright 2016 ArangoDB GmbH, Cologne, Germany
// / Copyright 2014 triagens GmbH, Cologne, Germany
// /
// / Licensed under the Apache License, Version 2.0 (the "License")
// / you may not use this file except in compliance with the License.
// / You may obtain a copy of the License at
// /
// /     http://www.apache.org/licenses/LICENSE-2.0
// /
// / Unless required by applicable law or agreed to in writing, software
// / distributed under the License is distributed on an "AS IS" BASIS,
// / WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// / See the License for the specific language governing permissions and
// / limitations under the License.
// /
// / Copyright holder is ArangoDB GmbH, Cologne, Germany
// /
// / @author Max Neunhoeffer
// //////////////////////////////////////////////////////////////////////////////

const functionsDocumentation = {
  'dump': 'dump tests'
};
const optionsDocumentation = [
];

const pu = require('@arangodb/process-utils');
const tu = require('@arangodb/test-utils');

// const BLUE = require('internal').COLORS.COLOR_BLUE;
const CYAN = require('internal').COLORS.COLOR_CYAN;
// const GREEN = require('internal').COLORS.COLOR_GREEN;
// const RED = require('internal').COLORS.COLOR_RED;
const RESET = require('internal').COLORS.COLOR_RESET;
// const YELLOW = require('internal').COLORS.COLOR_YELLOW;

// //////////////////////////////////////////////////////////////////////////////
// / @brief TEST: dump
// //////////////////////////////////////////////////////////////////////////////

function dump (options) {
  let cluster;

  if (options.cluster) {
    cluster = '-cluster';
  } else {
    cluster = '';
  }

  print(CYAN + 'dump tests...' + RESET);

  let instanceInfo = pu.startInstance('tcp', options, {}, 'dump');

  if (instanceInfo === false) {
    return {
      dump: {
        status: false,
        message: 'failed to start server!'
      }
    };
  }

  print(CYAN + Date() + ': Setting up' + RESET);

  let results = {};
  results.setup = tu.runInArangosh(options, instanceInfo,
    tu.makePathUnix('js/server/tests/dump/dump-setup' + cluster + '.js'));

  if (pu.arangod.check.instanceAlive(instanceInfo, options) &&
    (results.setup.status === true)) {
    print(CYAN + Date() + ': Dump and Restore - dump' + RESET);

    results.dump = pu.run.arangoDumpRestore(options, instanceInfo, 'dump',
      'UnitTestsDumpSrc');

    if (pu.arangod.check.instanceAlive(instanceInfo, options) &&
      (results.dump.status === true)) {
      print(CYAN + Date() + ': Dump and Restore - restore' + RESET);

      results.restore = pu.run.arangoDumpRestore(options, instanceInfo, 'restore',
        'UnitTestsDumpDst');

      if (pu.arangod.check.instanceAlive(instanceInfo, options) &&
        (results.restore.status === true)) {
        print(CYAN + Date() + ': Dump and Restore - dump after restore' + RESET);

        results.test = tu.runInArangosh(options, instanceInfo,
          tu.makePathUnix('js/server/tests/dump/dump' + cluster + '.js'), {
            'server.database': 'UnitTestsDumpDst'
          });

        if (pu.arangod.check.instanceAlive(instanceInfo, options) &&
          (results.test.status === true)) {
          print(CYAN + Date() + ': Dump and Restore - teardown' + RESET);

          results.tearDown = tu.runInArangosh(options, instanceInfo,
            tu.makePathUnix('js/server/tests/dump/dump-teardown' + cluster + '.js'));
        }
      }
    }
  }

  print(CYAN + 'Shutting down...' + RESET);
  pu.shutdownInstance(instanceInfo, options);
  print(CYAN + 'done.' + RESET);

  print();

  return results;
}

function setup (testFns, defaultFns, opts, fnDocs, optionsDoc) {
  testFns['dump'] = dump;
  defaultFns.push('dump');
  for (var attrname in functionsDocumentation) { fnDocs[attrname] = functionsDocumentation[attrname]; }
  for (var i = 0; i < optionsDocumentation.length; i++) { optionsDoc.push(optionsDocumentation[i]); }
}

exports.setup = setup;
