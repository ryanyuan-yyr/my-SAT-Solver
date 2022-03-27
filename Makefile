all:
	g++ -std=c++17 src/my_sat_solver.cpp src/sat_solver.cpp src/utility.cpp -o build/sat_solver