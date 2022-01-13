// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin developers
// Copyright (c) 2018-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_INTERPRETER_H
#define BITCOIN_SCRIPT_INTERPRETER_H

#include "script_error.h"
#include "primitives/transaction.h"

#include <vector>
#include <stdint.h>
#include <string>
#include <span.h>

class CPubKey;
class CScript;
class CTransaction;
class uint256;

/** Signature hash types/flags */
enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,

    SIGHASH_DEFAULT = 0, //!< Taproot only; implied when sighash byte is missing, and equivalent to SIGHASH_ALL
    SIGHASH_OUTPUT_MASK = 3,
    SIGHASH_INPUT_MASK = 0x80,
};

/** Script verification flags */
enum
{
    SCRIPT_VERIFY_NONE      = 0,

    // Evaluate P2SH subscripts (softfork safe, BIP16).
    SCRIPT_VERIFY_P2SH      = (1U << 0),

    // Passing a non-strict-DER signature or one with undefined hashtype to a checksig operation causes script failure.
    // Evaluating a pubkey that is not (0x04 + 64 bytes) or (0x02 or 0x03 + 32 bytes) by checksig causes script failure.
    // (softfork safe, but not used or intended as a consensus rule).
    SCRIPT_VERIFY_STRICTENC = (1U << 1),

    // Passing a non-strict-DER signature to a checksig operation causes script failure (softfork safe, BIP62 rule 1)
    SCRIPT_VERIFY_DERSIG    = (1U << 2),

    // Passing a non-strict-DER signature or one with S > order/2 to a checksig operation causes script failure
    // (softfork safe, BIP62 rule 5).
    SCRIPT_VERIFY_LOW_S     = (1U << 3),

    // verify dummy stack item consumed by CHECKMULTISIG is of zero-length (softfork safe, BIP62 rule 7).
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4),

    // Using a non-push operator in the scriptSig causes script failure (softfork safe, BIP62 rule 2).
    SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5),

    // Require minimal encodings for all push operations (OP_0... OP_16, OP_1NEGATE where possible, direct
    // pushes up to 75 bytes, OP_PUSHDATA up to 255 bytes, OP_PUSHDATA2 for anything larger). Evaluating
    // any other push causes the script to fail (BIP62 rule 3).
    // In addition, whenever a stack element is interpreted as a number, it must be of minimal length (BIP62 rule 4).
    // (softfork safe)
    SCRIPT_VERIFY_MINIMALDATA = (1U << 6),

    // Discourage use of NOPs reserved for upgrades (NOP1-10)
    //
    // Provided so that nodes can avoid accepting or mining transactions
    // containing executed NOP's whose meaning may change after a soft-fork,
    // thus rendering the script invalid; with this flag set executing
    // discouraged NOPs fails the script. This verification flag will never be
    // a mandatory flag applied to scripts in a block. NOPs that are not
    // executed, e.g.  within an unexecuted IF ENDIF block, are *not* rejected.
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS  = (1U << 7),

    // Require that only a single stack element remains after evaluation. This changes the success criterion from
    // "At least one stack element must remain, and when interpreted as a boolean, it must be true" to
    // "Exactly one stack element must remain, and when interpreted as a boolean, it must be true".
    // (softfork safe, BIP62 rule 6)
    // Note: CLEANSTACK should never be used without P2SH.
    SCRIPT_VERIFY_CLEANSTACK = (1U << 8),

    // Verify CHECKLOCKTIMEVERIFY
    //
    // See BIP65 for details.
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9),

    // Support sender address in contract output
    //
    SCRIPT_OUTPUT_SENDER = (1U << 29),

    // Making OP_CODESEPARATOR and FindAndDelete fail any non-segwit scripts
    //
    SCRIPT_VERIFY_CONST_SCRIPTCODE = (1U << 16),

    // support CHECKSEQUENCEVERIFY opcode
    //
    // See BIP112 for details
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10),

    // Signature(s) must be empty vector if a CHECK(MULTI)SIG operation failed
    //
    SCRIPT_VERIFY_NULLFAIL = (1U << 14),

    // Public keys in segregated witness scripts must be compressed
    //
    SCRIPT_VERIFY_WITNESS_PUBKEYTYPE = (1U << 15),

    // Performs the compiled byte code
    //
    SCRIPT_EXEC_BYTE_CODE = (1U << 30),
    // Segwit script only: Require the argument of OP_IF/NOTIF to be exactly 0x01 or empty vector
    //
    SCRIPT_VERIFY_MINIMALIF = (1U << 13),
    // Support segregated witness
    //
    SCRIPT_VERIFY_WITNESS = (1U << 11),

    // Making v1-v16 witness program non-standard
    //
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM = (1U << 12),

   // Taproot/Tapscript validation (BIPs 341 & 342)
   //
   SCRIPT_VERIFY_TAPROOT = (1U << 17),

   // Making unknown Taproot leaf versions non-standard
   //
   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION = (1U << 18),

   // Making unknown OP_SUCCESS non-standard
   SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS = (1U << 19),

   // Making unknown public key versions (in BIP 342 scripts) non-standard
   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE = (1U << 20)
};

enum class SigVersion
{
   BASE = 0,        //!< Bare scripts and BIP16 P2SH-wrapped redeemscripts
   WITNESS_V0 = 1,  //!< Witness v0 (P2WPKH and P2WSH); see BIP 141
   TAPROOT = 2,     //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, key path spending; see BIP 341
   TAPSCRIPT = 3,   //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, script path spending, leaf version 0xc0; see BIP 342
};

namespace BTC
{
   struct PrecomputedTransactionData
   {
      uint256 hashPrevouts, hashSequence, hashOutputs;
      bool ready = false;

      explicit PrecomputedTransactionData(const CTransaction& tx);
   };

   uint256 SignatureHash(const CScript& scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache);
}

/** Enum to specify what *TransactionSignatureChecker's behavior should be
 *  when dealing with missing transaction data.
 */
enum class MissingDataBehavior
{
    ASSERT_FAIL,  //!< Abort execution through assertion failure (for consensus code)
    FAIL,         //!< Just act as if the signature was invalid
};

struct ScriptExecutionData
{
   //! Whether m_tapleaf_hash is initialized.
   bool m_tapleaf_hash_init = false;
   //! The tapleaf hash.
   uint256 m_tapleaf_hash;

   //! Whether m_codeseparator_pos is initialized.
   bool m_codeseparator_pos_init = false;
   //! Opcode position of the last executed OP_CODESEPARATOR (or 0xFFFFFFFF if none executed).
   uint32_t m_codeseparator_pos;

   //! Whether m_annex_present and (when needed) m_annex_hash are initialized.
   bool m_annex_init = false;
   //! Whether an annex is present.
   bool m_annex_present;
   //! Hash of the annex data.
   uint256 m_annex_hash;

   //! Whether m_validation_weight_left is initialized.
   bool m_validation_weight_left_init = false;
   //! How much validation weight is left (decremented for every successful non-empty signature check).
   int64_t m_validation_weight_left;
};

/** Signature hash sizes */
static constexpr size_t WITNESS_V0_SCRIPTHASH_SIZE = 32;
static constexpr size_t WITNESS_V0_KEYHASH_SIZE = 20;
static constexpr size_t WITNESS_V1_TAPROOT_SIZE = 32;

static constexpr uint8_t TAPROOT_LEAF_MASK = 0xfe;
static constexpr uint8_t TAPROOT_LEAF_TAPSCRIPT = 0xc0;
static constexpr size_t TAPROOT_CONTROL_BASE_SIZE = 33;
static constexpr size_t TAPROOT_CONTROL_NODE_SIZE = 32;
static constexpr size_t TAPROOT_CONTROL_MAX_NODE_COUNT = 128;
static constexpr size_t TAPROOT_CONTROL_MAX_SIZE = TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * TAPROOT_CONTROL_MAX_NODE_COUNT;

uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);

class BaseSignatureChecker
{
public:
   virtual bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
   {
      return false;
   }
   virtual bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, const ScriptExecutionData& execdata, ScriptError* serror = nullptr) const
   {
      return false;
   }
   virtual bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode) const
    {
        return false;
    }

    virtual bool CheckLockTime(const CScriptNum& nLockTime) const
    {
         return false;
    }

    virtual bool CheckColdStake(const CScript& script) const
    {
         return false;
    }
    virtual bool CheckSequence(const CScriptNum& nSequence) const
    {
        return false;
    }

    virtual ~BaseSignatureChecker() {}
};


class TransactionSignatureChecker : public BaseSignatureChecker
{
private:
    const CTransaction* txTo;
    unsigned int nIn;

protected:
    virtual bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;
    virtual bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const;
    virtual bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const;

public:
    TransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn) : txTo(txToIn), nIn(nInIn) {}

    //BTC
    bool CheckECDSASignature(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
    bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey_in, SigVersion sigversion, const ScriptExecutionData& execdata, ScriptError* serror) const override;
    ///////

    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode) const override;
    bool CheckLockTime(const CScriptNum& nLockTime) const override;
    bool CheckColdStake(const CScript& script) const override {
        return txTo->CheckColdStake(script);
    }
};

class MutableTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    const CTransaction txTo;

public:
    MutableTransactionSignatureChecker(const CMutableTransaction* txToIn, unsigned int nInIn) : TransactionSignatureChecker(&txTo, nInIn), txTo(*txToIn) {}
};


int FindAndDelete(CScript& script, const CScript& b);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* error = NULL);
//bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* error = NULL);


namespace BTC
{

   uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache = nullptr);

   class BaseSignatureChecker
   {
   public:
      virtual bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
      {
         return false;
      }
      virtual bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, const ScriptExecutionData& execdata, ScriptError* serror = nullptr) const
      {
         return false;
      }
      virtual bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
      {
         return false;
      }

      virtual bool CheckLockTime(const CScriptNum& nLockTime) const
      {
         return false;
      }

      virtual bool CheckSequence(const CScriptNum& nSequence) const
      {
         return false;
      }

      virtual bool CheckColdStake(const CScript& script) const
      {
         return false;
      }

      virtual ~BaseSignatureChecker() {}
   };

class TransactionSignatureChecker : public BTC::BaseSignatureChecker
   {
   private:
      const CTransaction* txTo;
      unsigned int nIn;
      const CAmount amount;
      const PrecomputedTransactionData* txdata;

   protected:
      virtual bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;
      virtual bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const;
      virtual bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const;

   public:
      TransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn) : txTo(txToIn), nIn(nInIn), amount(amountIn), txdata(nullptr) {}
      TransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, const PrecomputedTransactionData& txdataIn) : txTo(txToIn), nIn(nInIn), amount(amountIn), txdata(&txdataIn) {}

   //BTC
   bool CheckECDSASignature(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
   bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey_in, SigVersion sigversion, const ScriptExecutionData& execdata, ScriptError* serror) const override;
      bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
      bool CheckLockTime(const CScriptNum& nLockTime) const override;
      bool CheckSequence(const CScriptNum& nSequence) const override;
      bool CheckColdStake(const CScript& script) const override {
         return txTo->CheckColdStake(script);
   }
   };

class MutableTransactionSignatureChecker : public BTC::TransactionSignatureChecker
   {
   private:
      const CTransaction txTo;

   public:
      MutableTransactionSignatureChecker(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn) : TransactionSignatureChecker(&txTo, nInIn, amountIn), txTo(*txToIn) {}
   };


    template <class T>
    class GenericTransactionSignatureChecker : public BaseSignatureChecker
    {
    private:
        const T* txTo;
        unsigned int nIn;
        const CAmount amount;
        const PrecomputedTransactionData* txdata;

    protected:
        virtual bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;

    public:
        GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn) : txTo(txToIn), nIn(nInIn), amount(amountIn), txdata(nullptr) {}
        GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn, const PrecomputedTransactionData& txdataIn) : txTo(txToIn), nIn(nInIn), amount(amountIn), txdata(&txdataIn) {}
        bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
        bool CheckLockTime(const CScriptNum& nLockTime) const override;
        bool CheckSequence(const CScriptNum& nSequence) const override;
    };

    //using TransactionSignatureChecker = GenericTransactionSignatureChecker<CTransaction>;
    //using MutableTransactionSignatureChecker = GenericTransactionSignatureChecker<CMutableTransaction>;

   int FindAndDelete(CScript& script, const CScript& b);
   bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness,
                     unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror = nullptr);
   bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags,
                   const BTC::BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror = nullptr);

}

#endif // BITCOIN_SCRIPT_INTERPRETER_H
