/*
 * Copyright ©2019 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2019 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <iostream>
#include <algorithm>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);
  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];
  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
getVec(DocTableReader** dtr_array_, IndexTableReader** itr_array_,
       int arraylen_, string word) {
  vector<QueryProcessor::QueryResult> ret;
  for (int i = 0; i < arraylen_; i++) {
    DocIDTableReader* d = itr_array_[i]->LookupWord(word);
    if (d != nullptr) {
      list<docid_element_header> ls = d->GetDocIDList();
      while (!ls.empty()) {
        docid_element_header h = ls.front();
        ls.pop_front();
        string s;
        if (dtr_array_[i]->LookupDocID(h.docid, &s)) {
          QueryProcessor::QueryResult res = {s, h.num_positions};
          ret.push_back(res);
        }
      }
    } 
    delete d;
  }
  return ret;
}  


vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;
  // MISSING:
  if (query.size() == 0) {
    return finalresult;
  }
  // store first vec in a temp
  finalresult = getVec(dtr_array_, itr_array_, arraylen_, query.front());
 
  if (finalresult.empty()) {
    return finalresult;
  }
  auto iter = query.begin();
  iter++;
  for (; iter != query.end(); iter++) {
    vector<QueryProcessor::QueryResult> res = 
      getVec(dtr_array_, itr_array_, arraylen_, *iter);

    if (res.empty()) {
      return finalresult;
    } 
    auto final_iter = finalresult.begin();
    while (final_iter != finalresult.end()) {
      bool found = false;
      auto res_iter = res.begin();
      while (res_iter != res.end()) {
        if (final_iter->document_name.compare(res_iter->document_name) == 0) {
          found = true;
          break;
        }
        res_iter++;
      }
      if (!found) {
        finalresult.erase(final_iter);
        if (finalresult.size() == 0) {
          return finalresult;
        }
      } else {
	final_iter->rank += res_iter->rank;
        final_iter++;
      }
    }
  }
  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
