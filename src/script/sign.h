// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGN_H
#define BITCOIN_SCRIPT_SIGN_H

#include "script/interpreter.h"
#include "key.h"
#include "keystore.h"
#include "script/standard.h"

class CKeyStore;
class CScript;
class CTransaction;

struct CMutableTransaction;

struct SignatureData {
   CScript scriptSig;
   CScriptWitness scriptWitness;

   SignatureData() {}
   explicit SignatureData(const CScript& script) : scriptSig(script) {}
};

bool Sign1(const CKeyID& address, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet);
bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL, bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false);
bool SignSignature(const CKeyStore& keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL, bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false);

/**
 * Given two sets of signatures for scriptPubKey, possibly with OP_0 placeholders,
 * combine them intelligently and return the result.
 */
CScript CombineSignatures(const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);
static CScript PushAll(const std::vector<valtype>& values);

namespace BTC {
   SignatureData CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                                   const SignatureData& scriptSig1, const SignatureData& scriptSig2);
   void UpdateTransaction(CMutableTransaction& tx, unsigned int nIn, const SignatureData& data);
   SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn);


   class BaseSignatureCreator {
   protected:
      const CKeyStore* keystore;

   public:
      explicit BaseSignatureCreator(const CKeyStore* keystoreIn) : keystore(keystoreIn) {}
      const CKeyStore& KeyStore() const { return *keystore; };
      virtual ~BaseSignatureCreator() {}
      virtual const BaseSignatureChecker& Checker() const =0;

      /** Create a singular (non-script) signature. */
      virtual bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const =0;
   };


   /** A signature creator for transactions. */
class TransactionSignatureCreator : public BTC::BaseSignatureCreator {
      const CTransaction* txTo;
      unsigned int nIn;
      int nHashType;
      CAmount amount;
      const BTC::TransactionSignatureChecker checker;

   public:
      TransactionSignatureCreator(const CKeyStore* keystoreIn, const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn=SIGHASH_ALL);
      const BTC::BaseSignatureChecker& Checker() const override { return checker; }
      bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
   };

   class MutableTransactionSignatureCreator : public TransactionSignatureCreator {
      CTransaction tx;

   public:
      MutableTransactionSignatureCreator(const CKeyStore* keystoreIn, const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) : TransactionSignatureCreator(keystoreIn, &tx, nInIn, amountIn, nHashTypeIn), tx(*txToIn) {}
   };

   /** Produce a script signature using a generic signature creator. */
   bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);
}
#endif // BITCOIN_SCRIPT_SIGN_H
