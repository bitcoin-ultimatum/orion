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
namespace BTC {
    struct KeyOriginInfo {
        unsigned char fingerprint[4]; //!< First 32 bits of the Hash160 of the public key at the root of the path
        std::vector<uint32_t> path;

        friend bool operator==(const KeyOriginInfo &a, const KeyOriginInfo &b) {
            return std::equal(std::begin(a.fingerprint), std::end(a.fingerprint), std::begin(b.fingerprint)) &&
                   a.path == b.path;
        }

        ADD_SERIALIZE_METHODS;

        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(fingerprint);
            READWRITE(path);
        }

        void clear() {
            memset(fingerprint, 0, 4);
            path.clear();
        }
    };

    extern const CBasicKeyStore &DUMMY_SIGNING_PROVIDER;

/** Interface for signature creators. */
    class BaseSignatureCreator {
    public:
        virtual ~BaseSignatureCreator() {}

        virtual const BaseSignatureChecker &Checker() const = 0;

        /** Create a singular (non-script) signature. */
        virtual bool CreateSig(const CKeyStore &provider, std::vector<unsigned char> &vchSig, const CKeyID &keyid,
                               const CScript &scriptCode, SigVersion sigversion) const = 0;
    };

/** A signature creator for transactions. */
    class MutableTransactionSignatureCreator : public BaseSignatureCreator {
        const CMutableTransaction *txTo;
        unsigned int nIn;
        int nHashType;
        CAmount amount;
        const MutableTransactionSignatureChecker checker;

    public:
        MutableTransactionSignatureCreator(const CMutableTransaction *txToIn, unsigned int nInIn,
                                           const CAmount &amountIn, int nHashTypeIn = SIGHASH_ALL);

        const BaseSignatureChecker &Checker() const override { return checker; }

        bool CreateSig(const CKeyStore &provider, std::vector<unsigned char> &vchSig, const CKeyID &keyid,
                       const CScript &scriptCode, SigVersion sigversion) const override;

        virtual void Method(){} ;

    };

/** A signature creator that just produces 71-byte empty signatures. */
    extern const BaseSignatureCreator &DUMMY_SIGNATURE_CREATOR;
/** A signature creator that just produces 72-byte empty signatures. */
    extern const BaseSignatureCreator &DUMMY_MAXIMUM_SIGNATURE_CREATOR;

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
        std::map<CKeyID, SigPair> signatures; ///< BIP 174 style partial signatures for the input. May contain all signatures necessary for producing a final scriptSig or scriptWitness.
        std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> misc_pubkeys;
        std::vector<CKeyID> missing_pubkeys; ///< KeyIDs of pubkeys which could not be found
        std::vector<CKeyID> missing_sigs; ///< KeyIDs of pubkeys for signatures which could not be found
        uint160 missing_redeem_script; ///< ScriptID of the missing redeemScript (if any)
        uint256 missing_witness_script; ///< SHA256 of the missing witnessScript (if any)

        SignatureData() {}

        explicit SignatureData(const CScript &script) : scriptSig(script) {}

        void MergeSignatureData(SignatureData sigdata);
    };

/** Produce a script signature using a generic signature creator. */
    bool
    ProduceSignature(const CKeyStore &provider, const BaseSignatureCreator &creator, const CScript &scriptPubKey,
                     SignatureData &sigdata);

    SignatureData CombineSignatures(const CTxOut &txout, const CMutableTransaction &tx, const SignatureData &scriptSig1,
                                    const SignatureData &scriptSig2);

    /** Extract signature data from a transaction input, and insert it. */
    SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout);
    void UpdateInput(CTxIn& input, const SignatureData& data);


}

bool Sign1(const CKeyID& address, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet);
bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL, bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false);
bool SignSignature(const CKeyStore& keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL, bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false);

/**
 * Given two sets of signatures for scriptPubKey, possibly with OP_0 placeholders,
 * combine them intelligently and return the result.
 */
CScript CombineSignatures(const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);
static CScript PushAll(const std::vector<valtype>& values);
#endif // BITCOIN_SCRIPT_SIGN_H
