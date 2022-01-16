#!/bin/bash

if [ ! -f ./testout.log ]; then
    echo "Log file not found"
    exit
fi


function print_report () {
    # Numero totale di test per la batteria
    total=$(grep -c "Test $1" testout.log)
    # Numero di test passati
    let success=$(grep -c "Test $1: Success" testout.log)
    # Numero di test falliti
    let failed=$total-$success
    # Percentuale di test passati e falliti
    # Stampa il sommario delle informazioni
    echo "Test $1: Lanciati $total, Passati $success, Falliti $failed "
}

# Numero totale di test
total=$(cat testout.log | wc -l)

# Stampa il report per ogni batteria
echo "TEST EXECUTED: $total"
for i in {1..3} 
do
    print_report $i
done

# Manda un segnale di diagnostica al server
kill -USR1 $(pidof objectstore)
wait
# Termina il server
kill -TERM $(pidof objectstore)
