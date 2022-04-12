// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/sign.h"

#include "primitives/transaction.h"
#include "key.h"
#include "keystore.h"
#include "script/standard.h"
#include "uint256.h"
#include "util.h"



typedef std::vector<unsigned char> valtype;

bool Sign1(const CKeyID& address, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet, std::vector<valtype>& ret)
{
    CKey key;
    if (!keystore.GetKey(address, key))
        return false;

    std::vector<unsigned char> vchSig;
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)nHashType);
    ret.push_back(std::move(CScript(vchSig.begin(), vchSig.end())));
    scriptSigRet << vchSig;

    return true;
}

bool SignN(const std::vector<valtype>& multisigdata, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet, std::vector<valtype>& ret)
{
    int nSigned = 0;
    int nRequired = multisigdata.front()[0];
    ret.push_back(valtype());
    for (unsigned int i = 1; i < multisigdata.size()-1 && nSigned < nRequired; i++)
    {
        const valtype& pubkey = multisigdata[i];
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
        {
            ++nSigned;
        }
    }
    return nSigned==nRequired;
}

/**
 * Sign scriptPubKey with private keys stored in keystore, given transaction hash and hash type.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TX_SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
bool SignStep(const CKeyStore& keystore, const CScript& scriptPubKey, uint256 hash, int nHashType,
              CScript& scriptSigRet, txnouttype& whichTypeRet, std::vector<valtype>& ret, bool fColdStake = false,
              bool fLeasing = false, bool fForceLeaserSign = false)
{
    scriptSigRet.clear();

    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
    {
        LogPrintf("*** solver solver failed \n");
        return false;
    }

    CKeyID keyID;
    CPubKey vch;
    switch (whichTypeRet)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_UNKNOWN:
    {
        LogPrintf("*** null data \n");
        return false;
    }
    case TX_ZEROCOINMINT:
        return false;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if(!Sign1(keyID, keystore, hash, nHashType, scriptSigRet, ret))
        {
            LogPrintf("*** Sign1 failed \n");
            return false;
        }
        return true;
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
        {
            LogPrintf("*** solver failed to sign \n");
            return false;
        }
        else
        {
            if (!keystore.GetPubKey(keyID, vch))
                return error("%s : Unable to get public key from keyID", __func__);
            //ret.push_back(std::move( CScript(scriptSigRet.begin() + 1, scriptSigRet.end()) ));
            ret.push_back(ToByteVector(vch));
            scriptSigRet << ToByteVector(vch);
        }

        return true;
    case TX_SCRIPTHASH:
        return keystore.GetCScript(uint160(vSolutions[0]), scriptSigRet);

    case TX_WITNESS_V0_KEYHASH:
        scriptSigRet = CScript(vSolutions[0].begin(), vSolutions[0].end());
        return true;

    case TX_WITNESS_V0_SCRIPTHASH: {
        uint160 h160;
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160.begin());
        return keystore.GetCScript(uint160(vSolutions[0]), scriptSigRet);
    }

    case TX_MULTISIG:
        scriptSigRet << OP_0; // workaround CHECKMULTISIG bug
        return (SignN(vSolutions, keystore, hash, nHashType, scriptSigRet,ret));

    case TX_COLDSTAKE: {
        if (fColdStake) {
            // sign with the cold staker key
            keyID = CKeyID(uint160(vSolutions[0]));
        } else {
            // sign with the owner key
            keyID = CKeyID(uint160(vSolutions[1]));
        }
        if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
            return error("*** %s: failed to sign with the %s key.",
                    __func__, fColdStake ? "cold staker" : "owner");
        CPubKey vch;
        if (!keystore.GetPubKey(keyID, vch))
            return error("%s : Unable to get public key from keyID", __func__);
        scriptSigRet << (fColdStake ? (int)OP_TRUE : OP_FALSE) << ToByteVector(vch);
        return true;
      }

    case TX_LEASE: {
        if (fLeasing) {
            if (!fForceLeaserSign) {
                LogPrintf("*** solver met leaser, who can't sign \n");
                return false;
            }
            // sign with the leaser key
            keyID = CKeyID(uint160(vSolutions[0]));
        } else {
            // sign with the owner key
            keyID = CKeyID(uint160(vSolutions[1]));
        }
        if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet, ret))
            return error("*** %s: failed to sign with the %s key.",
                         __func__, fLeasing ? "leaser" : "owner");
        CPubKey vch;
        if (!keystore.GetPubKey(keyID, vch))
            return error("%s : Unable to get public key from keyID", __func__);
        scriptSigRet << (fLeasing ? (int)OP_TRUE : OP_FALSE) << ToByteVector(vch);
        return true;
      }

    case TX_LEASINGREWARD: {
        // 0. TRXHASH
        // 1. N
        keyID = CKeyID(uint160(vSolutions[2]));
        if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
        {
            LogPrintf("*** solver failed to sign leasing reward \n");
            return false;
        }
        else
        {
            CPubKey vch;
            if (!keystore.GetPubKey(keyID, vch))
                return error("%s : Unable to get public key from keyID for leasing reward" , __func__);
            scriptSigRet << ToByteVector(vch);
        }
        return true;
      }
    }

    LogPrintf("*** solver no case met \n");
    return false;
}

bool SignSignature(const CKeyStore &keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType, bool fColdStake, bool fLeasing, bool fForceLeaserSign)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
   CScript subscript;
   std::vector<valtype> result;
   bool P2SH = false;
    // Leave out the signature from the hash, since a signature can't sign itself.
    // The checksig op will also drop the signatures from its hash.
    uint256 hash = SignatureHash(fromPubKey, txTo, nIn, nHashType);

    txnouttype whichType;
    if (!SignStep(keystore, fromPubKey, hash, nHashType, txin.scriptSig, whichType, result, fColdStake, fLeasing,
                  fForceLeaserSign))
        return false;

    if (whichType == TX_SCRIPTHASH)
    {
        // SignStep returns the subscript that need to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        subscript = txin.scriptSig;

        // Recompute txn hash using subscript in place of scriptPubKey:
        uint256 hash2 = SignatureHash(subscript, txTo, nIn, nHashType);

        bool fSolved = SignStep(keystore, subscript, hash2, nHashType, txin.scriptSig, whichType, result)
                       && whichType != TX_SCRIPTHASH;

        // Append serialized subscript whether or not it is completely signed:
        P2SH = true;
        txin.scriptSig = subscript;
        if (!fSolved) return false;
    }

    if (whichType == TX_WITNESS_V0_KEYHASH)
    {
        //Temporary return false for any types of witness programs before full integration of bitcoin witness signature check
        return false;

        //CKeyID keyID(uint160(txin.scriptSig));

        CScript subscript2;
        subscript2 << OP_DUP << OP_HASH160 << ToByteVector(txin.scriptSig) << OP_EQUALVERIFY << OP_CHECKSIG;

        // Recompute txn hash using subscript in place of scriptPubKey:
        uint256 hash2 = SignatureHash(subscript2, txTo, nIn, nHashType);

        txnouttype subType;
        bool fSolved = SignStep(keystore, subscript2, hash2, nHashType, txin.scriptSig, subType,result);

        // Append serialized subscript whether or not it is completely signed:
        //txin.scriptSig << static_cast<valtype>(subscript);
        txin.scriptSig = subscript2;

        if (!fSolved) return false;
    }
    else if (whichType == TX_WITNESS_V0_SCRIPTHASH)
    {
        //Temporary return false for any types of witness programs before full integration of bitcoin witness signature check
        return false;

        CScript subscript2 = txin.scriptSig;

        // Recompute txn hash using subscript in place of scriptPubKey:
        uint256 hash2 = SignatureHash(subscript2, txTo, nIn, nHashType);

        txnouttype subType;
        bool fSolved = SignStep(keystore, subscript2, hash2, nHashType, txin.scriptSig, subType, result)
                       && subType != TX_SCRIPTHASH
                       && subType != TX_WITNESS_V0_SCRIPTHASH
                       && subType != TX_WITNESS_V0_KEYHASH;

        // Append serialized subscript whether or not it is completely signed:
        //txin.scriptSig << static_cast<valtype>(subscript);
        txin.scriptSig = subscript2;

        if (!fSolved) return false;

    } else if (whichType == TX_WITNESS_UNKNOWN) {
        // This is for the future soft forks of Bitcoin
        return false;
    }

    CScriptWitness witness;
    witness.stack.clear();

    if(P2SH)
    {
       result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
       txin.scriptSig = PushAll(result);
    }
    // Test solution
    return BTC::VerifyScript(txin.scriptSig, fromPubKey, &witness, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&txTo, nIn));
    //return VerifyScript(txin.scriptSig, fromPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, MutableTransactionSignatureChecker(&txTo, nIn));
}

bool SignSignature(const CKeyStore &keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType, bool fColdStake, bool fLeasing, bool fForceLeaserSign)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignature(keystore, txout.scriptPubKey, txTo, nIn, nHashType, fColdStake, fLeasing, fForceLeaserSign);
}

/*static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values)
        result << v;
    return result;
}*/

static CScript PushAll(const std::vector<valtype>& values)
{
   CScript result;
   for (const valtype& v : values) {
      if (v.size() == 0) {
         result << OP_0;
      } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
         result << CScript::EncodeOP_N(v[0]);
      } else if (v.size() == 1 && v[0] == 0x81) {
         result << OP_1NEGATE;
      } else {
         result << v;
      }
   }
   return result;
}

static CScript CombineMultisig(const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                               const std::vector<valtype>& vSolutions,
                               const std::vector<valtype>& sigs1, const std::vector<valtype>& sigs2)
{
    // Combine all the signatures we've got:
    std::set<valtype> allsigs;
    for (const valtype& v : sigs1)
    {
        if (!v.empty())
            allsigs.insert(v);
    }
    for (const valtype& v : sigs2)
    {
        if (!v.empty())
            allsigs.insert(v);
    }

    // Build a map of pubkey -> signature by matching sigs to pubkeys:
    assert(vSolutions.size() > 1);
    unsigned int nSigsRequired = vSolutions.front()[0];
    unsigned int nPubKeys = vSolutions.size()-2;
    std::map<valtype, valtype> sigs;
    for (const valtype& sig : allsigs)
    {
        for (unsigned int i = 0; i < nPubKeys; i++)
        {
            const valtype& pubkey = vSolutions[i+1];
            if (sigs.count(pubkey))
                continue; // Already got a sig for this pubkey

            if (TransactionSignatureChecker(&txTo, nIn).CheckSig(sig, pubkey, scriptPubKey))
            {
                sigs[pubkey] = sig;
                break;
            }
        }
    }
    // Now build a merged CScript:
    unsigned int nSigsHave = 0;
    CScript result; result << OP_0; // pop-one-too-many workaround
    for (unsigned int i = 0; i < nPubKeys && nSigsHave < nSigsRequired; i++)
    {
        if (sigs.count(vSolutions[i+1]))
        {
            result << sigs[vSolutions[i+1]];
            ++nSigsHave;
        }
    }
    // Fill any missing with OP_0:
    for (unsigned int i = nSigsHave; i < nSigsRequired; i++)
        result << OP_0;

    return result;
}

static CScript CombineSignatures(const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                                 const txnouttype txType, const std::vector<valtype>& vSolutions,
                                 std::vector<valtype>& sigs1, std::vector<valtype>& sigs2)
{
    switch (txType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_ZEROCOINMINT:
    case TX_WITNESS_UNKNOWN:
        // Don't know anything about this, assume bigger one is correct:
        if (sigs1.size() >= sigs2.size())
            return PushAll(sigs1);
        return PushAll(sigs2);
    case TX_PUBKEY:
    case TX_PUBKEYHASH:
    case TX_COLDSTAKE:
    case TX_LEASE:
    case TX_LEASINGREWARD:
    case TX_WITNESS_V0_SCRIPTHASH:
    case TX_WITNESS_V0_KEYHASH:
        // Signatures are bigger than placeholders or empty scripts:
        if (sigs1.empty() || sigs1[0].empty())
            return PushAll(sigs2);
        return PushAll(sigs1);
    case TX_SCRIPTHASH:
        if (sigs1.empty() || sigs1.back().empty())
            return PushAll(sigs2);
        else if (sigs2.empty() || sigs2.back().empty())
            return PushAll(sigs1);
        else
        {
            // Recur to combine:
            valtype spk = sigs1.back();
            CScript pubKey2(spk.begin(), spk.end());

            txnouttype txType2;
            std::vector<std::vector<unsigned char> > vSolutions2;
            Solver(pubKey2, txType2, vSolutions2);
            sigs1.pop_back();
            sigs2.pop_back();
            CScript result = CombineSignatures(pubKey2, txTo, nIn, txType2, vSolutions2, sigs1, sigs2);
            result << spk;
            return result;
        }
    case TX_MULTISIG:
        return CombineMultisig(scriptPubKey, txTo, nIn, vSolutions, sigs1, sigs2);
    }

    return CScript();
}

CScript CombineSignatures(const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                          const CScript& scriptSig1, const CScript& scriptSig2)
{
    txnouttype txType;
    std::vector<std::vector<unsigned char> > vSolutions;
    Solver(scriptPubKey, txType, vSolutions);

    std::vector<valtype> stack1;
    BTC::EvalScript(stack1, scriptSig1, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE, nullptr);
    std::vector<valtype> stack2;
    BTC::EvalScript(stack2, scriptSig2, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE, nullptr);

    return CombineSignatures(scriptPubKey, txTo, nIn, txType, vSolutions, stack1, stack2);
}


namespace BTC {

    struct Stacks
    {
        std::vector<valtype> script;
        std::vector<valtype> witness;

        Stacks() = delete;
        Stacks(const Stacks&) = delete;
        explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
            std::vector<valtype> stack1;
            BTC::EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE, nullptr);
        }
    };

    static bool GetCScript(const CKeyStore& provider, const SignatureData& sigdata, const CScriptID& scriptid, CScript& script)
    {
        if (provider.GetCScript(scriptid, script)) {
            return true;
        }
        // Look for scripts in SignatureData
        if (CScriptID(sigdata.redeem_script) == scriptid) {
            script = sigdata.redeem_script;
            return true;
        } else if (CScriptID(sigdata.witness_script) == scriptid) {
            script = sigdata.witness_script;
            return true;
        }
        return false;
    }

    static bool GetPubKey(const CKeyStore& provider, const SignatureData& sigdata, const CKeyID& address, CPubKey& pubkey)
    {
        // Look for pubkey in all partial sigs
        const auto it = sigdata.signatures.find(address);
        if (it != sigdata.signatures.end()) {
            pubkey = it->second.first;
            return true;
        }
        // Look for pubkey in pubkey list
        const auto& pk_it = sigdata.misc_pubkeys.find(address);
        if (pk_it != sigdata.misc_pubkeys.end()) {
            pubkey = pk_it->second.first;
            return true;
        }
        // Query the underlying provider
        return provider.GetPubKey(address, pubkey);
    }
   static bool CreateSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const CKeyStore& provider, std::vector<unsigned char>& sig_out, const CPubKey& pubkey, const CScript& scriptcode, SigVersion sigversion)
   {
      CKeyID keyid = pubkey.GetID();
      const auto it = sigdata.signatures.find(keyid);
      if (it != sigdata.signatures.end()) {
         sig_out = it->second.second;
         return true;
      }
      KeyOriginInfo info;
      //TO_FIX: Set correct signed provider
      if (false/*provider.GetKeyOrigin(keyid, info)*/) {
         sigdata.misc_pubkeys.emplace(keyid, std::make_pair(pubkey, std::move(info)));
      }
      if (creator.CreateSig(provider, sig_out, keyid, scriptcode, sigversion)) {
         auto i = sigdata.signatures.emplace(keyid, SigPair(pubkey, sig_out));
         assert(i.second);
         return true;
      }
      // Could not make signature or signature not found, add keyid to missing
      sigdata.missing_sigs.push_back(keyid);
      return false;
   }

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TxoutType::SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
    static bool SignStep(const CKeyStore& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                         std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion, SignatureData& sigdata)
    {
        CScript scriptRet;
        uint160 h160;
        ret.clear();
        std::vector<unsigned char> sig;

        std::vector<valtype> vSolutions;

        if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
        {
            LogPrintf("*** solver solver failed \n");
            return false;
        }
        switch (whichTypeRet)
        {
            case TX_NONSTANDARD:
            case TX_NULL_DATA:
            case TX_WITNESS_UNKNOWN:
                return false;
            case TX_PUBKEY:
                if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
                ret.push_back(std::move(sig));
                return true;
            case TX_PUBKEYHASH: {
                CKeyID keyID = CKeyID(uint160(vSolutions[0]));
                CPubKey pubkey;
                if (!BTC::GetPubKey(provider, sigdata, keyID, pubkey)) {
                    // Pubkey could not be found, add to missing
                    sigdata.missing_pubkeys.push_back(keyID);
                    return false;
                }
                if (!CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) return false;
                ret.push_back(std::move(sig));
                ret.push_back(ToByteVector(pubkey));
                return true;
            }
            case TX_SCRIPTHASH:
                h160 = uint160(vSolutions[0]);
                if (BTC::GetCScript(provider, sigdata, h160, scriptRet)) {
                    ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                    return true;
                }
                // Could not find redeemScript, add to missing
                sigdata.missing_redeem_script = h160;
                return false;

            case TX_MULTISIG: {
                size_t required = vSolutions.front()[0];
                ret.push_back(valtype()); // workaround CHECKMULTISIG bug
                for (size_t i = 1; i < vSolutions.size() - 1; ++i) {
                    CPubKey pubkey = CPubKey(vSolutions[i]);
                    if (ret.size() < required + 1 && CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) {
                        ret.push_back(std::move(sig));
                    }
                }
                bool ok = ret.size() == required + 1;
                for (size_t i = 0; i + ret.size() < required + 1; ++i) {
                    ret.push_back(valtype());
                }
                return ok;
            }
            case TX_WITNESS_V0_KEYHASH:
                ret.push_back(vSolutions[0]);
                return true;

            case TX_WITNESS_V0_SCRIPTHASH:
                CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160.begin());
                if (GetCScript(provider, sigdata, h160, scriptRet)) {
                    ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                    return true;
                }
                // Could not find witnessScript, add to missing
                sigdata.missing_witness_script = uint256(vSolutions[0]);
                return false;

            default:
                return false;
        }
    }

    class SignatureExtractorChecker final : public BaseSignatureChecker
    {
    private:
        SignatureData& sigdata;
        BaseSignatureChecker& checker;

    public:
        SignatureExtractorChecker(SignatureData& sigdata, BaseSignatureChecker& checker) : sigdata(sigdata), checker(checker) {}
        bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
    };

    bool SignatureExtractorChecker::CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        if (checker.CheckSig(scriptSig, vchPubKey, scriptCode, sigversion)) {
            CPubKey pubkey(vchPubKey);
            sigdata.signatures.emplace(pubkey.GetID(), SigPair(pubkey, scriptSig));
            return true;
        }
        return false;
    }

    bool ProduceSignature(const CKeyStore& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata)
    {
        if (sigdata.complete) return true;

        std::vector<valtype> result;
        txnouttype whichType;
        bool solved = SignStep(provider, creator, fromPubKey, result, whichType, SigVersion::BASE, sigdata);
        bool P2SH = false;
        CScript subscript;
        sigdata.scriptWitness.stack.clear();

        if (solved && whichType == TX_SCRIPTHASH)
        {
            // Solver returns the subscript that needs to be evaluated;
            // the final scriptSig is the signatures from that
            // and then the serialized subscript:
            subscript = CScript(result[0].begin(), result[0].end());
            sigdata.redeem_script = subscript;
            solved = solved && SignStep(provider, creator, subscript, result, whichType, SigVersion::BASE, sigdata) && whichType != TX_SCRIPTHASH;
            P2SH = true;
        }

        if (solved && whichType == TX_WITNESS_V0_KEYHASH)
        {
            CScript witnessscript;
            witnessscript << OP_DUP << OP_HASH160 << ToByteVector(result[0]) << OP_EQUALVERIFY << OP_CHECKSIG;
            txnouttype subType;
            solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata);
            sigdata.scriptWitness.stack = result;
            sigdata.witness = true;
            result.clear();
        }
        else if (solved && whichType == TX_WITNESS_V0_SCRIPTHASH)
        {
            CScript witnessscript(result[0].begin(), result[0].end());
            sigdata.witness_script = witnessscript;
            txnouttype subType;
            solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata) && subType != TX_SCRIPTHASH && subType != TX_WITNESS_V0_SCRIPTHASH && subType != TX_WITNESS_V0_KEYHASH;
            result.push_back(std::vector<unsigned char>(witnessscript.begin(), witnessscript.end()));
            sigdata.scriptWitness.stack = result;
            sigdata.witness = true;
            result.clear();
        } else if (solved && whichType == TX_WITNESS_UNKNOWN) {
            sigdata.witness = true;
        }

        if (P2SH) {
            result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
        }
        sigdata.scriptSig = PushAll(result);

        // Test solution
        sigdata.complete = solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
        return sigdata.complete;
    }
    // Extracts signatures and scripts from incomplete scriptSigs. Please do not extend this, use PSBT instead
    SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout)
    {
        SignatureData data;
        assert(tx.vin.size() > nIn);
        data.scriptSig = tx.vin[nIn].scriptSig;
        //data.scriptWitness = tx.vin[nIn].scriptWitness;
        Stacks stack(data);

        // Get signatures
        MutableTransactionSignatureChecker tx_checker(&tx, nIn);
        SignatureExtractorChecker extractor_checker(data, tx_checker);
        if (VerifyScript(data.scriptSig, txout.scriptPubKey, &data.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, extractor_checker)) {
            data.complete = true;
            return data;
        }

        // Get scripts
        std::vector<std::vector<unsigned char>> solutions;
        txnouttype script_type;
        if (!Solver(txout.scriptPubKey, script_type, solutions))
        {
            LogPrintf("*** solver solver failed \n");
            return data;
        }
        SigVersion sigversion = SigVersion::BASE;
        CScript next_script = txout.scriptPubKey;

        if (script_type == TX_SCRIPTHASH && !stack.script.empty() && !stack.script.back().empty()) {
            // Get the redeemScript
            CScript redeem_script(stack.script.back().begin(), stack.script.back().end());
            data.redeem_script = redeem_script;
            next_script = std::move(redeem_script);

            // Get redeemScript type

            if (!Solver(next_script, script_type, solutions))
            {
                LogPrintf("*** solver solver failed \n");
                return data;
            }
            stack.script.pop_back();
        }
        if (script_type == TX_WITNESS_V0_SCRIPTHASH && !stack.witness.empty() && !stack.witness.back().empty()) {
            // Get the witnessScript
            CScript witness_script(stack.witness.back().begin(), stack.witness.back().end());
            data.witness_script = witness_script;
            next_script = std::move(witness_script);

            // Get witnessScript type
            if (!Solver(next_script, script_type, solutions))
            {
                LogPrintf("*** solver solver failed \n");
                return data;
            }
            stack.witness.pop_back();
            stack.script = std::move(stack.witness);
            stack.witness.clear();
            sigversion = SigVersion::WITNESS_V0;
        }
        if (script_type == TX_MULTISIG && !stack.script.empty()) {
            // Build a map of pubkey -> signature by matching sigs to pubkeys:
            assert(solutions.size() > 1);
            unsigned int num_pubkeys = solutions.size()-2;
            unsigned int last_success_key = 0;
            for (const valtype& sig : stack.script) {
                for (unsigned int i = last_success_key; i < num_pubkeys; ++i) {
                    const valtype& pubkey = solutions[i+1];
                    // We either have a signature for this pubkey, or we have found a signature and it is valid
                    if (data.signatures.count(CPubKey(pubkey).GetID()) || extractor_checker.CheckSig(sig, pubkey, next_script, sigversion)) {
                        last_success_key = i + 1;
                        break;
                    }
                }
            }
        }

        return data;
    }

    void UpdateInput(CTxIn& input, const SignatureData& data)
    {
        input.scriptSig = data.scriptSig;
        //input.scriptWitness = data.scriptWitness;MutableTransactionSignatureCreator
    }
    MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) : txTo(txToIn), nIn(nInIn), nHashType(nHashTypeIn), amount(amountIn), checker(txTo, nIn) {}

    bool MutableTransactionSignatureCreator::CreateSig(const CKeyStore &provider, std::vector<unsigned char> &vchSig, const CKeyID &keyid,
                                                       const CScript &scriptCode, SigVersion sigversion) const
    {
        CKey key;
        if (!provider.GetKey(keyid, key))
            return false;

        // Signing with uncompressed keys is disabled in witness scripts
        if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
            return false;

        uint256 hash = SignatureHash(scriptCode, *txTo, nIn, nHashType);
        if (!key.Sign(hash, vchSig))
            return false;
        vchSig.push_back((unsigned char)nHashType);
        return true;
    }

    void SignatureData::MergeSignatureData(SignatureData sigdata)
    {
        if (complete) return;
        if (sigdata.complete) {
            *this = std::move(sigdata);
            return;
        }
        if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
            redeem_script = sigdata.redeem_script;
        }
        if (witness_script.empty() && !sigdata.witness_script.empty()) {
            witness_script = sigdata.witness_script;
        }
        signatures.insert(std::make_move_iterator(sigdata.signatures.begin()), std::make_move_iterator(sigdata.signatures.end()));
    }
    const CBasicKeyStore& DUMMY_SIGNING_PROVIDER = CBasicKeyStore();

}

