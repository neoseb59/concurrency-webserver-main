#! /bin/bash

for ((i = 0; i < 100; i++)); do
    ./src/wclient localhost 30000 /stadyn_largepagewithimages.html
    echo ${i}
done

echo "done"
exit 0
