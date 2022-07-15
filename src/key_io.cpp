// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>

#include <base58.h>
#include <bech32.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include <assert.h>
#include <string.h>
#include <algorithm>


/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

bool N_DecodeBase58(const char* psz, std::vector<unsigned char>& vch)
{
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    std::vector<unsigned char> b256(strlen(psz) * 733 / 1000 + 1); // log(58) / log(256), rounded up.
    // Process the characters.
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        const char* ch = strchr(pszBase58, *psz);
        if (ch == NULL)
            return false;
        // Apply "b256 = b256 * 58 + ch".
        int carry = ch - pszBase58;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); it != b256.rend(); it++) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        return false;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin();
    while (it != b256.end() && *it == 0)
        it++;
    // Copy result into output vector.
    vch.reserve(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end())
        vch.push_back(*(it++));
    return true;
}

std::string N_DecodeBase58(const char* psz)
{
    std::vector<unsigned char> vch;
    N_DecodeBase58(psz, vch);
    std::stringstream ss;
    ss << std::hex;

    for (unsigned int i = 0; i < vch.size(); i++) {
        unsigned char* c = &vch[i];
        ss << std::setw(2) << std::setfill('0') << (int)c[0];
    }

    return ss.str();
}

std::string N_EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    // Skip & count leading zeroes.
    int zeroes = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    std::vector<unsigned char> b58((pend - pbegin) * 138 / 100 + 1); // log(256) / log(58), rounded up.
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); it != b58.rend(); it++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
        assert(carry == 0);
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin();
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

std::string N_EncodeBase58(const std::vector<unsigned char>& vch)
{
    return N_EncodeBase58(&vch[0], &vch[0] + vch.size());
}

bool N_DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return N_DecodeBase58(str.c_str(), vchRet);
}

std::string N_EncodeBase58Check(const std::vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(vchIn);
    uint256 hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return N_EncodeBase58(vch);
}

bool N_DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
{
    if (!N_DecodeBase58(psz, vchRet) ||
        (vchRet.size() < 4)) {
        vchRet.clear();
        return false;
    }
    // re-calculate the checksum, insure it matches the included 4-byte checksum
    uint256 hash = Hash(vchRet.begin(), vchRet.end() - 4);
    if (memcmp(&hash, &vchRet.end()[-4], 4) != 0) {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size() - 4);
    return true;
}

bool N_DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
{
    return N_DecodeBase58Check(str.c_str(), vchRet);
}

    class DestinationEncoder : public boost::static_visitor<std::string> {
    private:
        const CChainParams &m_params;
        const CChainParams::Base58Type m_addrType;

    public:
        DestinationEncoder(const CChainParams &params,
                           const CChainParams::Base58Type _addrType = CChainParams::PUBKEY_ADDRESS) : m_params(params),
                                                                                                      m_addrType(
                                                                                                              _addrType) {}

        std::string operator()(const CKeyID &id) const {
            std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
            data.insert(data.end(), id.begin(), id.end());
            return EncodeBase58Check(data);
        }


        std::string operator()(const CScriptID &id) const {
            std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
            data.insert(data.end(), id.begin(), id.end());
            return EncodeBase58Check(data);
        }

        std::string operator()(const WitnessV0KeyHash &id) const {
            std::vector<uint8_t> data = {0};
            data.reserve(33);
            ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
            return bech32::Encode(bech32::Encoding::BECH32, m_params.Bech32HRP(),
                                  data);
        }

        std::string operator()(const WitnessV0ScriptHash &id) const {
            std::vector<unsigned char> data = {0};
            data.reserve(53);
            ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
            return bech32::Encode(bech32::Encoding::BECH32, m_params.Bech32HRP(),
                                  data);
        }

        std::string operator()(const WitnessUnknown &id) const {
            if (id.version < 1 || id.version > 16 || id.length < 2 || id.length > 40) {
                return {};
            }
            std::vector<unsigned char> data = {(unsigned char) id.version};
            data.reserve(1 + (id.length * 8 + 4) / 5);
            ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.program, id.program + id.length);
            return bech32::Encode(bech32::Encoding::BECH32, m_params.Bech32HRP(),
                                  data);
        }

        std::string operator()(const PKHash &id) const {
            std::vector<unsigned char> data = m_params.Base58Prefix(m_addrType);
            data.insert(data.end(), id.begin(), id.end());
            return EncodeBase58Check(data);
        }

        std::string operator()(const ScriptHash &id) const {
            std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
            data.insert(data.end(), id.begin(), id.end());
            return EncodeBase58Check(data);
        }

        std::string operator()(const WitnessV1Taproot &tap) const {
            std::vector<unsigned char> data = {1};
            data.reserve(53);
            ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, tap.begin(), tap.end());
            return bech32::Encode(bech32::Encoding::BECH32M, m_params.Bech32HRP(),
                                  data);
        }

        std::string operator()(const CNoDestination &no) const { return {}; }

        CTxDestination DecodeDestination(const std::string &str, const CChainParams &params, bool &isStaking) {
            std::vector<unsigned char> data;
            uint160 hash;
            if (DecodeBase58Check(str, data, 21)) {
                // base58-encoded PIVX addresses.
                // Public-key-hash-addresses have version 30 (or 139 testnet).
                // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
                const std::vector<unsigned char> &pubkey_prefix = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
                if (data.size() == hash.size() + pubkey_prefix.size() &&
                    std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
                    std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
                    return CKeyID(hash);
                }
                // Public-key-hash-coldstaking-addresses have version 63 (or 73 testnet).
                const std::vector<unsigned char> &staking_prefix = params.Base58Prefix(CChainParams::STAKING_ADDRESS);
                if (data.size() == hash.size() + staking_prefix.size() &&
                    std::equal(staking_prefix.begin(), staking_prefix.end(), data.begin())) {
                    isStaking = true;
                    std::copy(data.begin() + staking_prefix.size(), data.end(), hash.begin());
                    return CKeyID(hash);
                }
                // Script-hash-addresses have version 13 (or 19 testnet).
                // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
                const std::vector<unsigned char> &script_prefix = params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
                if (data.size() == hash.size() + script_prefix.size() &&
                    std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
                    std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
                    return CScriptID(hash);
                }
            }
            return CNoDestination();
        }
    };

    CTxDestination DecodeDestination(const std::string &str, const CChainParams &params) {
        std::vector<unsigned char> data;
        uint160 hash;
        if (N_DecodeBase58Check(str, data)) {
            // base58-encoded Bitcoin addresses.
            // Public-key-hash-addresses have version 0 (or 111 testnet).
            // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
            const std::vector<unsigned char> &pubkey_prefix = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
            if (data.size() == hash.size() + pubkey_prefix.size() &&
                std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
                std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
                return CKeyID(hash);
            }
            // Script-hash-addresses have version 5 (or 196 testnet).
            // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
            const std::vector<unsigned char> &script_prefix = params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
            if (data.size() == hash.size() + script_prefix.size() &&
                std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
                std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
                return CScriptID(hash);
            }
        }
        data.clear();
        auto bech = bech32::Decode(str);
        if (bech.data.size() > 0 /*&& bech.first == params.Bech32HRP()*/) {
            // Bech32 decoding
            int version = bech.data[0]; // The first 5 bit symbol is the witness version (0-16)
            // The rest of the symbols are converted witness program bytes.
            data.reserve(((bech.data.size() - 1) * 5) / 8);
            if (ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); }, bech.data.begin() + 1,
                                         bech.data.end())) {
                if (version == 0) {
                    {
                        WitnessV0KeyHash keyid;
                        if (data.size() == keyid.size()) {
                            std::copy(data.begin(), data.end(), keyid.begin());
                            return keyid;
                        }
                    }
                    {
                        WitnessV0ScriptHash scriptid;
                        if (data.size() == scriptid.size()) {
                            std::copy(data.begin(), data.end(), scriptid.begin());
                            return scriptid;
                        }
                    }
                    return CNoDestination();
                }

                if (version == 1 && data.size() == WITNESS_V1_TAPROOT_SIZE) {
                    static_assert(WITNESS_V1_TAPROOT_SIZE == WitnessV1Taproot::size());
                    WitnessV1Taproot tap;
                    std::copy(data.begin(), data.end(), tap.begin());
                    return tap;
                }

                if (version > 16 || data.size() < 2 || data.size() > 40) {
                    return CNoDestination();
                }
                WitnessUnknown unk;
                unk.version = version;
                std::copy(data.begin(), data.end(), unk.program);
                unk.length = data.size();
                return unk;
            }
        }
        return CNoDestination();
    }
//} // namespace

    CTxDestination DecodeDestination(const std::string &str, const CChainParams &params, bool &isStaking) {
        std::vector<unsigned char> data;
        uint160 hash;
        if (DecodeBase58Check(str, data, 21)) {
            // base58-encoded PIVX addresses.
            // Public-key-hash-addresses have version 30 (or 139 testnet).
            // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
            const std::vector<unsigned char> &pubkey_prefix = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
            if (data.size() == hash.size() + pubkey_prefix.size() &&
                std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
                std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
                return CKeyID(hash);
            }
            // Public-key-hash-coldstaking-addresses have version 63 (or 73 testnet).
            const std::vector<unsigned char> &staking_prefix = params.Base58Prefix(CChainParams::STAKING_ADDRESS);
            if (data.size() == hash.size() + staking_prefix.size() &&
                std::equal(staking_prefix.begin(), staking_prefix.end(), data.begin())) {
                isStaking = true;
                std::copy(data.begin() + staking_prefix.size(), data.end(), hash.begin());
                return CKeyID(hash);
            }
            // Script-hash-addresses have version 13 (or 19 testnet).
            // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
            const std::vector<unsigned char> &script_prefix = params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
            if (data.size() == hash.size() + script_prefix.size() &&
                std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
                std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
                return CScriptID(hash);
            }
        }
        return CNoDestination();
    }

    CKey DecodeSecret(const std::string &str) {
        CKey key;
        std::vector<unsigned char> data;
        if (N_DecodeBase58Check(str, data)) {
            const std::vector<unsigned char> &privkey_prefix = Params().Base58Prefix(CChainParams::SECRET_KEY);
            if ((data.size() == 32 + privkey_prefix.size() ||
                 (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
                std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
                bool compressed = data.size() == 33 + privkey_prefix.size();
                key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32, compressed);
            }
        }
        if (!data.empty()) {
            memory_cleanse(data.data(), data.size());
        }
        return key;
    }

    std::string EncodeSecret(const CKey &key) {
        assert(key.IsValid());
        std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::SECRET_KEY);
        data.insert(data.end(), key.begin(), key.end());
        if (key.IsCompressed()) {
            data.push_back(1);
        }
        std::string ret = N_EncodeBase58Check(data);
        memory_cleanse(data.data(), data.size());
        return ret;
    }

    std::string EncodeDestination(const CTxDestination &dest, const CChainParams::Base58Type addrType) {
        return std::visit(DestinationEncoder(Params(), addrType), dest);
    }

//std::string EncodeDestination(const CTxDestination& dest, bool isStaking)
//{
//    return EncodeDestination(dest, isStaking ? CChainParams::STAKING_ADDRESS : CChainParams::PUBKEY_ADDRESS);
//}

    CTxDestination DecodeDestination(const std::string &str) {
        return DecodeDestination(str, Params());
    }

    CTxDestination DecodeDestination(const std::string &str, bool &isStaking) {
        return DecodeDestination(str, Params(), isStaking);
    }

    bool IsValidDestinationString(const std::string &str, const CChainParams &params) {
        return IsValidDestination(DecodeDestination(str, params));
    }

//bool IsValidDestinationString(const std::string& str)
//{
//    return IsValidDestinationString(str, Params());
//}

    bool IsValidContractSenderAddressString(const std::string &str) {
        return IsValidContractSenderAddress(DecodeDestination(str));
    }

    namespace KeyIO {

        CKey DecodeSecret(const std::string &str) {
            CKey key;
            std::vector<unsigned char> data;
            if (DecodeBase58Check(str, data, 34)) {
                const std::vector<unsigned char> &privkey_prefix = std::vector<unsigned char>(1,
                                                                                              239);//= Params().Base58Prefix(CChainParams::SECRET_KEY);
                if ((data.size() == 32 + privkey_prefix.size() ||
                     (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
                    std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
                    bool compressed = data.size() == 33 + privkey_prefix.size();
                    key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32,
                            compressed);
                }
            }
            if (!data.empty()) {
                memory_cleanse(data.data(), data.size());
            }
            return key;
        }

        std::string EncodeSecret(const CKey &key) {
            assert(key.IsValid());
            std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::SECRET_KEY);
            data.insert(data.end(), key.begin(), key.end());
            if (key.IsCompressed()) {
                data.push_back(1);
            }
            std::string ret = EncodeBase58Check(data);
            memory_cleanse(data.data(), data.size());
            return ret;
        }

        CExtKey DecodeExtKey(const std::string &str) {
            CExtKey key;
            std::vector<unsigned char> data;
            if (DecodeBase58Check(str, data, 78)) {
                const std::vector<unsigned char> &prefix = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
                if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() &&
                    std::equal(prefix.begin(), prefix.end(), data.begin())) {
                    key.Decode(data.data() + prefix.size());
                }
            }
            return key;
        }

        std::string EncodeExtKey(const CExtKey &key) {
            std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
            size_t size = data.size();
            data.resize(size + BIP32_EXTKEY_SIZE);
            key.Encode(data.data() + size);
            std::string ret = EncodeBase58Check(data);
            memory_cleanse(data.data(), data.size());
            return ret;
        }

        CExtPubKey DecodeExtPubKey(const std::string &str) {
            CExtPubKey key;
            std::vector<unsigned char> data;
            if (DecodeBase58Check(str, data, 78)) {
                const std::vector<unsigned char> &prefix = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
                if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() &&
                    std::equal(prefix.begin(), prefix.end(), data.begin())) {
                    key.Decode(data.data() + prefix.size());
                }
            }
            return key;
        }

        std::string EncodeExtPubKey(const CExtPubKey &key) {
            std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
            size_t size = data.size();
            data.resize(size + BIP32_EXTKEY_SIZE);
            key.Encode(data.data() + size);
            std::string ret = EncodeBase58Check(data);
            return ret;
        }

    }
#ifdef ENABLE_BITCORE_RPC
    bool DecodeIndexKey(const std::string &str, uint256 &hashBytes, int &type)
    {
        CTxDestination dest = DecodeDestination(str);
        if (IsValidDestination(dest))
        {
            const PKHash *keyID = boost::get<PKHash>(&dest);
            if(keyID)
            {
                memcpy(&hashBytes, keyID, 20);
                type = 1;
                return true;
            }

            const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
            if(scriptID)
            {
                memcpy(&hashBytes, scriptID, 20);
                type = 2;
                return true;
            }

            const WitnessV0ScriptHash *witnessV0ScriptID = boost::get<WitnessV0ScriptHash>(&dest);
            if (witnessV0ScriptID) {
                memcpy(&hashBytes, witnessV0ScriptID, 32);
                type = 3;
                return true;
            }

            const WitnessV0KeyHash *witnessV0KeyID = boost::get<WitnessV0KeyHash>(&dest);
            if (witnessV0KeyID) {
                memcpy(&hashBytes, witnessV0KeyID, 20);
                type = 4;
                return true;
            }
        }

        return false;
    }
#endif