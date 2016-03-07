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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#include "edge-collection.h"
#include "Basics/Logger.h"
#include "Indexes/EdgeIndex.h"
#include "Utils/CollectionNameResolver.h"
#include "Utils/Transaction.h"
#include "VocBase/document-collection.h"

#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

////////////////////////////////////////////////////////////////////////////////
/// @brief find edges matching search criteria and add them to the result
/// this function is called two times for each edge query:
/// the first call (with matchType 1) will query the index with the originally
/// requested direction, whereas the second call will query the index with the
/// opposite direction (with matchType 2 or 3) to find all counterparts
////////////////////////////////////////////////////////////////////////////////

static bool FindEdges(arangodb::Transaction* trx,
                      TRI_edge_direction_e direction,
                      arangodb::EdgeIndex* edgeIndex,
                      std::vector<TRI_doc_mptr_t>& result,
                      VPackSlice const entry, int matchType) {
  std::unique_ptr<std::vector<TRI_doc_mptr_t*>> found;

  if (direction == TRI_EDGE_OUT) {
    found.reset(edgeIndex->from()->lookupByKey(trx, &entry));
  } else if (direction == TRI_EDGE_IN) {
    found.reset(edgeIndex->to()->lookupByKey(trx, &entry));
  } else {
    TRI_ASSERT(false);  // TRI_EDGE_ANY not supported here
  }

  size_t const n = found->size();

  if (n > 0) {
    if (result.capacity() == 0) {
      // if result vector is still empty and we have results, re-init the
      // result vector to a "good" size. this will save later reallocations
      result.reserve(n);
    }

    // add all results found
    for (size_t i = 0; i < n; ++i) {
      TRI_doc_mptr_t* mptr = found->at(i);

      // the following queries will use the following sequences of matchTypes:
      // inEdges(): 1,  outEdges(): 1,  edges(): 1, 3

      // if matchType is 1, we'll return all found edges without filtering
      // We'll exclude all loop edges now (we already got them in iteration 1),
      // and also exclude all unidirectional edges
      //
      // if matchType is 3, the direction is also reversed. We'll exclude all
      // loop edges now (we already got them in iteration 1)

      if (matchType > 1) {
        // if the edge is a loop, we have already found it in iteration 1
        // we must skip it here, otherwise we would produce duplicates
        VPackSlice const slice(mptr->vpack());
        if (slice.get(TRI_VOC_ATTRIBUTE_FROM).equals(slice.get(TRI_VOC_ATTRIBUTE_TO))) {
          continue;
        }
      }

      result.emplace_back(*mptr);
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief looks up edges
///        DEPRECATED
////////////////////////////////////////////////////////////////////////////////

std::vector<TRI_doc_mptr_t> TRI_LookupEdgesDocumentCollection(
    arangodb::Transaction* trx, TRI_document_collection_t* document,
    TRI_edge_direction_e direction, TRI_voc_cid_t cid,
    std::string const& key) {

  auto resolver = trx->resolver();
  std::string cname = resolver->getCollectionName(cid);
  VPackBuilder b;
  b.add(VPackValue(cname + "/" + key)); 

  // initialize the result vector
  std::vector<TRI_doc_mptr_t> result;

  auto edgeIndex = document->edgeIndex();

  if (edgeIndex == nullptr) {
    LOG(ERR) << "collection does not have an edges index";
    return result;
  }

  if (direction == TRI_EDGE_IN) {
    // get all edges with a matching IN vertex
    FindEdges(trx, TRI_EDGE_IN, edgeIndex, result, b.slice(), 1);
  } else if (direction == TRI_EDGE_OUT) {
    // get all edges with a matching OUT vertex
    FindEdges(trx, TRI_EDGE_OUT, edgeIndex, result, b.slice(), 1);
  } else if (direction == TRI_EDGE_ANY) {
    // get all edges with a matching IN vertex
    FindEdges(trx, TRI_EDGE_IN, edgeIndex, result, b.slice(), 1);
    // add all non-reflexive edges with a matching OUT vertex
    FindEdges(trx, TRI_EDGE_OUT, edgeIndex, result, b.slice(), 3);
  }

  return result;
}
