while getopts m:n:l: flag
do
    case "${flag}" in
        m) n_hosts=${OPTARG};;
        n) n_players=${OPTARG};;
        l) lucky_number=${OPTARG};;
    esac
done

rm -f fifo_*.tmp
mkfifo fifo_0.tmp
exec 3<> fifo_0.tmp
for ((i=1;i<=n_hosts;i++))
do
    mkfifo "fifo_$i.tmp"
    eval "exec ${i+2}<> fifo_$i.tmp"
    eval "./host -d 0 -m ${i} -l ${lucky_number} &"
done

board=(0 0 0 0 0 0 0 0 0 0 0 0 0)
i=1
for ((a=1;a<=$n_players-7;a++))
do
    for ((b=$a+1;b<=$n_players-6;b++))
    do
        for ((c=$b+1;c<=$n_players-5;c++))
        do
            for ((d=$c+1;d<=$n_players-4;d++))
            do
                for ((e=$d+1;e<=$n_players-3;e++))
                do
                    for ((f=$e+1;f<=$n_players-2;f++))
                    do
                        for ((g=$f+1;g<=$n_players-1;g++))
                        do
                            for ((h=$g+1;h<=$n_players;h++))
                            do
                                # echo "$a $b $c $d $e $f $g $h"
                                if [ $i -le $n_hosts ]
                                then
                                    echo "$a $b $c $d $e $f $g $h" > "fifo_${i}.tmp"
                                    i=$((i+1))
                                else
                                    read host_id < "fifo_0.tmp"
                                    # echo $host_id
                                    for ((j=1;j<=8;j++))
                                    do
                                        read -r id num < "fifo_0.tmp"
                                        board[${id}]=$(($num+${board[${id}]}))
                                        # echo $id $num
                                    done
                                    echo "$a $b $c $d $e $f $g $h" > "fifo_${host_id}.tmp"
                                fi
                            done
                        done
                    done
                done
            done
        done
    done
done

while read -t 0.05 line < "fifo_0.tmp" 
do
    for ((j=1;j<=8;j++))
    do
        read -r id num < "fifo_0.tmp"
        board[${id}]=$(($num+${board[${id}]}))
    done
done

for ((id=1;id<=n_players;id++))
do
    rank=1
    for ((p=1;p<=n_players;p++))
    do
        if [ ${board[$id]} -lt ${board[$p]} ]
        then
            rank=$((rank+1))
        fi
    done
    echo $id $rank
done

for ((i=1;i<=n_hosts;i++))
do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > "fifo_$i.tmp"
done

wait
rm -f fifo_*.tmp
exit 0