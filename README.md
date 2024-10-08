# OS

## MULTI - THREADED NETWORK SERVER

This document detailing the commands used to run the server and client programs.

# Step 1: Download netcat

  sudo apt install gcc netcat

# Step 2: Compile
Make sure makefile is correctly implemented before running "make" with this code

  assignment3: assignment3.c
    gcc -pthread -o assignment3 assignment3.c

Compile in the ternimal

  make assignment3

# Step 3: Running the server
The template: 

  ./assignment3 -l <portNumber> -p <searchPattern>

with:
- <portNumber> : The port on which the server will listen for incoming connections
- <searchPattern> : The pattern to search for in the received data

Example:

  ./assignment3 -l 12345 -p "happy"


# Step 4: Sending data to the server
In this folder we have 3 books: pg74527.txt ; pg74529.txt ; pg74532.txt

To send data from a file to the server running port, use the following command:

  cat pg74527.txt | nc localhost 12345

To send multiple book files, open new ternimal.

# Step 5: Kill the terminal once finished
