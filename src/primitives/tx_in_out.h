// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_TX_IN_OUT_H
#define BTCU_TX_IN_OUT_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#include <list>

class CTransaction;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;
    
    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(*this));
    }
    
    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }
    bool IsMasternodeReward(const CTransaction* tx) const;

    void SetLeasingReward(const uint32_t nHeight) { hash.SetNull(); n = nHeight; }
    bool IsLeasingReward() const { return (hash.IsNull() && n > 0); }
    
    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }
    
    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }
    
    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }
    
    std::string ToString() const;
    std::string ToStringShort() const;
    
    uint256 GetHash();
    
};
/** An outpoint - a combination of a transaction hash and an index n into its vout */
class BaseOutPoint
{
public:
    uint256 hash;
    uint32_t n;
    bool isTransparent{true};

    BaseOutPoint() { SetNull(); }
    BaseOutPoint(const uint256& hashIn, const uint32_t nIn, bool isTransparentIn = true) :
            hash(hashIn), n(nIn), isTransparent(isTransparentIn) { }

    //SERIALIZE_METHODS(BaseOutPoint, obj) { READWRITE(obj.hash, obj.n); }
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const BaseOutPoint& a, const BaseOutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const BaseOutPoint& a, const BaseOutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const BaseOutPoint& a, const BaseOutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    std::string ToStringShort() const;

    size_t DynamicMemoryUsage() const { return 0; }

};

/** An outpoint - a combination of a transaction hash and an index n into its sapling
 * output description (vShieldedOutput) */
class SaplingOutPoint : public BaseOutPoint
{
public:
    SaplingOutPoint() : BaseOutPoint() {};
    SaplingOutPoint(const uint256& hashIn, const uint32_t nIn) : BaseOutPoint(hashIn, nIn, false) {};
    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScript prevPubKey;
    CScriptWitness scriptWitness; //!< Only serialized through CTransaction

   /* Setting nSequence to this value for every input in a transaction
* disables nLockTime. */
   static const uint32_t SEQUENCE_FINAL = 0xffffffff;

   /* Below flags apply in the context of BIP 68*/
   /* If this flag set, CTxIn::nSequence is NOT interpreted as a
    * relative lock-time. */
   static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1U << 31);

   /* If CTxIn::nSequence encodes a relative lock-time and this flag
    * is set, the relative lock-time has units of 512 seconds,
    * otherwise it specifies blocks with a granularity of 1. */
   static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

   /* If CTxIn::nSequence encodes a relative lock-time, this mask is
    * applied to extract that lock-time from the sequence field. */
   static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

   /* In order to use the same number of bits to encode roughly the
    * same wall-clock duration, and because blocks are naturally
    * limited to occur every 600s on average, the minimum granularity
    * for time-based relative lock-time is fixed at 512 seconds.
    * Converting from CTxIn::nSequence to seconds is performed by
    * multiplying by 512 = 2^9, or equivalently shifting up by
    * 9 bits. */
   static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;
    
    CTxIn()
    {
        nSequence = std::numeric_limits<unsigned int>::max();
    }
    
    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<unsigned int>::max());
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max());
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    }
    
    bool IsFinal() const
    {
        return (nSequence == std::numeric_limits<uint32_t>::max());
    }
    
    bool IsZerocoinSpend() const;
    bool IsZerocoinPublicSpend() const;
    bool IsLeasingReward() const { return prevout.IsLeasingReward(); }
    
    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }
    
    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }
    
    // Needed for sorting
    friend bool operator>(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout.hash > b.prevout.hash);
    }
    
    friend bool operator<(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout.hash < b.prevout.hash);
    }
    
    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    int nRounds;
    
    CTxOut()
    {
        SetNull();
    }
    
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    }
    
    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
        nRounds = -10; // an initial value, should be no way to get this by calculations
    }
    
    bool IsNull() const
    {
        return (nValue == -1);
    }
    
    void SetEmpty()
    {
        nValue = 0;
        scriptPubKey.clear();
    }
    
    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }
    
    uint256 GetHash() const;

    CAmount GetMinNotDustSize(const CFeeRate& minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee, which has units ubtcu-per-kilobyte.
        // If you'd pay more than 1/3 in fees to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will need a CTxIn of at least 148 bytes to spend
        // i.e. total is 148 + 32 = 182 bytes. Default -minrelaytxfee is 100 ubtcu per kB
        // and that means that fee per txout is 182 * 100 / 1000 = 18.2 ubtcu.
        // So dust is a txout less than 18.2 *3 = 54.6 ubtcu
        // with default -minrelaytxfee = minRelayTxFee = 100 ubtcu per kB.
        size_t nSize = GetSerializeSize(SER_DISK,0)+148u;
        return 3*minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(CFeeRate minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee, which has units ubtcu-per-kilobyte.
        // If you'd pay more than 1/3 in fees to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will need a CTxIn of at least 148 bytes to spend
        // i.e. total is 148 + 32 = 182 bytes. Default -minrelaytxfee is 100 ubtcu per kB
        // and that means that fee per txout is 182 * 100 / 1000 = 18.2 ubtcu.
        // So dust is a txout less than 18.2 *3 = 54.6 ubtcu
        // with default -minrelaytxfee = minRelayTxFee = 10000 ubtcu per kB.
        size_t nSize = GetSerializeSize(SER_DISK,0)+148u;
        return (nValue < 3*minRelayTxFee.GetFee(nSize));
    }

    bool IsPayToLeasing() const;
    bool IsLeasingReward() const;
    bool IsZerocoinMint() const;
    CAmount GetZerocoinMinted() const;
    
    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey &&
                a.nRounds      == b.nRounds);
    }
    
    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }
    
    std::string ToString() const;
};

#endif //BTCU_TX_IN_OUT_H
