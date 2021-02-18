    #!/bin/bash
    timed_wait=1
    i=0
    blockcount=$(./btcu-cli getblockcount)
        while true
        do
            if (( $i == $blockcount ))
            then
              break
            fi

            blockhash=$(./btcu-cli getblockhash $i)
            echo $(./btcu-cli getblock $blockhash)

            #echo $blockhash
            #sleep $timed_wait

            i=$((i+1))
        done
