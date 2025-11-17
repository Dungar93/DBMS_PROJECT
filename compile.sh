#!/bin/bash
echo "--- Cleaning old files ---"
rm -f pflayer/*.o
rm -f amlayer/*.o
rm -f test_pf_stats test_hf test_am

echo "--- 1. Building PF/HF Layer (pflayer) ---"
make -C pflayer
if [ $? -ne 0 ]; then
    echo "PF/HF Layer build failed."
    exit 1
fi

echo "--- 2. Building AM Layer (amlayer) ---"
make -C amlayer
if [ $? -ne 0 ]; then
    echo "AM Layer build failed."
    exit 1
fi

echo "--- 3. Linking AM Layer Object (amlayer.o) ---"
ld -r -o ./amlayer/amlayer.o ./amlayer/am.o ./amlayer/amfns.o ./amlayer/amsearch.o ./amlayer/aminsert.o ./amlayer/amstack.o ./amlayer/amglobals.o ./amlayer/amscan.o ./amlayer/amprint.o

echo "--- 4. Compiling Test Programs ---"
cc -o test_pf_stats test_pf_stats.c -I./pflayer ./pflayer/pflayer.o
cc -o test_hf test_hf.c -I./pflayer ./pflayer/pflayer.o
cc -o test_am test_am.c -I./pflayer -I./amlayer ./pflayer/pflayer.o ./amlayer/amlayer.o

echo "--- Build Complete ---"