#include <consensus/consensus.h>
#include "consensus/params.h"
#include "consensus/upgrades.h"
#include "util/system.h"

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
unsigned int dgpMaxBlockSerSize = 8000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
unsigned int dgpMaxBlockWeight = 8000000;

unsigned int dgpMaxBlockSize = 2000000; // qtum

/** The maximum allowed number of signature check operations in a block (network rule) */
int64_t dgpMaxBlockSigOps = 80000;

unsigned int dgpMaxProtoMsgLength = 8000000;

unsigned int dgpMaxTxSigOps = 16000;

void updateBlockSizeParams(unsigned int newBlockSize){
    unsigned int newSizeForParams=WITNESS_SCALE_FACTOR*newBlockSize;
    dgpMaxBlockSerSize=newSizeForParams;
    dgpMaxBlockWeight=newSizeForParams;
    dgpMaxBlockSigOps=(int64_t)(newSizeForParams/100);
    dgpMaxTxSigOps = (unsigned int)(dgpMaxBlockSigOps/5);
    dgpMaxProtoMsgLength=newSizeForParams;
}
namespace Consensus {

    bool Params::NetworkUpgradeActive(int nHeight, Consensus::UpgradeIndex idx) const {
        if (idx >= Consensus::MAX_NETWORK_UPGRADES)
            return error("%s: Upgrade index out of bounds: %d >= %d",
                         __func__, idx, Consensus::MAX_NETWORK_UPGRADES);

        if (nHeight < 0)
            return error("%s: Requested state for upgrade %s at negative height %d",
                         __func__, NetworkUpgradeInfo[idx].strName, nHeight);

        return NetworkUpgradeState(nHeight, *this, idx) == UPGRADE_ACTIVE;
    }
}