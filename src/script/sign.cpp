// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/sign.h>

#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <uint256.h>
#include <util/translation.h>
#include <util/vector.h>

typedef std::vector<unsigned char> valtype;

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction* tx, unsigned int input_idx, const CAmount& amount, int hash_type)
: txTo{tx}, nIn{input_idx}, nHashType{hash_type}, amount{amount}, checker{txTo, nIn, amount, MissingDataBehavior::FAIL},
  m_txdata(nullptr)
{
}

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction* tx, unsigned int input_idx, const CAmount& amount, const PrecomputedTransactionData* txdata, int hash_type)
: txTo{tx}, nIn{input_idx}, nHashType{hash_type}, amount{amount},
  checker{txdata ? MutableTransactionSignatureChecker{txTo, nIn, amount, *txdata, MissingDataBehavior::FAIL} :
          MutableTransactionSignatureChecker{txTo, nIn, amount, MissingDataBehavior::FAIL}},
  m_txdata(txdata)
{
}

bool MutableTransactionSignatureCreator::CreateSig(const CKeyStore& provider, std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
{
   assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0);

   CKey key;
   if (!provider.GetKey(address, key))
      return false;

   // Signing with uncompressed keys is disabled in witness scripts
   if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
      return false;

   // Signing without known amount does not work in witness scripts.
   if (sigversion == SigVersion::WITNESS_V0 && !MoneyRange(amount)) return false;

   // BASE/WITNESS_V0 signatures don't support explicit SIGHASH_DEFAULT, use SIGHASH_ALL instead.
   const int hashtype = nHashType == SIGHASH_DEFAULT ? SIGHASH_ALL : nHashType;

   uint256 hash = SignatureHash(scriptCode, *txTo, nIn, hashtype, amount, sigversion, m_txdata);
   if (!key.Sign(hash, vchSig))
      return false;
   vchSig.push_back((unsigned char)hashtype);
   return true;
}

bool MutableTransactionSignatureCreator::CreateSchnorrSig(const CKeyStore& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const
{
   assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

   CKey key;
   if (!provider.GetKeyByXOnly(pubkey, key)) return false;

   // BIP341/BIP342 signing needs lots of precomputed transaction data. While some
   // (non-SIGHASH_DEFAULT) sighash modes exist that can work with just some subset
   // of data present, for now, only support signing when everything is provided.
   if (!m_txdata || !m_txdata->m_bip341_taproot_ready || !m_txdata->m_spent_outputs_ready) return false;

   ScriptExecutionData execdata;
   execdata.m_annex_init = true;
   execdata.m_annex_present = false; // Only support annex-less signing for now.
   if (sigversion == SigVersion::TAPSCRIPT) {
      execdata.m_codeseparator_pos_init = true;
      execdata.m_codeseparator_pos = 0xFFFFFFFF; // Only support non-OP_CODESEPARATOR BIP342 signing for now.
      if (!leaf_hash) return false; // BIP342 signing needs leaf hash.
      execdata.m_tapleaf_hash_init = true;
      execdata.m_tapleaf_hash = *leaf_hash;
   }
   uint256 hash;
   if (!SignatureHashSchnorr(hash, execdata, *txTo, nIn, nHashType, sigversion, *m_txdata, MissingDataBehavior::FAIL)) return false;
   sig.resize(64);
   // Use uint256{} as aux_rnd for now.
   if (!key.SignSchnorr(hash, sig, merkle_root, {})) return false;
   if (nHashType) sig.push_back(nHashType);
   return true;
}

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
   if (provider.GetKeyOrigin(keyid, info)) {
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

static bool CreateTaprootScriptSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const CKeyStore& provider, std::vector<unsigned char>& sig_out, const XOnlyPubKey& pubkey, const uint256& leaf_hash, SigVersion sigversion)
{
   auto lookup_key = std::make_pair(pubkey, leaf_hash);
   auto it = sigdata.taproot_script_sigs.find(lookup_key);
   if (it != sigdata.taproot_script_sigs.end()) {
      sig_out = it->second;
   }
   if (creator.CreateSchnorrSig(provider, sig_out, pubkey, &leaf_hash, nullptr, sigversion)) {
      sigdata.taproot_script_sigs[lookup_key] = sig_out;
      return true;
   }
   return false;
}

static bool SignTaprootScript(const CKeyStore& provider, const BaseSignatureCreator& creator, SignatureData& sigdata, int leaf_version, const CScript& script, std::vector<valtype>& result)
{
   // Only BIP342 tapscript signing is supported for now.
   if (leaf_version != TAPROOT_LEAF_TAPSCRIPT) return false;
   SigVersion sigversion = SigVersion::TAPSCRIPT;

   uint256 leaf_hash = (CHashWriter(HASHER_TAPLEAF) << uint8_t(leaf_version) << script).GetSHA256();

   // <xonly pubkey> OP_CHECKSIG
   if (script.size() == 34 && script[33] == OP_CHECKSIG && script[0] == 0x20) {
      XOnlyPubKey pubkey{Span{script}.subspan(1, 32)};
      std::vector<unsigned char> sig;
      if (CreateTaprootScriptSig(creator, sigdata, provider, sig, pubkey, leaf_hash, sigversion)) {
         result = Vector(std::move(sig));
         return true;
      }
   }

   return false;
}

static bool SignTaproot(const CKeyStore& provider, const BaseSignatureCreator& creator, const WitnessV1Taproot& output, SignatureData& sigdata, std::vector<valtype>& result)
{
   TaprootSpendData spenddata;

   // Gather information about this output.
   if (provider.GetTaprootSpendData(output, spenddata)) {
      sigdata.tr_spenddata.Merge(spenddata);
   }

   // Try key path spending.
   {
      std::vector<unsigned char> sig;
      if (sigdata.taproot_key_path_sig.size() == 0) {
         if (creator.CreateSchnorrSig(provider, sig, spenddata.internal_key, nullptr, &spenddata.merkle_root, SigVersion::TAPROOT)) {
            sigdata.taproot_key_path_sig = sig;
         }
      }
      if (sigdata.taproot_key_path_sig.size()) {
         result = Vector(sigdata.taproot_key_path_sig);
         return true;
      }
   }

   // Try script path spending.
   std::vector<std::vector<unsigned char>> smallest_result_stack;
   for (const auto& [key, control_blocks] : sigdata.tr_spenddata.scripts) {
      const auto& [script, leaf_ver] = key;
      std::vector<std::vector<unsigned char>> result_stack;
      if (SignTaprootScript(provider, creator, sigdata, leaf_ver, script, result_stack)) {
         result_stack.emplace_back(std::begin(script), std::end(script)); // Push the script
         result_stack.push_back(*control_blocks.begin()); // Push the smallest control block
         if (smallest_result_stack.size() == 0 ||
             GetSerializeSize(result_stack, PROTOCOL_VERSION) < GetSerializeSize(smallest_result_stack, PROTOCOL_VERSION)) {
            smallest_result_stack = std::move(result_stack);
         }
      }
   }
   if (smallest_result_stack.size() != 0) {
      result = std::move(smallest_result_stack);
      return true;
   }

   return false;
}

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TxoutType::SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
static bool SignStep(const CKeyStore& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                     std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion, SignatureData& sigdata,
                     bool fColdStake = false, bool fLeasing = false, bool fForceLeaserSign = false)
{
   CScript scriptRet;
   uint160 h160;
   ret.clear();
   std::vector<unsigned char> sig;

   std::vector<valtype> vSolutions;
   if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
      return false;

   switch (whichTypeRet) {
      case TX_NONSTANDARD:
      case TX_NULL_DATA:
      case TX_WITNESS_UNKNOWN:
      case TX_ZEROCOINMINT:
         return false;
      case TX_PUBKEY:
         if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
         ret.push_back(std::move(sig));
         return true;
      case TX_PUBKEYHASH: {
         CKeyID keyID = CKeyID(uint160(vSolutions[0]));
         CPubKey pubkey;
         if (!GetPubKey(provider, sigdata, keyID, pubkey)) {
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
         if (GetCScript(provider, sigdata, CScriptID{h160}, scriptRet)) {
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
            // We need to always call CreateSig in order to fill sigdata with all
            // possible signatures that we can create. This will allow further PSBT
            // processing to work as it needs all possible signature and pubkey pairs
            if (CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) {
               if (ret.size() < required + 1) {
                  ret.push_back(std::move(sig));
               }
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
         CRIPEMD160().Write(vSolutions[0].data(), vSolutions[0].size()).Finalize(h160.begin());
         if (GetCScript(provider, sigdata, CScriptID{h160}, scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
         }
         // Could not find witnessScript, add to missing
         sigdata.missing_witness_script = uint256(vSolutions[0]);
         return false;

      case TX_WITNESS_V1_TAPROOT:
         return SignTaproot(provider, creator, WitnessV1Taproot(XOnlyPubKey{vSolutions[0]}), sigdata, ret);

      case TX_COLDSTAKE: {
         CKeyID keyID;
         if (fColdStake) {
            // sign with the cold staker key
            keyID = CKeyID(uint160(vSolutions[0]));
         } else {
            // sign with the owner key
            keyID = CKeyID(uint160(vSolutions[1]));
         }
         if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
         ret.push_back(std::move(sig));

         CPubKey pubkey;
         if (!GetPubKey(provider, sigdata, keyID, pubkey)) {
            // Pubkey could not be found, add to missing
            sigdata.missing_pubkeys.push_back(keyID);
            return false;
         }
         scriptRet << ret[0];
         scriptRet << (fColdStake ? (int)OP_TRUE : OP_FALSE) << ToByteVector(pubkey);
         ret.clear();
         ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
         return true;
      }

      case TX_LEASE_CLTV:
      case TX_LEASE: {
         CKeyID keyID;
         if (fLeasing) {
            if (!fForceLeaserSign) {
               LogPrintf("*** solver met leaser, who can't sign \n");
               return false;
            }
            // sign with the leaser key
            keyID = (whichTypeRet == TX_LEASE) ? CKeyID(uint160(vSolutions[0])): CKeyID(uint160(vSolutions[1]));
         } else {
            // sign with the owner key
            keyID = (whichTypeRet == TX_LEASE) ? CKeyID(uint160(vSolutions[1])): CKeyID(uint160(vSolutions[2]));
         }
         if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
         ret.push_back(std::move(sig));

         CPubKey pubkey;
         if (!GetPubKey(provider, sigdata, keyID, pubkey)) {
            // Pubkey could not be found, add to missing
            sigdata.missing_pubkeys.push_back(keyID);
            return false;
         }

         scriptRet << ret[0];
         scriptRet << (fLeasing ? (int)OP_TRUE : OP_FALSE) << ToByteVector(pubkey);
         ret.clear();
         ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
         return true;
      }

      case TX_LEASINGREWARD: {
         // 0. TRXHASH
         // 1. N
         CKeyID keyID = CKeyID(uint160(vSolutions[2]));

         CPubKey pubkey;
         if (!GetPubKey(provider, sigdata, keyID, pubkey)) {
            // Pubkey could not be found, add to missing
            sigdata.missing_pubkeys.push_back(keyID);
            return false;
         }
         if (!CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) return false;
         ret.push_back(ToByteVector(pubkey));
         return true;
      }
   } // no default case, so the compiler can warn about missing cases
   assert(false);
}

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

bool ProduceSignature(const CKeyStore& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata, bool fColdStake, bool fLeasing, bool fForceLeaserSign)
{
   if (sigdata.complete) return true;

   std::vector<valtype> result;
   txnouttype whichType;
   int flags = STANDARD_SCRIPT_VERIFY_FLAGS;
   bool solved = SignStep(provider, creator, fromPubKey, result, whichType, SigVersion::BASE, sigdata, fColdStake, fLeasing, fForceLeaserSign);
   bool P2SH = false;
   CScript subscript;

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
   } else if (whichType == TX_WITNESS_V1_TAPROOT && !P2SH) {
      sigdata.witness = true;
      if (solved) {
         sigdata.scriptWitness.stack = std::move(result);
      }
      result.clear();
   } else if (solved && whichType == TX_WITNESS_UNKNOWN) {
      sigdata.witness = true;
   }

   if (!sigdata.witness) sigdata.scriptWitness.stack.clear();
   if (P2SH) {
      result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
   }
   if(solved && (whichType == TX_LEASE_CLTV || whichType == TX_LEASE || whichType == TX_COLDSTAKE))
      sigdata.scriptSig = CScript(result[0].begin(), result[0].end());
   else
      sigdata.scriptSig = PushAll(result);

   //allow checklocktime for leasing clvt trx
   if(whichType == TX_LEASE_CLTV)
      flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;

   // Test solution
   sigdata.complete = solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, flags, creator.Checker());
   return sigdata.complete;
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
            //TO_FIX: set corrrect parameters
            if (TransactionSignatureChecker(&txTo, nIn,0, MissingDataBehavior::ASSERT_FAIL).CheckECDSASignature(sig, pubkey, scriptPubKey,SigVersion::BASE))
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
        case TX_LEASE_CLTV:
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
    EvalScript(stack1, scriptSig1, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(),SigVersion::BASE);
    std::vector<valtype> stack2;
    EvalScript(stack2, scriptSig2, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);

    return CombineSignatures(scriptPubKey, txTo, nIn, txType, vSolutions, stack1, stack2);
}

bool IsSolvable(const CKeyStore& store, const CScript& script, bool fColdStaking)
{
    // This check is to make sure that the script we created can actually be solved for and signed by us
    // if we were to have the private keys. This is just to make sure that the script is valid and that,
    // if found in a transaction, we would still accept and relay that transaction. In particular,
    BTC::DummySignatureCreator creator(&store);
    SignatureData sigs;
    if (BTC::ProduceSignature(creator, script, sigs, fColdStaking)) {
        // VerifyScript check is just defensive, and should never fail.
        assert(VerifyScript(sigs.scriptSig, script, nullptr, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker(), nullptr));
        return true;
    }
    return false;
}

namespace BTC {
    namespace
    {
        static CScript PushAll(const std::vector<valtype>& values)
        {
            CScript result;
            for (const valtype& v : values) {
                if (v.size() == 0) {
                    result << OP_0;
                } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
                    result << CScript::EncodeOP_N(v[0]);
                } else {
                    result << v;
                }
            }
            return result;
        }

        struct Stacks
        {
            std::vector<valtype> script;
            std::vector<valtype> witness;

            Stacks() {}
            explicit Stacks(const std::vector<valtype>& scriptSigStack_) : script(scriptSigStack_), witness() {}
            explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
                EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
            }

            SignatureData Output() const {
                SignatureData result;
                result.scriptSig = BTC::PushAll(script);
                result.scriptWitness.stack = witness;
                return result;
            }
        };
    }



    TransactionSignatureCreator::TransactionSignatureCreator(const CKeyStore* keystoreIn, const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) :
    BaseSignatureCreator(keystoreIn), txTo(txToIn), nIn(nInIn), nHashType(nHashTypeIn), amount(amountIn), checker(txTo, nIn, amountIn, MissingDataBehavior::ASSERT_FAIL) {}

    bool TransactionSignatureCreator::CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
    {
        CKey key;
        if (!keystore->GetKey(address, key))
            return false;

        // Signing with uncompressed keys is disabled in witness scripts
        if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
            return false;

        uint256 hash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, sigversion);
        if (!key.Sign(hash, vchSig))
            return false;
        vchSig.push_back((unsigned char)nHashType);
        return true;
    }

    static bool Sign1(const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
    {
        std::vector<unsigned char> vchSig;
        if (!creator.CreateSig(vchSig, address, scriptCode, sigversion))
            return false;
        ret.push_back(vchSig);
        return true;
    }

    static bool SignN(const std::vector<valtype>& multisigdata, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
    {
        int nSigned = 0;
        int nRequired = multisigdata.front()[0];
        for (unsigned int i = 1; i < multisigdata.size()-1 && nSigned < nRequired; i++)
        {
            const valtype& pubkey = multisigdata[i];
            CKeyID keyID = CPubKey(pubkey).GetID();
            if (Sign1(keyID, creator, scriptCode, ret, sigversion))
                ++nSigned;
        }
        return nSigned==nRequired;
    }


    static std::vector<valtype> CombineMultisig(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                                                const std::vector<valtype>& vSolutions,
                                                const std::vector<valtype>& sigs1, const std::vector<valtype>& sigs2, SigVersion sigversion)
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

                if (checker.CheckECDSASignature(sig, pubkey, scriptPubKey, sigversion))
                {
                    sigs[pubkey] = sig;
                    break;
                }
            }
        }
        // Now build a merged CScript:
        unsigned int nSigsHave = 0;
        std::vector<valtype> result; result.push_back(valtype()); // pop-one-too-many workaround
        for (unsigned int i = 0; i < nPubKeys && nSigsHave < nSigsRequired; i++)
        {
            if (sigs.count(vSolutions[i+1]))
            {
                result.push_back(sigs[vSolutions[i+1]]);
                ++nSigsHave;
            }
        }
        // Fill any missing with OP_0:
        for (unsigned int i = nSigsHave; i < nSigsRequired; i++)
            result.push_back(valtype());

        return result;
    }

    static Stacks CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                                    const txnouttype txType, const std::vector<valtype>& vSolutions,
                                    Stacks sigs1, Stacks sigs2, SigVersion sigversion)
    {
        switch (txType)
        {
            case TX_NONSTANDARD:
            case TX_NULL_DATA:
            case TX_WITNESS_UNKNOWN:
                // Don't know anything about this, assume bigger one is correct:
                if (sigs1.script.size() >= sigs2.script.size())
                    return sigs1;
                return sigs2;
            case TX_PUBKEY:
            case TX_PUBKEYHASH:
            case TX_COLDSTAKE:
            case TX_LEASE_CLTV:
            case TX_LEASE:
            case TX_LEASINGREWARD:
                // Signatures are bigger than placeholders or empty scripts:
                if (sigs1.script.empty() || sigs1.script[0].empty())
                    return sigs2;
                return sigs1;
            case TX_WITNESS_V0_KEYHASH:
                // Signatures are bigger than placeholders or empty scripts:
                if (sigs1.witness.empty() || sigs1.witness[0].empty())
                    return sigs2;
                return sigs1;
            case TX_SCRIPTHASH:
                if (sigs1.script.empty() || sigs1.script.back().empty())
                    return sigs2;
                else if (sigs2.script.empty() || sigs2.script.back().empty())
                    return sigs1;
                else
                {
                    // Recur to combine:
                    valtype spk = sigs1.script.back();
                    CScript pubKey2(spk.begin(), spk.end());

                    txnouttype txType2;
                    std::vector<std::vector<unsigned char> > vSolutions2;
                    Solver(pubKey2, txType2, vSolutions2);
                    sigs1.script.pop_back();
                    sigs2.script.pop_back();
                    Stacks result = CombineSignatures(pubKey2, checker, txType2, vSolutions2, sigs1, sigs2, sigversion);
                    result.script.push_back(spk);
                    return result;
                }
            case TX_MULTISIG:
                return Stacks(CombineMultisig(scriptPubKey, checker, vSolutions, sigs1.script, sigs2.script, sigversion));
            case TX_WITNESS_V0_SCRIPTHASH:
                if (sigs1.witness.empty() || sigs1.witness.back().empty())
                    return sigs2;
                else if (sigs2.witness.empty() || sigs2.witness.back().empty())
                    return sigs1;
                else
                {
                    // Recur to combine:
                    CScript pubKey2(sigs1.witness.back().begin(), sigs1.witness.back().end());
                    txnouttype txType2;
                    std::vector<valtype> vSolutions2;
                    Solver(pubKey2, txType2, vSolutions2);
                    sigs1.witness.pop_back();
                    sigs1.script = sigs1.witness;
                    sigs1.witness.clear();
                    sigs2.witness.pop_back();
                    sigs2.script = sigs2.witness;
                    sigs2.witness.clear();
                    Stacks result = CombineSignatures(pubKey2, checker, txType2, vSolutions2, sigs1, sigs2, SigVersion::WITNESS_V0);
                    result.witness = result.script;
                    result.script.clear();
                    result.witness.push_back(valtype(pubKey2.begin(), pubKey2.end()));
                    return result;
                }
            default:
                return Stacks();
        }
    }

    SignatureData CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                                    const SignatureData& scriptSig1, const SignatureData& scriptSig2)
    {
        txnouttype txType;
        std::vector<std::vector<unsigned char> > vSolutions;
        Solver(scriptPubKey, txType, vSolutions);

        return CombineSignatures(scriptPubKey, checker, txType, vSolutions, Stacks(scriptSig1), Stacks(scriptSig2), SigVersion::BASE).Output();
    }

    void UpdateTransaction(CMutableTransaction& tx, unsigned int nIn, const SignatureData& data)
    {
        assert(tx.vin.size() > nIn);
        tx.vin[nIn].scriptSig = data.scriptSig;
        tx.vin[nIn].scriptWitness = data.scriptWitness;
    }

    SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn)
    {
        SignatureData data;
        assert(tx.vin.size() > nIn);
        data.scriptSig = tx.vin[nIn].scriptSig;
        data.scriptWitness = tx.vin[nIn].scriptWitness;
        return data;
    }

    /**
  * Sign scriptPubKey using signature made with creator.
  * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
  * unless whichTypeRet is TxoutType::SCRIPTHASH, in which case scriptSigRet is the redemption script.
  * Returns false if scriptPubKey could not be completely satisfied.
  */
    static bool SignStep(const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                         std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion, bool fColdStake = false,
                         bool fLeasing = false, bool fForceLeaserSign = false)
    {
        CScript scriptRet;
        uint160 h160;
        ret.clear();

        std::vector<valtype> vSolutions;
        if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
            return false;

        CKeyID keyID;
        switch (whichTypeRet)
        {
            case TX_NONSTANDARD:
            case TX_NULL_DATA:
            case TX_WITNESS_UNKNOWN:
            case TX_ZEROCOINMINT:
                return false;
            case TX_PUBKEY:
                keyID = CPubKey(vSolutions[0]).GetID();
                return BTC::Sign1(keyID, creator, scriptPubKey, ret, sigversion);
            case TX_PUBKEYHASH:
                keyID = CKeyID(uint160(vSolutions[0]));
                if (!BTC::Sign1(keyID, creator, scriptPubKey, ret, sigversion))
                    return false;
                else
                {
                    CPubKey vch;
                    creator.KeyStore().GetPubKey(keyID, vch);
                    ret.push_back(ToByteVector(vch));
                }
                return true;
            case TX_SCRIPTHASH:
                if (creator.KeyStore().GetCScript(CScriptID(uint160(vSolutions[0])), scriptRet)) {
                    ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                    return true;
                }
                return false;

            case TX_MULTISIG:
                ret.push_back(valtype()); // workaround CHECKMULTISIG bug
                return (BTC::SignN(vSolutions, creator, scriptPubKey, ret, sigversion));

            case TX_WITNESS_V0_KEYHASH:
                ret.push_back(vSolutions[0]);
                return true;

            case TX_WITNESS_V0_SCRIPTHASH:
                CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160.begin());
                if (creator.KeyStore().GetCScript(CScriptID(h160), scriptRet)) {
                    ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                    return true;
                }
                return false;

            case TX_COLDSTAKE: {
                if (fColdStake) {
                    // sign with the cold staker key
                    keyID = CKeyID(uint160(vSolutions[0]));
                } else {
                    // sign with the owner key
                    keyID = CKeyID(uint160(vSolutions[1]));
                }
                //if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
                if(!BTC::Sign1(keyID, creator, scriptPubKey, ret, sigversion))
                    return error("*** %s: failed to sign with the %s key.",
                                 __func__, fColdStake ? "cold staker" : "owner");
                CPubKey vch;
                if (!creator.KeyStore().GetPubKey(keyID, vch))
                    return error("%s : Unable to get public key from keyID", __func__);
                scriptRet << ret[0];
                scriptRet << (fColdStake ? (int)OP_TRUE : OP_FALSE) << ToByteVector(vch);
                ret.clear();
                ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                return true;
            }

            case TX_LEASE_CLTV:
            case TX_LEASE: {
                if (fLeasing) {
                    if (!fForceLeaserSign) {
                        LogPrintf("*** solver met leaser, who can't sign \n");
                        return false;
                    }
                    // sign with the leaser key
                    keyID = (whichTypeRet == TX_LEASE) ? CKeyID(uint160(vSolutions[0])): CKeyID(uint160(vSolutions[1]));
                } else {
                    // sign with the owner key
                    keyID = (whichTypeRet == TX_LEASE) ? CKeyID(uint160(vSolutions[1])): CKeyID(uint160(vSolutions[2]));
                }
                //if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet, ret))
                if(!BTC::Sign1(keyID, creator, scriptPubKey, ret, sigversion))
                    return error("*** %s: failed to sign with the %s key.",
                                 __func__, fLeasing ? "leaser" : "owner");
                CPubKey vch;
                if (!creator.KeyStore().GetPubKey(keyID, vch))
                    return error("%s : Unable to get public key from keyID", __func__);

                scriptRet << ret[0];
                scriptRet << (fLeasing ? (int)OP_TRUE : OP_FALSE) << ToByteVector(vch);
                ret.clear();
                ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
                return true;
            }

            case TX_LEASINGREWARD: {
                // 0. TRXHASH
                // 1. N
                keyID = CKeyID(uint160(vSolutions[2]));
                //if (!Sign1(keyID, keystore, hash, nHashType, scriptSigRet,ret))
                if(!BTC::Sign1(keyID, creator, scriptPubKey, ret, sigversion))
                {
                    LogPrintf("*** solver failed to sign leasing reward \n");
                    return false;
                }
                else
                {
                    CPubKey vch;
                    if (!creator.KeyStore().GetPubKey(keyID, vch))
                        return error("%s : Unable to get public key from keyID for leasing reward" , __func__);
                    ret.push_back(ToByteVector(vch));
                }
                return true;
            }

            default:
                return false;
        }
    }


    bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata, bool fColdStake, bool fLeasing, bool fForceLeaserSign)
    {
        CScript script = fromPubKey;
        std::vector<valtype> result;
        txnouttype whichType;
        bool solved = BTC::SignStep(creator, script, result, whichType, SigVersion::BASE, fColdStake, fLeasing, fForceLeaserSign);
        bool P2SH = false;
        CScript subscript;
        sigdata.scriptWitness.stack.clear();
        int flags = STANDARD_SCRIPT_VERIFY_FLAGS;

        if (solved && whichType == TX_SCRIPTHASH)
        {
            // Solver returns the subscript that needs to be evaluated;
            // the final scriptSig is the signatures from that
            // and then the serialized subscript:
            script = subscript = CScript(result[0].begin(), result[0].end());
            solved = solved && BTC::SignStep(creator, script, result, whichType, SigVersion::BASE) && whichType != TX_SCRIPTHASH;
            P2SH = true;
        }

        if (solved && whichType == TX_WITNESS_V0_KEYHASH)
        {
            CScript witnessscript;
            witnessscript << OP_DUP << OP_HASH160 << ToByteVector(result[0]) << OP_EQUALVERIFY << OP_CHECKSIG;
            txnouttype subType;
            solved = solved && BTC::SignStep(creator, witnessscript, result, subType, SigVersion::WITNESS_V0);
            sigdata.scriptWitness.stack = result;
            result.clear();
        }
        else if (solved && whichType == TX_WITNESS_V0_SCRIPTHASH)
        {
            CScript witnessscript(result[0].begin(), result[0].end());
            txnouttype subType;
            solved = solved && BTC::SignStep(creator, witnessscript, result, subType, SigVersion::WITNESS_V0) && subType != TX_SCRIPTHASH && subType != TX_WITNESS_V0_SCRIPTHASH && subType != TX_WITNESS_V0_KEYHASH;
            result.push_back(std::vector<unsigned char>(witnessscript.begin(), witnessscript.end()));
            sigdata.scriptWitness.stack = result;
            result.clear();
        }

        if (P2SH) {
            result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
        }
        if(solved && (whichType == TX_LEASE_CLTV || whichType == TX_LEASE || whichType == TX_COLDSTAKE))
            sigdata.scriptSig = CScript(result[0].begin(), result[0].end());
        else
            sigdata.scriptSig = PushAll(result);

        //allow checklocktime for leasing clvt trx
        if(whichType == TX_LEASE_CLTV)
            flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;

        // Test solution
        return solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, flags, creator.Checker());
    }

    bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata, SigVersion sigversion, bool fColdStake, ScriptError* serror)
    {
        CScript script = fromPubKey;
        bool solved = true;
        std::vector<valtype> result;
        txnouttype whichType;
        solved = SignStep(creator, script, result, whichType, sigversion, fColdStake);
        CScript subscript;

        if (solved && whichType == TX_SCRIPTHASH)
        {
            // Solver returns the subscript that needs to be evaluated;
            // the final scriptSig is the signatures from that
            // and then the serialized subscript:
            script = subscript = CScript(result[0].begin(), result[0].end());
            solved = solved && SignStep(creator, script, result, whichType, sigversion, fColdStake) && whichType != TX_SCRIPTHASH;
            result.emplace_back(subscript.begin(), subscript.end());
        }

        sigdata.scriptSig = PushAll(result);

        // Test solution
        return solved && VerifyScript(sigdata.scriptSig, fromPubKey, nullptr, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker(), serror);
    }

    namespace {
/** Dummy signature checker which accepts all signatures. */
        class DummySignatureChecker : public BaseSignatureChecker
        {
        public:
            DummySignatureChecker() {}

            bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
            {
                return true;
            }
        };
        const DummySignatureChecker dummyChecker;
    } // namespace

    const BaseSignatureChecker& DummySignatureCreator::Checker() const
    {
        return dummyChecker;
    }

    bool DummySignatureCreator::CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const
    {
        // Create a dummy signature that is a valid DER-encoding
        vchSig.assign(72, '\000');
        vchSig[0] = 0x30;
        vchSig[1] = 69;
        vchSig[2] = 0x02;
        vchSig[3] = 33;
        vchSig[4] = 0x01;
        vchSig[4 + 33] = 0x02;
        vchSig[5 + 33] = 32;
        vchSig[6 + 33] = 0x01;
        vchSig[6 + 33 + 32] = SIGHASH_ALL;
        return true;
    }

    bool IsSolvable(const CKeyStore& store, const CScript& script)
    {
        // This check is to make sure that the script we created can actually be solved for and signed by us
        // if we were to have the private keys. This is just to make sure that the script is valid and that,
        // if found in a transaction, we would still accept and relay that transaction. In particular,
        // it will reject witness outputs that require signing with an uncompressed public key.
        DummySignatureCreator creator(&store);
        SignatureData sigs;
        // Make sure that STANDARD_SCRIPT_VERIFY_FLAGS includes SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, the most
        // important property this function is designed to test for.
        static_assert(STANDARD_SCRIPT_VERIFY_FLAGS & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, "IsSolvable requires standard script flags to include WITNESS_PUBKEYTYPE");
        if (ProduceSignature(creator, script, sigs)) {
            // VerifyScript check is just defensive, and should never fail.
            assert(VerifyScript(sigs.scriptSig, script, &sigs.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker()));
            return true;
        }
        return false;
    }
}

void UpdateInput(CTxIn& input, const SignatureData& data)
{
   input.scriptSig = data.scriptSig;
   input.scriptWitness = data.scriptWitness;
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

bool SignSignature(const CKeyStore &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType)
{
   assert(nIn < txTo.vin.size());

   MutableTransactionSignatureCreator creator(&txTo, nIn, amount, nHashType);

   SignatureData sigdata;
   bool ret = ProduceSignature(provider, creator, fromPubKey, sigdata, false, false, false);
   UpdateInput(txTo.vin.at(nIn), sigdata);
   return ret;
}

bool SignSignature(const CKeyStore &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
   assert(nIn < txTo.vin.size());
   const CTxIn& txin = txTo.vin[nIn];
   assert(txin.prevout.n < txFrom.vout.size());
   const CTxOut& txout = txFrom.vout[txin.prevout.n];

   return SignSignature(provider, txout.scriptPubKey, txTo, nIn, txout.nValue, nHashType);
}

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

const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR = DummySignatureCreator(32, 32);
const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR = DummySignatureCreator(33, 32);

bool IsSolvable(const CKeyStore& provider, const CScript& script)
{
   // This check is to make sure that the script we created can actually be solved for and signed by us
   // if we were to have the private keys. This is just to make sure that the script is valid and that,
   // if found in a transaction, we would still accept and relay that transaction. In particular,
   // it will reject witness outputs that require signing with an uncompressed public key.
   SignatureData sigs;
   // Make sure that STANDARD_SCRIPT_VERIFY_FLAGS includes SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, the most
   // important property this function is designed to test for.
   static_assert(STANDARD_SCRIPT_VERIFY_FLAGS & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, "IsSolvable requires standard script flags to include WITNESS_PUBKEYTYPE");
   if (ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, script, sigs, false, false, false)) {
      // VerifyScript check is just defensive, and should never fail.
      bool verified = VerifyScript(sigs.scriptSig, script, &sigs.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER);
      assert(verified);
      return true;
   }
   return false;
}

bool IsSegWitOutput(const CKeyStore& provider, const CScript& script)
{
   int version;
   valtype program;
   if (script.IsWitnessProgram(version, program)) return true;
   if (script.IsPayToScriptHash()) {
      std::vector<valtype> solutions;
      txnouttype whichtype;
      if (!Solver(script, whichtype, solutions))
         return false;
      if (whichtype == TX_SCRIPTHASH) {
         auto h160 = uint160(solutions[0]);
         CScript subscript;
         if (provider.GetCScript(CScriptID{h160}, subscript)) {
            if (subscript.IsWitnessProgram(version, program)) return true;
         }
      }
   }
   return false;
}

/* Wrapper around ProduceSignature to combine two scriptsigs */
SignatureData CombineSignatures(const CKeyStore& provider, const CTxOut& txout, const CMutableTransaction& tx, const SignatureData& scriptSig1, const SignatureData& scriptSig2)
{
   SignatureData data;
   data.MergeSignatureData(scriptSig1);
   data.MergeSignatureData(scriptSig2);
   ProduceSignature(provider, MutableTransactionSignatureCreator(&tx, 0, txout.nValue, SIGHASH_DEFAULT), txout.scriptPubKey, data, false, false, false);
   return data;
}

namespace {
    class SignatureExtractorChecker final : public DeferringSignatureChecker
    {
    private:
        SignatureData& sigdata;

    public:
        SignatureExtractorChecker(SignatureData& sigdata, BaseSignatureChecker& checker) : DeferringSignatureChecker(checker), sigdata(sigdata) {}

        bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
        {
            if (m_checker.CheckECDSASignature(scriptSig, vchPubKey, scriptCode, sigversion)) {
                CPubKey pubkey(vchPubKey);
                sigdata.signatures.emplace(pubkey.GetID(), SigPair(pubkey, scriptSig));
                return true;
            }
            return false;
        }
    };

    struct Stacks
    {
        std::vector<valtype> script;
        std::vector<valtype> witness;

        Stacks() = delete;
        Stacks(const Stacks&) = delete;
        explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
            EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
        }
    };
}
// Extracts signatures and scripts from incomplete scriptSigs. Please do not extend this, use PSBT instead
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout)
{
    SignatureData data;
    assert(tx.vin.size() > nIn);
    data.scriptSig = tx.vin[nIn].scriptSig;
    data.scriptWitness = tx.vin[nIn].scriptWitness;
    Stacks stack(data);

    // Get signatures
    MutableTransactionSignatureChecker tx_checker(&tx, nIn, txout.nValue, MissingDataBehavior::FAIL);
    SignatureExtractorChecker extractor_checker(data, tx_checker);
    if (VerifyScript(data.scriptSig, txout.scriptPubKey, &data.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, extractor_checker)) {
        data.complete = true;
        return data;
    }

    // Get scripts
    std::vector<std::vector<unsigned char>> solutions;
    txnouttype script_type;
    if (!Solver(txout.scriptPubKey, script_type, solutions))
        return data;

    SigVersion sigversion = SigVersion::BASE;
    CScript next_script = txout.scriptPubKey;

    if (script_type == TX_SCRIPTHASH && !stack.script.empty() && !stack.script.back().empty()) {
        // Get the redeemScript
        CScript redeem_script(stack.script.back().begin(), stack.script.back().end());
        data.redeem_script = redeem_script;
        next_script = std::move(redeem_script);

        // Get redeemScript type
        if (!Solver(next_script, script_type, solutions))
            return data;
        stack.script.pop_back();
    }
    if (script_type == TX_WITNESS_V0_SCRIPTHASH && !stack.witness.empty() && !stack.witness.back().empty()) {
        // Get the witnessScript
        CScript witness_script(stack.witness.back().begin(), stack.witness.back().end());
        data.witness_script = witness_script;
        next_script = std::move(witness_script);

        // Get witnessScript type
        if (!Solver(next_script, script_type, solutions))
            return data;
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
                if (data.signatures.count(CPubKey(pubkey).GetID()) || extractor_checker.CheckECDSASignature(sig, pubkey, next_script, sigversion)) {
                    last_success_key = i + 1;
                    break;
                }
            }
        }
    }

    return data;
}

/*
bool SignTransaction(CMutableTransaction& mtx, const SigningProvider* keystore, const std::map<COutPoint, Coin>& coins, int nHashType, std::map<int, bilingual_str>& input_errors)
{
   bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

   // Use CTransaction for the constant parts of the
   // transaction to avoid rehashing.
   const CTransaction txConst(mtx);

   PrecomputedTransactionData txdata;
   std::vector<CTxOut> spent_outputs;
   for (unsigned int i = 0; i < mtx.vin.size(); ++i) {
      CTxIn& txin = mtx.vin[i];
      auto coin = coins.find(txin.prevout);
      if (coin == coins.end() || coin->second.IsSpent()) {
         txdata.Init(txConst,  {},  true);
         break;
      } else {
         spent_outputs.emplace_back(coin->second.out.nValue, coin->second.out.scriptPubKey);
      }
   }

   bool DummySignatureCreator::CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const
   {
      // Create a dummy signature that is a valid DER-encoding
      vchSig.assign(72, '\000');
      vchSig[0] = 0x30;
      vchSig[1] = 69;
      vchSig[2] = 0x02;
      vchSig[3] = 33;
      vchSig[4] = 0x01;
      vchSig[4 + 33] = 0x02;
      vchSig[5 + 33] = 32;
      vchSig[6 + 33] = 0x01;
      vchSig[6 + 33 + 32] = SIGHASH_ALL;
      return true;
   }

   // Sign what we can:
   for (unsigned int i = 0; i < mtx.vin.size(); ++i) {
      CTxIn& txin = mtx.vin[i];
      auto coin = coins.find(txin.prevout);
      if (coin == coins.end() || coin->second.IsSpent()) {
         input_errors[i] = _("Input not found or already spent");
         continue;
      }
      const CScript& prevPubKey = coin->second.out.scriptPubKey;
      const CAmount& amount = coin->second.out.nValue;

      SignatureData sigdata = DataFromTransaction(mtx, i, coin->second.out);
      // Only sign SIGHASH_SINGLE if there's a corresponding output:
      if (!fHashSingle || (i < mtx.vout.size())) {
         ProduceSignature(*keystore, MutableTransactionSignatureCreator(&mtx, i, amount, &txdata, nHashType), prevPubKey, sigdata);
      }

      UpdateInput(txin, sigdata);

      // amount must be specified for valid segwit signature
      if (amount == MAX_MONEY_OUT && !txin.scriptWitness.IsNull()) {
         input_errors[i] = _("Missing amount");
         continue;
      }

      ScriptError serror = SCRIPT_ERR_OK;
      if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, amount, txdata, MissingDataBehavior::FAIL), &serror)) {
         if (serror == SCRIPT_ERR_INVALID_STACK_OPERATION) {
            // Unable to sign input and verification failed (possible attempt to partially sign).
            input_errors[i] = Untranslated("Unable to sign input, invalid stack size (possibly missing key)");
         } else if (serror == SCRIPT_ERR_SIG_NULLFAIL) {
            // Verification failed (possibly due to insufficient signatures).
            input_errors[i] = Untranslated("CHECK(MULTI)SIG failing with non-zero signature (possibly need more signatures)");
         } else {
            input_errors[i] = Untranslated(ScriptErrorString(serror));
         }
      } else {
         // If this input succeeds, make sure there is no error set for it
         input_errors.erase(i);
      }
   }
   return input_errors.empty();
}
*/