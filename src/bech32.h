// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Bech32 and Bech32m are string encoding formats used in newer
// address types. The outputs consist of a human-readable part
// (alphanumeric), a separator character (1), and a base32 data
// section, the last 6 characters of which are a checksum. The
// module is namespaced under bech32 for historical reasons.
//
// For more information, see BIP 173 and BIP 350.

#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include <time.h>
#include <atomic>
#include <stdint.h>
#include <string>
#include <vector>

#include "allocators.h"

namespace bech32
{
    typedef std::vector<uint8_t> data;
    enum class Encoding {
      INVALID, //!< Failed decoding

/** Decode a Bech32 string. Returns (hrp, data). Empty hrp means failure. */
//std::pair<std::string, std::vector<uint8_t>> Decode(const std::string& str);
      BECH32,  //!< Bech32 encoding as defined in BIP173
      BECH32M, //!< Bech32m encoding as defined in BIP350
   };

    /** Encode a Bech32 or Bech32m string. If hrp contains uppercase characters, this will cause an
 *  assertion error. Encoding must be one of BECH32 or BECH32M. */
    std::string Encode(Encoding encoding, const std::string& hrp, const std::vector<uint8_t>& values);

    struct DecodeResult
    {
        Encoding encoding;         //!< What encoding was detected in the result; Encoding::INVALID if failed.
        std::string hrp;           //!< The human readable part
        std::vector<uint8_t> data; //!< The payload (excluding checksum)

        DecodeResult() : encoding(Encoding::INVALID) {}
        DecodeResult(Encoding enc, std::string&& h, std::vector<uint8_t>&& d) : encoding(enc), hrp(std::move(h)), data(std::move(d)) {}
    };
/** Decode a Bech32 or Bech32m string. */
    DecodeResult Decode(const std::string& str);

/** Return the positions of errors in a Bech32 string. */
   std::pair<std::string, std::vector<int>> LocateErrors(const std::string& str);

} // namespace bech32


/**
 * Base class for all bech32-encoded data
 */
class CBech32Data
{
    friend class CBTCUAddress;
    friend class CBTCUSecret;

    int nVersion = 255;

    //! the actually encoded data
    typedef std::vector<uint8_t> vector_uchar;
    vector_uchar vchData;

public:
    CBech32Data() = default;
    ~CBech32Data() = default;
    void SetData(int version, const void* pdata, size_t nSize);
    void Clear();

    bool SetString(const std::string& str);
    std::string ToString() const;
    int CompareTo(const CBech32Data& b32) const;
};

#endif // BITCOIN_BECH32_H
