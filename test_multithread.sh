#! /bin/bash

for ((i = 0; i < 1000; i++)); do
    ./src/wclient localhost 30000 /test.html
    echo ${i}
done

echo "done"
exit 0
