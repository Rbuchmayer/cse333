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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "CSE333.h"
#include "HashTable.h"
#include "HashTable_priv.h"


// A private utility function to grow the hashtable (increase
// the number of buckets) if its load factor has become too high.
static void ResizeHashtable(HashTable ht);

// a free function that does nothing
static void LLNullFree(LLPayload_t freeme) { }
static void HTNullFree(HTValue_t freeme) { }

// Helper to check if a key is in a linked list
bool inChain(HTKey_t key, LLIter iter);

HashTable AllocateHashTable(HWSize_t num_buckets) {
  HashTable ht;
  HWSize_t  i;

  // defensive programming
  if (num_buckets == 0) {
    return NULL;
  }

  // allocate the hash table record
  ht = (HashTable) malloc(sizeof(HashTableRecord));
  if (ht == NULL) {
    return NULL;
  }

  // initialize the record
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets =
    (LinkedList *) malloc(num_buckets * sizeof(LinkedList));
  if (ht->buckets == NULL) {
    // make sure we don't leak!
    free(ht);
    return NULL;
  }
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = AllocateLinkedList();
    if (ht->buckets[i] == NULL) {
      // allocating one of our bucket chain lists failed,
      // so we need to free everything we allocated so far
      // before returning NULL to indicate failure.  Since
      // we know the chains are empty, we'll pass in a
      // free function pointer that does nothing; it should
      // never be called.
      HWSize_t j;
      for (j = 0; j < i; j++) {
        FreeLinkedList(ht->buckets[j], LLNullFree);
      }
      free(ht->buckets);
      free(ht);
      return NULL;
    }
  }

  return (HashTable) ht;
}

void FreeHashTable(HashTable table,
                   ValueFreeFnPtr value_free_function) {
  HWSize_t i;

  Verify333(table != NULL);  // be defensive

  // loop through and free the chains on each bucket
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList  bl = table->buckets[i];
    HTKeyValue *nextKV;

    // pop elements off the the chain list, then free the list
    while (NumElementsInLinkedList(bl) > 0) {
      Verify333(PopLinkedList(bl, (LLPayload_t*)&nextKV));
      value_free_function(nextKV->value);
      free(nextKV);
    }
    // the chain list is empty, so we can pass in the
    // null free function to FreeLinkedList.
    FreeLinkedList(bl, LLNullFree);
  }

  // free the bucket array within the table record,
  // then free the table record itself.
  free(table->buckets);
  free(table);
}

HWSize_t NumElementsInHashTable(HashTable table) {
  Verify333(table != NULL);
  return table->num_elements;
}

HTKey_t FNVHash64(unsigned char *buffer, HWSize_t len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //
  // http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {
    /* xor the bottom with the current octet */
    hval ^= (uint64_t) * bp++;
    /* multiply by the 64 bit FNV magic prime mod 2^64 */
    hval *= FNV_64_PRIME;
  }
  /* return our new hash value */
  return hval;
}

HTKey_t FNVHashInt64(HTValue_t hashval) {
  unsigned char buf[8];
  int i;
  uint64_t hashme = (uint64_t)hashval;

  for (i = 0; i < 8; i++) {
    buf[i] = (unsigned char) (hashme & 0x00000000000000FFULL);
    hashme >>= 8;
  }
  return FNVHash64(buf, 8);
}

HWSize_t HashKeyToBucketNum(HashTable ht, HTKey_t key) {
  return key % ht->num_buckets;
}


int InsertHashTable(HashTable table,
                    HTKeyValue newkeyvalue,
                    HTKeyValue *oldkeyvalue) {
 
  HWSize_t insertbucket;
  LinkedList insertchain;

  Verify333(table != NULL);
  ResizeHashtable(table);

  // calculate which bucket we're inserting into,
  // grab its linked list chain
  insertbucket = HashKeyToBucketNum(table, newkeyvalue.key);
  insertchain = table->buckets[insertbucket];

  // make new pointer to get payload
  HTKeyValue* newptr = (HTKeyValue*) malloc(sizeof(HTKeyValue));
  if (newptr == NULL) {
    return 0;
  }
  newptr->key = newkeyvalue.key;
  newptr->value = newkeyvalue.value;
  
  if (NumElementsInLinkedList(insertchain) == 0) {
    bool app = PushLinkedList(insertchain, (void *) newptr);
    if (!app) {     
      free(newptr);
      return 0;
    }
    table->num_elements++;
    return 1;
  }
 
  // check if in the list
  
  LLIter iter = LLMakeIterator(insertchain, 0);
  if (iter == NULL) {   
    return 0;
  }
  int found = inChain(newkeyvalue.key, iter);

  // not found
  if (found == 0) {
    LLIteratorFree(iter);
    bool app = AppendLinkedList(insertchain, (void *) newptr);
    if (!app) {
      free(newptr);
      return 0;
    }
    table->num_elements++;
    return 1;
  } else{
  // found
  HTKeyValue* old;
  
  LLIteratorGetPayload(iter, (void **) &old);
  if (old == NULL) {
    free(newptr);
    LLIteratorFree(iter);
    return 0;
  }
  // give back old value
  oldkeyvalue->key = old->key;
  oldkeyvalue->value = old->value;
  free(old);
  if (!AppendLinkedList(insertchain, (void *) newptr)) {
    LLIteratorFree(iter);
    free(newptr);
    return 0;
  }
  
  LLIteratorDelete(iter, &HTNullFree);
  LLIteratorFree(iter);
  return 2;
  }
}


int LookupHashTable(HashTable table,
                    HTKey_t key,
                    HTKeyValue *keyvalue) {
  Verify333(table != NULL);

  // find the correct chain  
  HWSize_t lookupbucket;
  LinkedList lookupchain;
  lookupbucket = HashKeyToBucketNum(table, key);
  lookupchain = table->buckets[lookupbucket];
  
  
  if (NumElementsInLinkedList(lookupchain) == 0) {
    return 0;
  }

  LLIter iter = LLMakeIterator(lookupchain, 0);
  if (iter == NULL) {
    return -1;
  }
  // check if in the chain
  if (inChain(key, iter)) {
    HTKeyValue* ptr; 
    LLIteratorGetPayload(iter, (void **) &ptr);
    keyvalue->value = ptr->value;
    keyvalue->key = ptr->key;
    LLIteratorFree(iter);
    return 1;
  }
  LLIteratorFree(iter);
  return 0; 
}

int RemoveFromHashTable(HashTable table,
                        HTKey_t key,
                        HTKeyValue *keyvalue) {
  Verify333(table != NULL);
  // find correct chain
  HWSize_t lookupbucket;
  LinkedList lookupchain;
  lookupbucket = HashKeyToBucketNum(table, key);
  lookupchain = table->buckets[lookupbucket];
 
  if (NumElementsInLinkedList(lookupchain) == 0) {
    return 0;
  }

  LLIter iter = LLMakeIterator(lookupchain, 0);
  if (iter == NULL) {
    return -1;
  }
 
  // check if the key is in the chain
  if (inChain(key, iter)) {
    HTKeyValue* ptr;
    LLIteratorGetPayload(iter, (void **) &ptr);
    keyvalue->value = ptr->value;
    keyvalue->key = ptr->key;
    free(ptr);
    LLIteratorDelete(iter, &HTNullFree);
    table->num_elements--;
    
    LLIteratorFree(iter);
    return 1;
  }
  LLIteratorFree(iter); 
  return 0;
}



HTIter HashTableMakeIterator(HashTable table) {
  HTIterRecord *iter;
  HWSize_t      i;

  Verify333(table != NULL);  // be defensive

  // malloc the iterator
  iter = (HTIterRecord *) malloc(sizeof(HTIterRecord));
  if (iter == NULL) {
    return NULL;
  }

  // if the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->is_valid = false;
    iter->ht = table;
    iter->bucket_it = NULL;
    return iter;
  }

  // initialize the iterator.  there is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->is_valid = true;
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (NumElementsInLinkedList(table->buckets[i]) > 0) {
      iter->bucket_num = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLMakeIterator(table->buckets[iter->bucket_num], 0UL);
  if (iter->bucket_it == NULL) {
    // out of memory!
    free(iter);
    return NULL;
  }
  return iter;
}

void HTIteratorFree(HTIter iter) {
  Verify333(iter != NULL);
  
  // empty chain
  if (iter->bucket_it != NULL) {
    LLIteratorFree(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  // set to invalid
  iter->is_valid = false;
  free(iter); 
}

int HTIteratorNext(HTIter iter) {
  Verify333(iter != NULL);

  // Step 4 -- implement HTIteratorNext.
  if (!iter->is_valid) {
    return 0;
  }
 
  // there is a next element in the chain                                    
  if (LLIteratorHasNext(iter->bucket_it)) {
    LLIteratorNext(iter->bucket_it);
    return 1;
  }

  // end of table
  if (iter->bucket_num == (iter->ht->num_buckets) - 1) {
    iter->is_valid = false;
    return 0;
  }

  // no next element, so find next bucket
  int i;
  for (i = iter->bucket_num + 1; i < iter->ht->num_buckets; i++) {
    if (NumElementsInLinkedList((iter->ht->buckets[i])) > 0) {
      iter->bucket_num = i;
      break;
    }
  }
  
  // no more buckets
  if (i == iter->ht->num_buckets) {
    iter->is_valid = false;
    return 0;
  }

  // make new iter for the new chain
  LLIteratorFree(iter->bucket_it); 
  iter->bucket_it = LLMakeIterator(iter->ht->buckets[iter->bucket_num], 0);
  if (iter->bucket_it == NULL) {
    iter->is_valid = false;
    return 0;
  }  
  return 1;  
}

int HTIteratorPastEnd(HTIter iter) {
  Verify333(iter != NULL);
  
  // Step 5 -- implement HTIteratorPastEnd.
  if (iter->is_valid) { 
    return 0;
  }
  
  return 1;  // you might need to change this return value.
}

int HTIteratorGet(HTIter iter, HTKeyValue *keyvalue) {
  Verify333(iter != NULL);
  
  // Step 6 -- implement HTIteratorGet.
  if (!iter->is_valid) {
    return 0;
  }
  HTKeyValue* ptr;
  LLIteratorGetPayload(iter->bucket_it, (void **) &ptr);
  keyvalue->key = ptr->key;
  keyvalue->value = ptr->value;
  
  return 1;  // you might need to change this return value.
}

int HTIteratorDelete(HTIter iter, HTKeyValue *keyvalue) {
  HTKeyValue kv;
  int res, retval;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  res = HTIteratorGet(iter, &kv);
  if (res == 0)
    return 0;

  // Advance the iterator.
  res = HTIteratorNext(iter);
  if (res == 0) {
    retval = 2;
  } else {
    retval = 1;
  }
  res = RemoveFromHashTable(iter->ht, kv.key, keyvalue);
  Verify333(res == 1);
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return retval;
}

static void ResizeHashtable(HashTable ht) {
  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  HashTable newht = AllocateHashTable(ht->num_buckets * 9);

  // Give up if out of memory.
  if (newht == NULL)
    return;

  // Loop through the old ht with an iterator,
  // inserting into the new HT.
  HTIter it = HashTableMakeIterator(ht);
  if (it == NULL) {
    // Give up if out of memory.
    FreeHashTable(newht, &HTNullFree);
    return;
  }

  while (!HTIteratorPastEnd(it)) {
    HTKeyValue item, dummy;

    Verify333(HTIteratorGet(it, &item) == 1);
    if (InsertHashTable(newht, item, &dummy) != 1) {
      // failure, free up everything, return.
      HTIteratorFree(it);
      FreeHashTable(newht, &HTNullFree);
      return;
    }
    HTIteratorNext(it);
  }

  // Worked!  Free the iterator.
  HTIteratorFree(it);

  // Sneaky: swap the structures, then free the new table,
  // and we're done.
  {
    HashTableRecord tmp;

    tmp = *ht;
    *ht = *newht;
    *newht = tmp;
    FreeHashTable(newht, &HTNullFree);
  }

  return;
}

// helper function to lookup keys in linked list
bool inChain(HTKey_t k, LLIter iter) {
  if (iter == NULL) {
    return false;
  }
    do {
      // get payload of iter and check key
      HTKeyValue* ptr;
      LLIteratorGetPayload(iter, (void **) &ptr);
      if (ptr->key == k) {
	return true;
      }
      if (LLIteratorHasNext(iter)) {
	LLIteratorNext(iter);
      }
    }while (LLIteratorHasNext(iter));
    
    // still need to check last element
    HTKeyValue* ptr;
    LLIteratorGetPayload(iter, (void**) &ptr);
    if (ptr->key == k) {
      return true;
    } 
    return false;
}
