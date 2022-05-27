// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"

#include "crypter.h"
#include "key.h"
#include "script/script.h"
#include "util.h"


bool CKeyStore::GetPubKey(const CKeyID& address, CPubKey& vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key))
        return false;
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CKeyStore::AddKey(const CKey& key)
{
    return AddKeyPubKey(key, key.GetPubKey());
}

bool CBasicKeyStore::AddKeyPubKey(const CKey& key, const CPubKey& pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    return true;
}

bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("CBasicKeyStore::AddCScript() : redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

bool CBasicKeyStore::GetCScript(const CScriptID& hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end()) {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

bool CBasicKeyStore::GetCScripts(std::set<CScriptID>& scripts) const
{
    LOCK(cs_KeyStore);
    scripts.clear();
    for (auto& v: mapScripts) {
        scripts.insert(v.first);
    }
    return !scripts.empty();
}

bool CBasicKeyStore::AddWatchOnly(const CScript& dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript& dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript& dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}

bool CBasicKeyStore::AddMultiSig(const CScript& dest)
{
    LOCK(cs_KeyStore);
    setMultiSig.insert(dest);
    return true;
}

bool CBasicKeyStore::RemoveMultiSig(const CScript& dest)
{
    LOCK(cs_KeyStore);
    setMultiSig.erase(dest);
    return true;
}

bool CBasicKeyStore::HaveMultiSig(const CScript& dest) const
{
    LOCK(cs_KeyStore);
    return setMultiSig.count(dest) > 0;
}

bool CBasicKeyStore::HaveMultiSig() const
{
    LOCK(cs_KeyStore);
    return (!setMultiSig.empty());
}

bool CBasicKeyStore::HaveKey(const CKeyID& address) const
{
    bool result;
    {
        LOCK(cs_KeyStore);
        result = (mapKeys.count(address) > 0);
    }
    return result;
}

void CBasicKeyStore::GetKeys(std::set<CKeyID>& setAddress) const
{
    setAddress.clear();
    {
        LOCK(cs_KeyStore);
        KeyMap::const_iterator mi = mapKeys.begin();
        while (mi != mapKeys.end()) {
            setAddress.insert((*mi).first);
            mi++;
        }
    }
}

bool CBasicKeyStore::GetKey(const CKeyID& address, CKey& keyOut) const
{
    {
        LOCK(cs_KeyStore);
        KeyMap::const_iterator mi = mapKeys.find(address);
        if (mi != mapKeys.end()) {
            keyOut = mi->second;
            return true;
        }
    }
    return false;
}

bool CBasicKeyStore::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
   std::pair<CPubKey, KeyOriginInfo> out;
   //bool ret = LookupHelper(origins, keyid, out);
   //if (ret) info = std::move(out.second);
   return false;//ret;
}

bool CBasicKeyStore::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
   return false;//LookupHelper(tr_spenddata, output_key, spenddata);
}

CKeyID GetKeyForDestination(const CKeyStore& store, const CTxDestination& dest)
{
   // Only supports destinations which map to single public keys, i.e. P2PKH,
   // P2WPKH, and P2SH-P2WPKH.
   if (auto id = std::get_if<PKHash>(&dest)) {
      return ToKeyID(*id);;
   }
   if (auto witness_id = std::get_if<WitnessV0KeyHash>(&dest)) {
      return ToKeyID(*witness_id);
   }
   if (auto script_hash = std::get_if<ScriptHash>(&dest)) {
      CScript script;
      CScriptID script_id(*script_hash);
      CTxDestination inner_dest;
      if (store.GetCScript(script_id, script) && ExtractDestination(script, inner_dest)) {
         if (auto inner_witness_id = std::get_if<WitnessV0KeyHash>(&inner_dest)) {
            return ToKeyID(*inner_witness_id);
         }
      }
   }
   return CKeyID();
}