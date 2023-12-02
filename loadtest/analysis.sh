#!/bin/bash
echo Analysing and Generating PNG!!

# Different number of clients
SIZE='
10
20
40
60
100
'

# make empty files
touch results.txt > results.txt

cat /dev/null > throughput.txt
cat /dev/null > aat.txt

# run with different number of clients
for i in ${SIZE}; do
    bash loadtest.sh ${i} 5 0  | tee >(awk -v cl=$i '{printf("%f %f\n", cl, $9)}' >> throughput.txt) >(awk -v cl=$i '{printf("%f %f\n", cl, $5)}' >> aat.txt) 
done

# Plot the throughput data
cat throughput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Throughput" -X "Number of Clients" -Y "Throughput" -r 0.25> ./throughput.png


# Plot the average access time data
cat aat.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Average response time" -X "Number of Clients" -Y "Average response time" -r 0.25> ./aat.png



echo Plotted!!
