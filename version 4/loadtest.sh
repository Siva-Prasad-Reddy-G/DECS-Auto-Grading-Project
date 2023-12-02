
for (( i=1 ; i<=$1 ; i++ )); 
do
    ./client 127.0.0.1 5001 0 check.cpp &
done