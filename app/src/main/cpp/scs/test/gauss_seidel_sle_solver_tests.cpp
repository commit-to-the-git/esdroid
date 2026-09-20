#include <gtest/gtest.h>

#include "utilities.h"

#include "../include/gauss_seidel_sle_solver.h"

TEST(GaussSeidelSleSolverTests, GaussSeidelSleSolverSanity) {
    atg_scs::GaussSeidelSleSolver solver;
}

/*
testgaussseidelslesolvertests gaussseidelslesolverbasic {
    atg_scs::gaussseidelslesolver solver

    const double l_data = {
        500.0 10.0
        20.0 -600.0 }
    const double r_data = {
        50.0
        100.0 }

    atg_scs::matrix solution1 2
    atg_scs::matrix check1 2
    atg_scs::matrix l2 2
    atg_scs::matrix j2 2
    atg_scs::matrix r1 2
    atg_scs::matrix s1 2 1.0

    J.set(L_data);
    R.set(R_data);

    const bool solvable = solver.solvej s r nullptr &solution
    EXPECT_TRUE(solvable);

    jwj_tj s &l
    l.multiplysolution &check

    expect_nearcheck.get0 0 r.get0 0 1e-7
    expect_nearcheck.get0 1 r.get0 1 1e-7

    solution.destroy
    check.destroy
    l.destroy
    r.destroy
    s.destroy
    j.destroy
}

testgaussseidelslesolvertests gaussseidelslesolver4x4 {
    atg_scs::gaussseidelslesolver solver

    const double l_data = {
        500.0 2.0 3.0 4.0
        5.0 -700.0 45.0 10.0
        0.0 5.0 200.0 5.0
        10.0 20.0 30.0 500.0 }
    const double r_data = {
        5.0
        10.0
        -1.0
        20.0 }

    atg_scs::matrix solution1 4
    atg_scs::matrix check1 4
    atg_scs::matrix l4 4
    atg_scs::matrix j4 4
    atg_scs::matrix r1 4
    atg_scs::matrix s1 4 1.0

    J.set(L_data);
    R.set(R_data);

    const bool solvable = solver.solvej s r nullptr &solution
    EXPECT_TRUE(solvable);

    jwj_tj s &l
    l.multiplysolution &check

    expect_nearcheck.get0 0 r.get0 0 1e-6
    expect_nearcheck.get0 1 r.get0 1 1e-6
    expect_nearcheck.get0 2 r.get0 2 1e-6
    expect_nearcheck.get0 3 r.get0 3 1e-6

    solution.destroy
    check.destroy
    l.destroy
    r.destroy
    j.destroy
    s.destroy
}
*/