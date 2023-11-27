if [ $# -ne 3 ]; then
    echo "Usage <number of clients> <loop num> <sleep time>"
    exit
fi

gcc server.c -o server
gcc client.c -o client

echo "hello"
mkdir -p outputs
rm -f outputs/*

echo "started"
for((i=1;i<=$1;i++));
do
    ./client 127.0.0.1 5001 cmd.c $2 $3>outputs/op$i.txt &
done

echo "loop over"
wait
cat outputs/*.txt | awk '
    BEGIN{
        sum=0
        total=0.1
        thru=0
    }
    
    {
        printf("Hello")
        sum=sum+($0*$2)
        total=total+$2
        thru=thru+$1
    }

    END{
        printf("Average time taken = %f ms. Throughput = %f\n", sum/total, thru)
    }
' 


