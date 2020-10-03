#/bin/sh

echo "[" > $2

for i in $(seq 1 $3)
do
	if [ $i != 1 ]
	then
		echo -n "," >> $2
	fi
	cat $1 | head -n -1 | tail -n +2  >> $2
done

echo "]"  >> $2
