// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGCACHE_H
#define BITCOIN_SCRIPT_SIGCACHE_H

#include "script/interpreter.h"

#include <vector>

// DoS prevention: limit cache size to 32MB (over 1000000 entries on 64-bit
// systems). Due to how we count cache size, actual memory usage is slightly
// more (~32.25 MB)
static const unsigned int DEFAULT_MAX_SIG_CACHE_SIZE = 32;
// Maximum sig cache size allowed
static const int64_t MAX_MAX_SIG_CACHE_SIZE = 16384;

class CPubKey;




   class SignatureCacheHasher
   {
   public:
      template <uint8_t hash_select>
      uint32_t operator()(const uint256& key) const
      {
         static_assert(hash_select <8, "SignatureCacheHasher only has 8 hashes available.");
         uint32_t u;
         std::memcpy(&u, key.begin()+4*hash_select, 4);
         return u;
      }
   };

class CachingTransactionSignatureChecker : public TransactionSignatureChecker
   {
   private:
      bool store;

   public:
      CachingTransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, bool storeIn, PrecomputedTransactionData& txdataIn) : TransactionSignatureChecker(txToIn, nInIn, amountIn, txdataIn, MissingDataBehavior::ASSERT_FAIL), store(storeIn) {}

   bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const override;
   bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const override;
   };

void InitSignatureCache();
#endif // BITCOIN_SCRIPT_SIGCACHE_H
