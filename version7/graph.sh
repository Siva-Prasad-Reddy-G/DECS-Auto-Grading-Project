echo Analysing and Generating PNG

clients='
10
20
40
60
80
'

touch results.txt
>results.txt

touch throughput.txt
touch avgresp.txt
>throughput.txt
>avgresp.txt

for i in ${clients};
do
	bash loadtest.sh ${i} 2 0 | tee >(awk -v cl=$i '{printf("%f %f\n",cl,$9)}'>> throughput.txt) >(awk -v cl=$i '{printf("%f %f\n",cl,$5)}'>>avgresp.txt)
done

# Plot the throughput results
cat throughput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Throughput" -X "Number of Clients" -Y "Throughput" -r 0.25> ./throughput.png


# Plot the average access time results
cat avgresp.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Average response time" -X "Number of Clients" -Y "Average response time" -r 0.25> ./aat.png

# rm results.txt

echo Done!!
