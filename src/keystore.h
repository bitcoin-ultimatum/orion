// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "key.h"
#include "pubkey.h"
#include "sync.h"
#include "script/standard.h"
#include "sapling/address.h"
#include "sapling/zip32.h"

#include <boost/signals2/signal.hpp>

class CScript;
class CScriptID;

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey& key, const CPubKey& pubkey) = 0;
    virtual bool AddKey(const CKey& key);

    //! Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID& address) const = 0;
    virtual bool GetKey(const CKeyID& address, CKey& keyOut) const = 0;
    virtual void GetKeys(std::set<CKeyID>& setAddress) const = 0;
    virtual bool GetPubKey(const CKeyID& address, CPubKey& vchPubKeyOut) const;

    //! Support for BIP 0013 : see https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript& redeemScript) = 0;
    virtual bool HaveCScript(const CScriptID& hash) const = 0;
    virtual bool GetCScript(const CScriptID& hash, CScript& redeemScriptOut) const = 0;
    virtual bool GetCScripts(std::set<CScriptID>& scripts) const = 0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript& dest) = 0;
    virtual bool RemoveWatchOnly(const CScript& dest) = 0;
    virtual bool HaveWatchOnly(const CScript& dest) const = 0;
    virtual bool HaveWatchOnly() const = 0;

    //! Support for MultiSig addresses
    virtual bool AddMultiSig(const CScript& dest) = 0;
    virtual bool RemoveMultiSig(const CScript& dest) = 0;
    virtual bool HaveMultiSig(const CScript& dest) const = 0;
    virtual bool HaveMultiSig() const = 0;


    //! Support for Sapling
    // Add a Sapling spending key to the store.
    virtual bool AddSaplingSpendingKey(const libzcash::SaplingExtendedSpendingKey &sk) = 0;

    // Check whether a Sapling spending key corresponding to a given Sapling viewing key is present in the store.
    virtual bool HaveSaplingSpendingKey(
            const libzcash::SaplingExtendedFullViewingKey &extfvk) const = 0;
    virtual bool GetSaplingSpendingKey(
            const libzcash::SaplingExtendedFullViewingKey &extfvk,
            libzcash::SaplingExtendedSpendingKey& skOut) const = 0;

    //! Support for Sapling full viewing keys
    virtual bool AddSaplingFullViewingKey(const libzcash::SaplingExtendedFullViewingKey &extfvk) = 0;
    virtual bool HaveSaplingFullViewingKey(const libzcash::SaplingIncomingViewingKey &ivk) const = 0;
    virtual bool GetSaplingFullViewingKey(
            const libzcash::SaplingIncomingViewingKey &ivk,
            libzcash::SaplingExtendedFullViewingKey& extfvkOut) const = 0;
    virtual void GetSaplingPaymentAddresses(std::set<libzcash::SaplingPaymentAddress> &setAddress) const = 0;

    //! Sapling incoming viewing keys
    virtual bool AddSaplingIncomingViewingKey(
            const libzcash::SaplingIncomingViewingKey &ivk,
            const libzcash::SaplingPaymentAddress &addr) = 0;
    virtual bool HaveSaplingIncomingViewingKey(const libzcash::SaplingPaymentAddress &addr) const = 0;
    virtual bool GetSaplingIncomingViewingKey(
            const libzcash::SaplingPaymentAddress &addr,
            libzcash::SaplingIncomingViewingKey& ivkOut) const = 0;

};

typedef std::map<CKeyID, CKey> KeyMap;
typedef std::map<CScriptID, CScript> ScriptMap;
typedef std::set<CScript> WatchOnlySet;
typedef std::set<CScript> MultiSigScriptSet;

// Full viewing key has equivalent functionality to a transparent address
// When encrypting wallet, encrypt SaplingSpendingKeyMap, while leaving SaplingFullViewingKeyMap unencrypted
typedef std::map<
        libzcash::SaplingExtendedFullViewingKey,
        libzcash::SaplingExtendedSpendingKey> SaplingSpendingKeyMap;
typedef std::map<
        libzcash::SaplingIncomingViewingKey,
        libzcash::SaplingExtendedFullViewingKey> SaplingFullViewingKeyMap;
// Only maps from default addresses to ivk, may need to be reworked when adding diversified addresses.
typedef std::map<libzcash::SaplingPaymentAddress, libzcash::SaplingIncomingViewingKey> SaplingIncomingViewingKeyMap;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    ScriptMap mapScripts;
    WatchOnlySet setWatchOnly;
    MultiSigScriptSet setMultiSig;

public:
    // todo future: Move every Sapling map to the new sspkm box.
    SaplingSpendingKeyMap mapSaplingSpendingKeys;
    SaplingFullViewingKeyMap mapSaplingFullViewingKeys;
    SaplingIncomingViewingKeyMap mapSaplingIncomingViewingKeys;

    bool AddKeyPubKey(const CKey& key, const CPubKey& pubkey);
    bool HaveKey(const CKeyID& address) const;
    void GetKeys(std::set<CKeyID>& setAddress) const;
    bool GetKey(const CKeyID& address, CKey& keyOut) const;

    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID& hash) const;
    virtual bool GetCScript(const CScriptID& hash, CScript& redeemScriptOut) const;
    virtual bool GetCScripts(std::set<CScriptID>& scripts) const;

    virtual bool AddWatchOnly(const CScript& dest);
    virtual bool RemoveWatchOnly(const CScript& dest);
    virtual bool HaveWatchOnly(const CScript& dest) const;
    virtual bool HaveWatchOnly() const;

    virtual bool AddMultiSig(const CScript& dest);
    virtual bool RemoveMultiSig(const CScript& dest);
    virtual bool HaveMultiSig(const CScript& dest) const;
    virtual bool HaveMultiSig() const;

    //! Sapling
    bool AddSaplingSpendingKey(const libzcash::SaplingExtendedSpendingKey &sk);
    bool HaveSaplingSpendingKey(const libzcash::SaplingExtendedFullViewingKey &extfvk) const;
    bool GetSaplingSpendingKey(const libzcash::SaplingExtendedFullViewingKey &extfvk, libzcash::SaplingExtendedSpendingKey &skOut) const;

    virtual bool AddSaplingFullViewingKey(const libzcash::SaplingExtendedFullViewingKey &extfvk);
    virtual bool HaveSaplingFullViewingKey(const libzcash::SaplingIncomingViewingKey &ivk) const;
    virtual bool GetSaplingFullViewingKey(
            const libzcash::SaplingIncomingViewingKey &ivk,
            libzcash::SaplingExtendedFullViewingKey& extfvkOut) const;
    virtual bool AddSaplingIncomingViewingKey(
            const libzcash::SaplingIncomingViewingKey &ivk,
            const libzcash::SaplingPaymentAddress &addr);
    virtual bool HaveSaplingIncomingViewingKey(const libzcash::SaplingPaymentAddress &addr) const;
    virtual bool GetSaplingIncomingViewingKey(
            const libzcash::SaplingPaymentAddress &addr,
            libzcash::SaplingIncomingViewingKey& ivkOut) const;

    bool GetSaplingExtendedSpendingKey(
            const libzcash::SaplingPaymentAddress &addr,
            libzcash::SaplingExtendedSpendingKey &extskOut) const;

    void GetSaplingPaymentAddresses(std::set<libzcash::SaplingPaymentAddress> &setAddress) const;
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char> > > CryptedKeyMap;

/** Return the CKeyID of the key involved in a script (if there is a unique one). */
CKeyID GetKeyForDestination(const CKeyStore& store, const CTxDestination& dest);

//! Sapling
typedef std::map<libzcash::SaplingExtendedFullViewingKey, std::vector<unsigned char> > CryptedSaplingSpendingKeyMap;

#endif // BITCOIN_KEYSTORE_H
