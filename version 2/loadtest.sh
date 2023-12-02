if [ $# -ne 4 ]; then
    echo "Usage <number of clients> <loop num> <sleep time> <timeout-secs>"
    exit
fi

plots_path=./plots

gcc -o client client.c
mkdir -p outputs
rm -f outputs/*

for (( i=1 ; i<=$1 ; i++ )); 
do
    ./client 127.0.0.1 5002 cmd.c $2 $3 $i $4 > outputs/op$i.txt &
done
>>$plots_path/results.txt

wait


grep "Average" outputs/*.txt | awk  -v nclients="$1" '
    BEGIN{
        sum=0
        total=0.1
        thru=0
        succ=0
        timeouts=0
        errors=0
    }
    
    {
        sum=sum+($12*$14)
        total=total+$14
        thru=thru+$27
        succ=succ+$2
        timeouts=timeouts+$32
        errors=errors+$37
    }

    END{
        printf("Average time taken = %f ms. Throughput = %f and Successful = %d of %d | Number of clients = %d Timeout-rate = %d Error-rate = %d\n", sum/total, thru, succ, total, nclients, timeouts, errors)
    }
' >> $plots_path/results.txt


