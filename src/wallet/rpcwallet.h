// Copyright (c) 2016-2021 The Bitcoin Core developers
// Copyright (c) 2021 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BTCU_WALLET_RPCWALLET_H
#define BTCU_WALLET_RPCWALLET_H

#include <string>
#include "sapling/key_io_sapling.h"

class CRPCTable;
class CWallet;
class JSONRPCRequest;

std::string HelpRequiringPassphrase(CWallet* const pwallet);
bool EnsureWalletIsAvailable(CWallet* const pwallet, bool avoidException);
void EnsureWalletIsUnlocked(CWallet* const pwallet, bool fAllowAnonOnly = false);

#endif //PIVX_WALLET_RPCWALLET_H
