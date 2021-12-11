#! /bin/bash

for ((i = 0; i < 10; i++)); do
    ./src/wclient localhost 30000 "/spin.cgi?20" &
    echo ${i}
done

wait

echo "done"
exit 0
