all: simulation.o 

simulation.o: solution.cpp
							g++ solution.cpp -o solution.o -lpthread 