if [ $# -ne 3 ]; then
    echo "Usage <number of clients> <loop num> <sleep time>"
    exit
fi


mkdir -p outputs
rm -f outputs/*

# send processes to background to create load on server
for (( i=1 ; i<=$1 ; i++ )); 
do
    ./client 127.0.0.1 3000 pr.cpp $2 $3 $i > outputs/op$i.txt &
done

#wait till all are done
wait

#calculate average response time and throughput

grep "result" outputs/*.txt | awk '
    BEGIN{
        sum=0
        total=0
        thru=0
    }
    
    {
        sum=sum+($2*$3)
        total=total+$3
        thru=thru+$4
    }

    END{
        printf("Average time taken = %f ms. Throughput = %f\n", sum/total, thru)
    }
' 

