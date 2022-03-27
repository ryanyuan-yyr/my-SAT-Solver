all:
	g++ -std=c++17 src/my_sat_solver.cpp src/sat_solver.cpp src/sat_solver.hpp utils/utility.hpp -o sat_solver