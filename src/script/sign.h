// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGN_H
#define BITCOIN_SCRIPT_SIGN_H

#include <coins.h>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/keyorigin.h>
#include <script/standard.h>
#include "script/interpreter.h"

class CKey;
class CKeyID;
class CScript;
class CTransaction;

struct bilingual_str;
struct CMutableTransaction;

/** Interface for signature creators. */
class BaseSignatureCreator {
public:
   virtual ~BaseSignatureCreator() {}
   virtual const BaseSignatureChecker& Checker() const =0;

   /** Create a singular (non-script) signature. */
   virtual bool CreateSig(const CKeyStore& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const =0;
   virtual bool CreateSchnorrSig(const CKeyStore& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const =0;
};

/** A signature creator for transactions. */
class MutableTransactionSignatureCreator : public BaseSignatureCreator {
   const CMutableTransaction* txTo;
   unsigned int nIn;
   int nHashType;
   CAmount amount;
   const MutableTransactionSignatureChecker checker;
   const PrecomputedTransactionData* m_txdata;

public:
   MutableTransactionSignatureCreator(const CMutableTransaction* tx, unsigned int input_idx, const CAmount& amount, int hash_type);
   MutableTransactionSignatureCreator(const CMutableTransaction* tx, unsigned int input_idx, const CAmount& amount, const PrecomputedTransactionData* txdata, int hash_type);
   const BaseSignatureChecker& Checker() const override { return checker; }
   bool CreateSig(const CKeyStore& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
   bool CreateSchnorrSig(const CKeyStore& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const override;
};

namespace {
/** Dummy signature checker which accepts all signatures. */
    class DummySignatureChecker final : public BaseSignatureChecker
    {
    public:
        DummySignatureChecker() {}
        bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override { return true; }
        bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror) const override { return true; }
    };
    const DummySignatureChecker DUMMY_CHECKER;

    class DummySignatureCreator final : public BaseSignatureCreator {
    private:
        char m_r_len = 32;
        char m_s_len = 32;
    public:
        DummySignatureCreator(char r_len, char s_len) : m_r_len(r_len), m_s_len(s_len) {}
        const BaseSignatureChecker& Checker() const override { return DUMMY_CHECKER; }
        bool CreateSig(const CKeyStore& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override
        {
            // Create a dummy signature that is a valid DER-encoding
            vchSig.assign(m_r_len + m_s_len + 7, '\000');
            vchSig[0] = 0x30;
            vchSig[1] = m_r_len + m_s_len + 4;
            vchSig[2] = 0x02;
            vchSig[3] = m_r_len;
            vchSig[4] = 0x01;
            vchSig[4 + m_r_len] = 0x02;
            vchSig[5 + m_r_len] = m_s_len;
            vchSig[6 + m_r_len] = 0x01;
            vchSig[6 + m_r_len + m_s_len] = SIGHASH_ALL;
            return true;
        }
        bool CreateSchnorrSig(const CKeyStore& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* tweak, SigVersion sigversion) const override
        {
            sig.assign(64, '\000');
            return true;
        }
    };

}
/** A signature creator that just produces 71-byte empty signatures. */
extern const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR;
/** A signature creator that just produces 72-byte empty signatures. */
extern const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR;

typedef std::pair<CPubKey, std::vector<unsigned char>> SigPair;

// This struct contains information from a transaction input and also contains signatures for that input.
// The information contained here can be used to create a signature and is also filled by ProduceSignature
// in order to construct final scriptSigs and scriptWitnesses.
struct SignatureData {
   bool complete = false; ///< Stores whether the scriptSig and scriptWitness are complete
   bool witness = false; ///< Stores whether the input this SigData corresponds to is a witness input
   CScript scriptSig; ///< The scriptSig of an input. Contains complete signatures or the traditional partial signatures format
   CScript redeem_script; ///< The redeemScript (if any) for the input
   CScript witness_script; ///< The witnessScript (if any) for the input. witnessScripts are used in P2WSH outputs.
   CScriptWitness scriptWitness; ///< The scriptWitness of an input. Contains complete signatures or the traditional partial signatures format. scriptWitness is part of a transaction input per BIP 144.
   TaprootSpendData tr_spenddata; ///< Taproot spending data.
   std::map<CKeyID, SigPair> signatures; ///< BIP 174 style partial signatures for the input. May contain all signatures necessary for producing a final scriptSig or scriptWitness.
   std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> misc_pubkeys;
   std::vector<unsigned char> taproot_key_path_sig; /// Schnorr signature for key path spending
   std::map<std::pair<XOnlyPubKey, uint256>, std::vector<unsigned char>> taproot_script_sigs; ///< (Partial) schnorr signatures, indexed by XOnlyPubKey and leaf_hash.
   std::vector<CKeyID> missing_pubkeys; ///< KeyIDs of pubkeys which could not be found
   std::vector<CKeyID> missing_sigs; ///< KeyIDs of pubkeys for signatures which could not be found
   uint160 missing_redeem_script; ///< ScriptID of the missing redeemScript (if any)
   uint256 missing_witness_script; ///< SHA256 of the missing witnessScript (if any)

   SignatureData() {}
   explicit SignatureData(const CScript& script) : scriptSig(script) {}
   void MergeSignatureData(SignatureData sigdata);
};

/** Produce a script signature using a generic signature creator. */
bool ProduceSignature(const CKeyStore& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata, SigVersion sigversion = SigVersion::BASE, bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false);

/** Produce a script signature for a transaction. */
bool SignSignature(const CKeyStore &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType);
bool SignSignature(const CKeyStore &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType);

/** Extract signature data from a transaction input, and insert it. */
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout);
void UpdateInput(CTxIn& input, const SignatureData& data);

/* Check whether we know how to sign for an output like this, assuming we
 * have all private keys. While this function does not need private keys, the passed
 * provider is used to look up public keys and redeemscripts by hash.
 * Solvability is unrelated to whether we consider this output to be ours. */
bool IsSolvable(const CKeyStore& provider, const CScript& script);

/** Check whether a scriptPubKey is known to be segwit. */
bool IsSegWitOutput(const CKeyStore& provider, const CScript& script);

SignatureData CombineSignatures(const CKeyStore& provider, const CTxOut& txout, const CMutableTransaction& tx, const SignatureData& scriptSig1, const SignatureData& scriptSig2);

void UpdateTransaction(CMutableTransaction& tx, unsigned int nIn, const SignatureData& data);

#endif // BITCOIN_SCRIPT_SIGN_H
