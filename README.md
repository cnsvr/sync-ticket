# Introduction

This program will be run as multi-threaded by considering synchronization and data consistency issues. The scenario of the simulation is that the clients arrive at the ticket office for seat reservation and tellers reserve the seats for them. The requirements, operations, and flow of the program are as follows:
  * There are 3 different theater halls in the system with the given available number of seats:
    - ODA TIYATROSU, 60 seats
    - USKUDAR STUDYO SAHNE, 80 seats
    - KUCUK SAHNE, 200 seats
  * Each simulation will run on a specific theater hall determined by the configuration file.
  * Each theater hall has a ticket office with 3 tellers; A, B, and C. Tellers make reservations on behalf of the clients.
  * Each ticket office has a common queue, so there are no separate queues for each teller.
Once a teller becomes available, he/she serves the client at the head of the queue.
  * Tellers must be present in the ticket office before the first clients’ arrival.
  * Each client provides four fields of information (via the configuration file): To whose name
will the ticket be registered, arrival time, service time, and the desired seat number.
  * The number of clients is specified by the input configuration file. You can assume that
the number of clients is always a positive integer value that does not exceed 300.
  * The clients arrive at the theater hall sequentially but wait until his/her arrival time.
  * After waiting some time, the client goes to the ticket office and waits for his/her turn in the
queue. Then, he/she goes to one teller and requests the seat stated in the configuration
file for the reservation. Please check Figure 1 to see the simulation in detail.
  * If a requested seat is available, the teller successfully handles the client’s request within the client’s service time. Then, the teller marks the seat as reserved and gives a ticket to
the client.
  * If the requested seat is not available, the teller checks the currently available seats and
reserves the seat with the lowest number, then gives a ticket. If the teller cannot find
such a seat, the client leaves the ticket office.
  * The important thing here is that you need to consider conditions when two different
clients request the same seat at the same time or two tellers try to reserve the same seat at the same time.
