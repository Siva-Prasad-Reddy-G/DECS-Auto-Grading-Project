if [ $# -ne 3 ]; then
    echo "Usage <number of clients> <loop num> <sleep time>"
    exit
fi

gcc -o client client.c
mkdir -p outputs
rm -f outputs/*

for (( i=1 ; i<=$1 ; i++ )); 
do
    ./client 127.0.0.1 5002 cmd.c $2 $3 $i > outputs/op$i.txt &
done

wait


grep "Average" outputs/*.txt | awk '
    BEGIN{
        sum=0
        total=0
        thru=0
    }
    
    {
        sum=sum+($10*$12)
        total=total+$12
        thru=thru+$24
    }

    END{
        printf("Average time taken = %f ms. Throughput = %f\n", sum/total, thru)
    }
' 


