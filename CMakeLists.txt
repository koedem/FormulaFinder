cmake_minimum_required(VERSION 3.23)
project(FormulaFinder)

set(CMAKE_CXX_STANDARD 23)

add_executable(FormulaFinder main.cpp SimpleClock.h Real.h Operators.h Formula.h MergeFinder.h FormulaFinder.h Utils.h Generator.h Log.h)
