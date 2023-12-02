#!/bin/bash
echo Analysing and Generating PNG!!

# Different Sizes of Cache
SIZE='
10
40
80
120
150
'

# Create and clear the file to store the results of the analysis
touch results.txt
> results.txt

cat /dev/null > throughput.txt
cat /dev/null > aat.txt

# Run the analysis for different sizes of threads
for i in ${SIZE}; do
    bash loadtest.sh ${i} 2 0  | tee >(awk -v cl=$i '{printf("%f %f\n", cl, $9)}' >> throughput.txt) >(awk -v cl=$i '{printf("%f %f\n", cl, $5)}' >> aat.txt) 
done

# Plot the throughput results
cat throughput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Throughput" -X "Number of Clients" -Y "Throughput" -r 0.25> ./throughput.png


# Plot the average access time results
cat aat.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Average response time" -X "Number of Clients" -Y "Average response time" -r 0.25> ./aat.png

# rm results.txt

echo Done!!
