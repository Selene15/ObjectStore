#!/bin/bash

# Redirige standard input ed error sul file
exec &> testout.log

obj_pid=$(pidof objectstore)

#Lancio 50 client per i test di tipo 1
for i in {0..49}
do
	./client "user$i" 1 &
done
wait


#Lancio 50 client per i test di tipo 2 e 3
for i in {0..29}
do
	./client "user$i" 2 &
done
	
for i in {30..49}
do
	./client "user$i" 3 &
done
wait

