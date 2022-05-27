// Copyright (c) 2013-2014 The Bitcoin Core developers
// Copyright (c) 2017-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"
#include "rpc/client.h"

#include "wallet/wallet.h"

#include "test/test_btcu.h"
#include "key_io.h"

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include <univalue.h>


extern UniValue createArgs(int nRequired, const char* address1 = NULL, const char* address2 = NULL);
extern UniValue CallRPC(std::string args);

extern CWallet* pwalletMain;

BOOST_FIXTURE_TEST_SUITE(rpc_wallet_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(rpc_addmultisig)
{
    LOCK(pwalletMain->cs_wallet);
    SelectParams(CBaseChainParams::MAIN);
    rpcfn_type addmultisig = tableRPC["addmultisigaddress"]->actor;

    // old, 65-byte-long:
    const char address1Hex[] = "041431A18C7039660CD9E3612A2A47DC53B69CB38EA4AD743B7DF8245FD0438F8E7270415F1085B9DC4D7DA367C69F1245E27EE5552A481D6854184C80F0BB8456";
    // new, compressed:
    const char address2Hex[] = "029BBEFF390CE736BD396AF43B52A1C14ED52C086B1E5585C15931F68725772BAC";

    UniValue v;
    CBTCUAddress address;
    BOOST_CHECK_NO_THROW(v = addmultisig(createArgs(1, address1Hex), false));
    address.SetString(v.get_str());
    BOOST_CHECK(address.IsValid() && address.IsScript());

    BOOST_CHECK_NO_THROW(v = addmultisig(createArgs(1, address1Hex, address2Hex), false));
    address.SetString(v.get_str());
    BOOST_CHECK(address.IsValid() && address.IsScript());

    BOOST_CHECK_NO_THROW(v = addmultisig(createArgs(2, address1Hex, address2Hex), false));
    address.SetString(v.get_str());
    BOOST_CHECK(address.IsValid() && address.IsScript());

    BOOST_CHECK_THROW(addmultisig(createArgs(0), false), std::runtime_error);
    BOOST_CHECK_THROW(addmultisig(createArgs(1), false), std::runtime_error);
    BOOST_CHECK_THROW(addmultisig(createArgs(2, address1Hex), false), std::runtime_error);

    BOOST_CHECK_THROW(addmultisig(createArgs(1, ""), false), std::runtime_error);
    BOOST_CHECK_THROW(addmultisig(createArgs(1, "NotAValidPubkey"), false), std::runtime_error);

    std::string short1(address1Hex, address1Hex + sizeof(address1Hex) - 2); // last byte missing
    BOOST_CHECK_THROW(addmultisig(createArgs(2, short1.c_str()), false), std::runtime_error);

    std::string short2(address1Hex + 1, address1Hex + sizeof(address1Hex)); // first byte missing
    BOOST_CHECK_THROW(addmultisig(createArgs(2, short2.c_str()), false), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpc_wallet)
{
    // Test RPC calls for various wallet statistics
    UniValue r;

    SelectParams(CBaseChainParams::MAIN);

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CPubKey demoPubkey = pwalletMain->GenerateNewKey();
    CTxDestination demoAddress = GetDestinationForKey(demoPubkey, OutputType::LEGACY);
    UniValue retValue;
    std::string strAccount = "walletDemoAccount";
    std::string strPurpose = AddressBook::AddressBookPurpose::RECEIVE;
    BOOST_CHECK_NO_THROW({ /*Initialize Wallet with an account */
        CWalletDB walletdb(pwalletMain->strWalletFile);
        CAccount account;
        account.vchPubKey = demoPubkey;
        pwalletMain->SetAddressBook(PKHash(account.vchPubKey), strAccount, strPurpose);
        walletdb.WriteAccount(strAccount, account);
    });

    CPubKey setaccountDemoPubkey = pwalletMain->GenerateNewKey();
    CTxDestination setaccountDemoAddress = GetDestinationForKey(setaccountDemoPubkey, OutputType::LEGACY);

    /*********************************
     *             setaccount
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("setaccount " + EncodeDestination(setaccountDemoAddress) + " nullaccount"));
    /* 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqFF is not owned by the test wallet. */
    BOOST_CHECK_THROW(CallRPC("setaccount 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqFF nullaccount"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("setaccount"), std::runtime_error);
    /* 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqF (33 chars) is an illegal address (should be 34 chars) */
    BOOST_CHECK_THROW(CallRPC("setaccount 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqF nullaccount"), std::runtime_error);


    /*********************************
     *             listunspent
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("listunspent"));
    BOOST_CHECK_THROW(CallRPC("listunspent string"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("listunspent 0 string"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("listunspent 0 1 not_array"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("listunspent 0 1 [] extra"), std::runtime_error);
    BOOST_CHECK_NO_THROW(r = CallRPC("listunspent 0 1 []"));
    BOOST_CHECK(r.get_array().empty());

    /*********************************
     *         listreceivedbyaddress
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaddress"));
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaddress 0"));
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaddress not_int"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaddress 0 not_bool"), std::runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaddress 0 true"));
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaddress 0 true extra"), std::runtime_error);

    /*********************************
     *         listreceivedbyaccount
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaccount"));
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaccount 0"));
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaccount not_int"), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaccount 0 not_bool"), std::runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("listreceivedbyaccount 0 true"));
    BOOST_CHECK_THROW(CallRPC("listreceivedbyaccount 0 true extra"), std::runtime_error);

    /*********************************
     *         getrawchangeaddress
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("getrawchangeaddress"));

    /*********************************
     *         getnewaddress
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("getnewaddress"));
    BOOST_CHECK_NO_THROW(CallRPC("getnewaddress getnewaddress_demoaccount"));

    /*********************************
     *         getnewleasingaddress
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("getnewleasingaddress"));

    /*********************************
     *         getaccountaddress
     *********************************/
    BOOST_CHECK_NO_THROW(CallRPC("getaccountaddress \"\""));
    BOOST_CHECK_NO_THROW(CallRPC("getaccountaddress accountThatDoesntExists")); // Should generate a new account
    BOOST_CHECK_NO_THROW(retValue = CallRPC("getaccountaddress " + strAccount));
    BOOST_CHECK(DecodeDestination(retValue.get_str()) == demoAddress);

    /*********************************
     *             getaccount
     *********************************/
    BOOST_CHECK_THROW(CallRPC("getaccount"), std::runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("getaccount " + EncodeDestination(demoAddress)));

    /*********************************
     *     signmessage + verifymessage
     *********************************/
    BOOST_CHECK_NO_THROW(retValue = CallRPC("signmessage " + EncodeDestination(demoAddress) + " mymessage"));
    BOOST_CHECK_THROW(CallRPC("signmessage"), std::runtime_error);
    /* Should throw error because this address is not loaded in the wallet */
    BOOST_CHECK_THROW(CallRPC("signmessage 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqFFR mymessage"), std::runtime_error);

    SelectParams(CBaseChainParams::MAIN);
    /* missing arguments */
    BOOST_CHECK_THROW(CallRPC("verifymessage " + EncodeDestination(demoAddress)), std::runtime_error);
    BOOST_CHECK_THROW(CallRPC("verifymessage " + EncodeDestination(demoAddress) + " " + retValue.get_str()), std::runtime_error);
    /* Illegal address */
    BOOST_CHECK_THROW(CallRPC("verifymessage 1HWueZ9CbH41fjsBqdfyhWKQgbtSguSkNz " + retValue.get_str() + " mymessage"), std::runtime_error);
    /* wrong address */
    BOOST_CHECK(CallRPC("verifymessage 1EmoXtVCCaJVm2msqSw1zPBbPJjRQhNqFF " + retValue.get_str() + " mymessage").get_bool() == false);
    /* Correct address and signature but wrong message */
    BOOST_CHECK(CallRPC("verifymessage " + EncodeDestination(demoAddress) + " " + retValue.get_str() + " wrongmessage").get_bool() == false);
    /* Correct address, message and signature*/
    BOOST_CHECK(CallRPC("verifymessage " + EncodeDestination(demoAddress) + " " + retValue.get_str() + " mymessage").get_bool() == true);

    /*********************************
     *         getaddressesbyaccount
     *********************************/
    BOOST_CHECK_THROW(CallRPC("getaddressesbyaccount"), std::runtime_error);
    BOOST_CHECK_NO_THROW(retValue = CallRPC("getaddressesbyaccount " + strAccount));
    UniValue arr = retValue.get_array();
    BOOST_CHECK(arr.size() > 0);
    BOOST_CHECK(DecodeDestination(arr[0].get_str()) == demoAddress);
}

BOOST_AUTO_TEST_SUITE_END()
