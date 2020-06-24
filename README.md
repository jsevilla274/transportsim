# Reliable Transport Protocol Simulator
Implementations of the rdt3.0 and Go-Back-N protocols described in the textbook Computer Networking: A Top-Down Approach 6th Edition by James Kurose and Keith Ross using a slightly modified version of the "ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1" described in https://media.pearsoncmg.com/aw/aw_kurose_network_3/labs/lab5/lab5.html

My implementations of the protocols are in the files "prog2_rdt.c" and "prog2_gbn.c" between the comment labels "STUDENT CODE START" and "STUDENT CODE END". Both implementations are unidirectional with the A entity being the sender and the B entity being the receiver. Simply compile any of these files into an executable and run.

## prog2_rdt.c (rdt3.0 or "Alternating Bit protocol")
To test this implementation, it is recommended you run it with the following start prompt settings:
- **Number of messages to simulate:** 10
- **Loss probability:** 0.1
- **Corruption probability:** 0.3
- **Average time between Layer 5 messages:** 1000
- **Trace level:** 2

## prog2_gbn.c (Go-Back-N protocol)
To test this implementation, it is recommended you run it with the following start prompt settings:
- **Number of messages to simulate:** 20
- **Loss probability:** 0.2
- **Corruption probability:** 0.2
- **Average time between Layer 5 messages:** 200
- **Trace level:** 2

Obviously these are just recommendations and the code should be robust for many combinations of settings. The only constant you may want to tweak is the "TIMEOUT_LEN" as I merely settled on this value after experimentation on my machine.