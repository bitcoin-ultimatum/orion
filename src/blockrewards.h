//
// Created by dim4egster on 2/17/21.
//

#ifndef BITCOIN_ULTIMATUM_BLOCKREWARDS_H
#define BITCOIN_ULTIMATUM_BLOCKREWARDS_H

#include "main.h"
#include "masternodeman.h"
#include "spork.h"
#include "utilmoneystr.h"

int64_t GetBlockValue(int nHeight)
{
   if (Params().NetworkID() == CBaseChainParams::TESTNET) {
      if (nHeight < 200 && nHeight > 0)
         return 250000 * COIN;
   }

   if (Params().IsRegTestNet()) {
      if (nHeight == 0)
         return 250 * COIN;

   }

   int64_t nSubsidy = 0;
   if (nHeight == 0) {
      nSubsidy = 60001 * COIN;
   } else if (nHeight < 86400 && nHeight > 0) {
      nSubsidy = 250 * COIN;
   } else if (nHeight < (Params().NetworkID() == CBaseChainParams::TESTNET ? 145000 : 151200) && nHeight >= 86400) {
      nSubsidy = 225 * COIN;
   } else if (nHeight <= Params().LAST_POW_BLOCK() && nHeight >= 151200) {
      nSubsidy = 45 * COIN;
   } else if (nHeight <= 302399 && nHeight > Params().LAST_POW_BLOCK()) {
      nSubsidy = 45 * COIN;
   } else if (nHeight <= 345599 && nHeight >= 302400) {
      nSubsidy = 40.5 * COIN;
   } else if (nHeight <= 388799 && nHeight >= 345600) {
      nSubsidy = 36 * COIN;
   } else if (nHeight <= 431999 && nHeight >= 388800) {
      nSubsidy = 31.5 * COIN;
   } else if (nHeight <= 475199 && nHeight >= 432000) {
      nSubsidy = 27 * COIN;
   } else if (nHeight <= 518399 && nHeight >= 475200) {
      nSubsidy = 22.5 * COIN;
   } else if (nHeight <= 561599 && nHeight >= 518400) {
      nSubsidy = 18 * COIN;
   } else if (nHeight <= 604799 && nHeight >= 561600) {
      nSubsidy = 13.5 * COIN;
   } else if (nHeight <= 647999 && nHeight >= 604800) {
      nSubsidy = 9 * COIN;
   } else if (nHeight < Params().Zerocoin_Block_V2_Start()) {
      nSubsidy = 4.5 * COIN;
   } else {
      nSubsidy = 5 * COIN;
   }
   return nSubsidy;
}

CAmount GetSeeSaw(const CAmount& blockValue, int nMasternodeCount, int nHeight)
{
   //if a mn count is inserted into the function we are looking for a specific result for a masternode count
   if (nMasternodeCount < 1){
      if (sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT))
         nMasternodeCount = mnodeman.stable_size();
      else
         nMasternodeCount = mnodeman.size();
   }

   int64_t nMoneySupply = chainActive.Tip()->nMoneySupply;
   int64_t mNodeCoins = nMasternodeCount * MN_DEPOSIT_SIZE * COIN;

   // Use this log to compare the masternode count for different clients
   //LogPrintf("Adjusting seesaw at height %d with %d masternodes (without drift: %d) at %ld\n", nHeight, nMasternodeCount, nMasternodeCount - Params().MasternodeCountDrift(), GetTime());

   if (fDebug)
      LogPrintf("GetMasternodePayment(): moneysupply=%s, nodecoins=%s \n", FormatMoney(nMoneySupply).c_str(),
                FormatMoney(mNodeCoins).c_str());

   CAmount ret = 0;
   if (mNodeCoins == 0) {
      ret = 0;
   } else if (nHeight <= 325000) {
      if (mNodeCoins <= (nMoneySupply * .05) && mNodeCoins > 0) {
         ret = blockValue * .85;
      } else if (mNodeCoins <= (nMoneySupply * .1) && mNodeCoins > (nMoneySupply * .05)) {
         ret = blockValue * .8;
      } else if (mNodeCoins <= (nMoneySupply * .15) && mNodeCoins > (nMoneySupply * .1)) {
         ret = blockValue * .75;
      } else if (mNodeCoins <= (nMoneySupply * .2) && mNodeCoins > (nMoneySupply * .15)) {
         ret = blockValue * .7;
      } else if (mNodeCoins <= (nMoneySupply * .25) && mNodeCoins > (nMoneySupply * .2)) {
         ret = blockValue * .65;
      } else if (mNodeCoins <= (nMoneySupply * .3) && mNodeCoins > (nMoneySupply * .25)) {
         ret = blockValue * .6;
      } else if (mNodeCoins <= (nMoneySupply * .35) && mNodeCoins > (nMoneySupply * .3)) {
         ret = blockValue * .55;
      } else if (mNodeCoins <= (nMoneySupply * .4) && mNodeCoins > (nMoneySupply * .35)) {
         ret = blockValue * .5;
      } else if (mNodeCoins <= (nMoneySupply * .45) && mNodeCoins > (nMoneySupply * .4)) {
         ret = blockValue * .45;
      } else if (mNodeCoins <= (nMoneySupply * .5) && mNodeCoins > (nMoneySupply * .45)) {
         ret = blockValue * .4;
      } else if (mNodeCoins <= (nMoneySupply * .55) && mNodeCoins > (nMoneySupply * .5)) {
         ret = blockValue * .35;
      } else if (mNodeCoins <= (nMoneySupply * .6) && mNodeCoins > (nMoneySupply * .55)) {
         ret = blockValue * .3;
      } else if (mNodeCoins <= (nMoneySupply * .65) && mNodeCoins > (nMoneySupply * .6)) {
         ret = blockValue * .25;
      } else if (mNodeCoins <= (nMoneySupply * .7) && mNodeCoins > (nMoneySupply * .65)) {
         ret = blockValue * .2;
      } else if (mNodeCoins <= (nMoneySupply * .75) && mNodeCoins > (nMoneySupply * .7)) {
         ret = blockValue * .15;
      } else {
         ret = blockValue * .1;
      }
   } else if (nHeight > 325000) {
      if (mNodeCoins <= (nMoneySupply * .01) && mNodeCoins > 0) {
         ret = blockValue * .90;
      } else if (mNodeCoins <= (nMoneySupply * .02) && mNodeCoins > (nMoneySupply * .01)) {
         ret = blockValue * .88;
      } else if (mNodeCoins <= (nMoneySupply * .03) && mNodeCoins > (nMoneySupply * .02)) {
         ret = blockValue * .87;
      } else if (mNodeCoins <= (nMoneySupply * .04) && mNodeCoins > (nMoneySupply * .03)) {
         ret = blockValue * .86;
      } else if (mNodeCoins <= (nMoneySupply * .05) && mNodeCoins > (nMoneySupply * .04)) {
         ret = blockValue * .85;
      } else if (mNodeCoins <= (nMoneySupply * .06) && mNodeCoins > (nMoneySupply * .05)) {
         ret = blockValue * .84;
      } else if (mNodeCoins <= (nMoneySupply * .07) && mNodeCoins > (nMoneySupply * .06)) {
         ret = blockValue * .83;
      } else if (mNodeCoins <= (nMoneySupply * .08) && mNodeCoins > (nMoneySupply * .07)) {
         ret = blockValue * .82;
      } else if (mNodeCoins <= (nMoneySupply * .09) && mNodeCoins > (nMoneySupply * .08)) {
         ret = blockValue * .81;
      } else if (mNodeCoins <= (nMoneySupply * .10) && mNodeCoins > (nMoneySupply * .09)) {
         ret = blockValue * .80;
      } else if (mNodeCoins <= (nMoneySupply * .11) && mNodeCoins > (nMoneySupply * .10)) {
         ret = blockValue * .79;
      } else if (mNodeCoins <= (nMoneySupply * .12) && mNodeCoins > (nMoneySupply * .11)) {
         ret = blockValue * .78;
      } else if (mNodeCoins <= (nMoneySupply * .13) && mNodeCoins > (nMoneySupply * .12)) {
         ret = blockValue * .77;
      } else if (mNodeCoins <= (nMoneySupply * .14) && mNodeCoins > (nMoneySupply * .13)) {
         ret = blockValue * .76;
      } else if (mNodeCoins <= (nMoneySupply * .15) && mNodeCoins > (nMoneySupply * .14)) {
         ret = blockValue * .75;
      } else if (mNodeCoins <= (nMoneySupply * .16) && mNodeCoins > (nMoneySupply * .15)) {
         ret = blockValue * .74;
      } else if (mNodeCoins <= (nMoneySupply * .17) && mNodeCoins > (nMoneySupply * .16)) {
         ret = blockValue * .73;
      } else if (mNodeCoins <= (nMoneySupply * .18) && mNodeCoins > (nMoneySupply * .17)) {
         ret = blockValue * .72;
      } else if (mNodeCoins <= (nMoneySupply * .19) && mNodeCoins > (nMoneySupply * .18)) {
         ret = blockValue * .71;
      } else if (mNodeCoins <= (nMoneySupply * .20) && mNodeCoins > (nMoneySupply * .19)) {
         ret = blockValue * .70;
      } else if (mNodeCoins <= (nMoneySupply * .21) && mNodeCoins > (nMoneySupply * .20)) {
         ret = blockValue * .69;
      } else if (mNodeCoins <= (nMoneySupply * .22) && mNodeCoins > (nMoneySupply * .21)) {
         ret = blockValue * .68;
      } else if (mNodeCoins <= (nMoneySupply * .23) && mNodeCoins > (nMoneySupply * .22)) {
         ret = blockValue * .67;
      } else if (mNodeCoins <= (nMoneySupply * .24) && mNodeCoins > (nMoneySupply * .23)) {
         ret = blockValue * .66;
      } else if (mNodeCoins <= (nMoneySupply * .25) && mNodeCoins > (nMoneySupply * .24)) {
         ret = blockValue * .65;
      } else if (mNodeCoins <= (nMoneySupply * .26) && mNodeCoins > (nMoneySupply * .25)) {
         ret = blockValue * .64;
      } else if (mNodeCoins <= (nMoneySupply * .27) && mNodeCoins > (nMoneySupply * .26)) {
         ret = blockValue * .63;
      } else if (mNodeCoins <= (nMoneySupply * .28) && mNodeCoins > (nMoneySupply * .27)) {
         ret = blockValue * .62;
      } else if (mNodeCoins <= (nMoneySupply * .29) && mNodeCoins > (nMoneySupply * .28)) {
         ret = blockValue * .61;
      } else if (mNodeCoins <= (nMoneySupply * .30) && mNodeCoins > (nMoneySupply * .29)) {
         ret = blockValue * .60;
      } else if (mNodeCoins <= (nMoneySupply * .31) && mNodeCoins > (nMoneySupply * .30)) {
         ret = blockValue * .59;
      } else if (mNodeCoins <= (nMoneySupply * .32) && mNodeCoins > (nMoneySupply * .31)) {
         ret = blockValue * .58;
      } else if (mNodeCoins <= (nMoneySupply * .33) && mNodeCoins > (nMoneySupply * .32)) {
         ret = blockValue * .57;
      } else if (mNodeCoins <= (nMoneySupply * .34) && mNodeCoins > (nMoneySupply * .33)) {
         ret = blockValue * .56;
      } else if (mNodeCoins <= (nMoneySupply * .35) && mNodeCoins > (nMoneySupply * .34)) {
         ret = blockValue * .55;
      } else if (mNodeCoins <= (nMoneySupply * .363) && mNodeCoins > (nMoneySupply * .35)) {
         ret = blockValue * .54;
      } else if (mNodeCoins <= (nMoneySupply * .376) && mNodeCoins > (nMoneySupply * .363)) {
         ret = blockValue * .53;
      } else if (mNodeCoins <= (nMoneySupply * .389) && mNodeCoins > (nMoneySupply * .376)) {
         ret = blockValue * .52;
      } else if (mNodeCoins <= (nMoneySupply * .402) && mNodeCoins > (nMoneySupply * .389)) {
         ret = blockValue * .51;
      } else if (mNodeCoins <= (nMoneySupply * .415) && mNodeCoins > (nMoneySupply * .402)) {
         ret = blockValue * .50;
      } else if (mNodeCoins <= (nMoneySupply * .428) && mNodeCoins > (nMoneySupply * .415)) {
         ret = blockValue * .49;
      } else if (mNodeCoins <= (nMoneySupply * .441) && mNodeCoins > (nMoneySupply * .428)) {
         ret = blockValue * .48;
      } else if (mNodeCoins <= (nMoneySupply * .454) && mNodeCoins > (nMoneySupply * .441)) {
         ret = blockValue * .47;
      } else if (mNodeCoins <= (nMoneySupply * .467) && mNodeCoins > (nMoneySupply * .454)) {
         ret = blockValue * .46;
      } else if (mNodeCoins <= (nMoneySupply * .48) && mNodeCoins > (nMoneySupply * .467)) {
         ret = blockValue * .45;
      } else if (mNodeCoins <= (nMoneySupply * .493) && mNodeCoins > (nMoneySupply * .48)) {
         ret = blockValue * .44;
      } else if (mNodeCoins <= (nMoneySupply * .506) && mNodeCoins > (nMoneySupply * .493)) {
         ret = blockValue * .43;
      } else if (mNodeCoins <= (nMoneySupply * .519) && mNodeCoins > (nMoneySupply * .506)) {
         ret = blockValue * .42;
      } else if (mNodeCoins <= (nMoneySupply * .532) && mNodeCoins > (nMoneySupply * .519)) {
         ret = blockValue * .41;
      } else if (mNodeCoins <= (nMoneySupply * .545) && mNodeCoins > (nMoneySupply * .532)) {
         ret = blockValue * .40;
      } else if (mNodeCoins <= (nMoneySupply * .558) && mNodeCoins > (nMoneySupply * .545)) {
         ret = blockValue * .39;
      } else if (mNodeCoins <= (nMoneySupply * .571) && mNodeCoins > (nMoneySupply * .558)) {
         ret = blockValue * .38;
      } else if (mNodeCoins <= (nMoneySupply * .584) && mNodeCoins > (nMoneySupply * .571)) {
         ret = blockValue * .37;
      } else if (mNodeCoins <= (nMoneySupply * .597) && mNodeCoins > (nMoneySupply * .584)) {
         ret = blockValue * .36;
      } else if (mNodeCoins <= (nMoneySupply * .61) && mNodeCoins > (nMoneySupply * .597)) {
         ret = blockValue * .35;
      } else if (mNodeCoins <= (nMoneySupply * .623) && mNodeCoins > (nMoneySupply * .61)) {
         ret = blockValue * .34;
      } else if (mNodeCoins <= (nMoneySupply * .636) && mNodeCoins > (nMoneySupply * .623)) {
         ret = blockValue * .33;
      } else if (mNodeCoins <= (nMoneySupply * .649) && mNodeCoins > (nMoneySupply * .636)) {
         ret = blockValue * .32;
      } else if (mNodeCoins <= (nMoneySupply * .662) && mNodeCoins > (nMoneySupply * .649)) {
         ret = blockValue * .31;
      } else if (mNodeCoins <= (nMoneySupply * .675) && mNodeCoins > (nMoneySupply * .662)) {
         ret = blockValue * .30;
      } else if (mNodeCoins <= (nMoneySupply * .688) && mNodeCoins > (nMoneySupply * .675)) {
         ret = blockValue * .29;
      } else if (mNodeCoins <= (nMoneySupply * .701) && mNodeCoins > (nMoneySupply * .688)) {
         ret = blockValue * .28;
      } else if (mNodeCoins <= (nMoneySupply * .714) && mNodeCoins > (nMoneySupply * .701)) {
         ret = blockValue * .27;
      } else if (mNodeCoins <= (nMoneySupply * .727) && mNodeCoins > (nMoneySupply * .714)) {
         ret = blockValue * .26;
      } else if (mNodeCoins <= (nMoneySupply * .74) && mNodeCoins > (nMoneySupply * .727)) {
         ret = blockValue * .25;
      } else if (mNodeCoins <= (nMoneySupply * .753) && mNodeCoins > (nMoneySupply * .74)) {
         ret = blockValue * .24;
      } else if (mNodeCoins <= (nMoneySupply * .766) && mNodeCoins > (nMoneySupply * .753)) {
         ret = blockValue * .23;
      } else if (mNodeCoins <= (nMoneySupply * .779) && mNodeCoins > (nMoneySupply * .766)) {
         ret = blockValue * .22;
      } else if (mNodeCoins <= (nMoneySupply * .792) && mNodeCoins > (nMoneySupply * .779)) {
         ret = blockValue * .21;
      } else if (mNodeCoins <= (nMoneySupply * .805) && mNodeCoins > (nMoneySupply * .792)) {
         ret = blockValue * .20;
      } else if (mNodeCoins <= (nMoneySupply * .818) && mNodeCoins > (nMoneySupply * .805)) {
         ret = blockValue * .19;
      } else if (mNodeCoins <= (nMoneySupply * .831) && mNodeCoins > (nMoneySupply * .818)) {
         ret = blockValue * .18;
      } else if (mNodeCoins <= (nMoneySupply * .844) && mNodeCoins > (nMoneySupply * .831)) {
         ret = blockValue * .17;
      } else if (mNodeCoins <= (nMoneySupply * .857) && mNodeCoins > (nMoneySupply * .844)) {
         ret = blockValue * .16;
      } else if (mNodeCoins <= (nMoneySupply * .87) && mNodeCoins > (nMoneySupply * .857)) {
         ret = blockValue * .15;
      } else if (mNodeCoins <= (nMoneySupply * .883) && mNodeCoins > (nMoneySupply * .87)) {
         ret = blockValue * .14;
      } else if (mNodeCoins <= (nMoneySupply * .896) && mNodeCoins > (nMoneySupply * .883)) {
         ret = blockValue * .13;
      } else if (mNodeCoins <= (nMoneySupply * .909) && mNodeCoins > (nMoneySupply * .896)) {
         ret = blockValue * .12;
      } else if (mNodeCoins <= (nMoneySupply * .922) && mNodeCoins > (nMoneySupply * .909)) {
         ret = blockValue * .11;
      } else if (mNodeCoins <= (nMoneySupply * .935) && mNodeCoins > (nMoneySupply * .922)) {
         ret = blockValue * .10;
      } else if (mNodeCoins <= (nMoneySupply * .945) && mNodeCoins > (nMoneySupply * .935)) {
         ret = blockValue * .09;
      } else if (mNodeCoins <= (nMoneySupply * .961) && mNodeCoins > (nMoneySupply * .945)) {
         ret = blockValue * .08;
      } else if (mNodeCoins <= (nMoneySupply * .974) && mNodeCoins > (nMoneySupply * .961)) {
         ret = blockValue * .07;
      } else if (mNodeCoins <= (nMoneySupply * .987) && mNodeCoins > (nMoneySupply * .974)) {
         ret = blockValue * .06;
      } else if (mNodeCoins <= (nMoneySupply * .99) && mNodeCoins > (nMoneySupply * .987)) {
         ret = blockValue * .05;
      } else {
         ret = blockValue * .01;
      }
   }
   return ret;
}

int64_t GetMasternodePayment(int nHeight, int64_t blockValue, int nMasternodeCount, bool isZBTCUStake)
{
   int64_t ret = 0;

   if (Params().NetworkID() == CBaseChainParams::TESTNET) {
      if (nHeight < 200)
         return 0;
   }

   if (nHeight <= 43200) {
      ret = blockValue / 5;
   } else if (nHeight < 86400 && nHeight > 43200) {
      ret = blockValue / (100 / 30);
   } else if (nHeight < (Params().NetworkID() == CBaseChainParams::TESTNET ? 145000 : 151200) && nHeight >= 86400) {
      ret = 50 * COIN;
   } else if (nHeight <= Params().LAST_POW_BLOCK() && nHeight >= 151200) {
      ret = blockValue / 2;
   } else if (nHeight < Params().Zerocoin_Block_V2_Start()) {
      return GetSeeSaw(blockValue, nMasternodeCount, nHeight);
   } else {
      //When zBTCU is staked, masternode only gets 2 BTCU
      ret = 3 * COIN;
      if (isZBTCUStake)
         ret = 2 * COIN;
   }

   return ret;
}
#endif //BITCOIN_ULTIMATUM_BLOCKREWARDS_H
