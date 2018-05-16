#!/bin/sh

./logappend -T 1 -K secret -A -E Fred log1
./logappend -T 2 -K secret -A -E Jill log1
./logappend -T 3 -K secret -A -E Tim log1
./logappend -T 4 -K secret -A -E Bob log1
./logappend -T 5 -K secret -A -E Steve log1
./logappend -T 6 -K secret -A -E Doug log1
./logappend -T 7 -K secret -A -E Joe log1
./logappend -T 8 -K secret -A -E Ron log1
./logappend -T 9 -K secret -A -E Hunter log1
./logappend -T 10 -K secret -A -E Fred -R 1 log1
./logappend -T 11 -K secret -A -E Ron -R 1 log1
./logappend -T 12 -K secret -A -E Joe -R 1  log1
./logappend -T 13 -K secret -A -E Bob -R 2 log1
./logappend -T 14 -K secret -A -E Jill -R 144 log1
./logappend -T 15 -K secret -A -E Steve -R 16 log1


