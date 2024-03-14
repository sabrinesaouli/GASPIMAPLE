#include <random>
#include <iostream>
#include <algorithm>
#include "saga.h"

using namespace SAGA;

// Initialize the population with random solutions
void GeneticAlgorithm::initialize_population(std::mt19937 rng)
{

    // Create a uniform distribution for integers in [0, 1]
    std::uniform_int_distribution<int> dist(0, 1);

    int nclauses = formula_.getNumClauses();
    int nvars = formula_.getNumVariables();
    for (std::size_t i = 0; i < population_size_; i++)
    {
        // Create a new solution with random values in its vector
        Solution sol(nvars + 1, nclauses);
        assert(sol.getSolution().size() == nvars + 1);
        assert(formula_.fix.size() == nvars + 1);
        assert(formula_.fixed_vars.size() == nvars + 1);
        for (std::size_t j = 1; j <= nvars; ++j)
        {
            if (formula_.fix[j])
                sol[j] = formula_.fixed_vars[j] ? 1 : 0;
            else
                sol[j] = dist(rng); // generate a random bit using the distribution and the generator
        }
        population_.push_back(sol); // Add the solution to the population
    }
}

int GeneticAlgorithm::fitness(Solution &solution)
{

    if (memo.contains(solution.getSolution()))
    {
        // std::cout << "memo hit" << std::endl;
        int fit = memo.get(solution.getSolution()).get_value_or(formula_.getNumClauses());
        return fit;
    }
    else
    {
        bool sat = false;
        int fitness = 0;
        Minisat::ClauseAllocator &ca = solver_.getCa();
        Minisat::vec<Minisat::CRef> &clauses = solver_.getOriginalclauses();
        assert(clauses.size() != 0);
        assert(ca.size() != 0);
        std::vector<int> unsat_vars;

        for (int c = 0; c < clauses.size(); ++c)
        {
            // std::cout << "Start looking for unsat clauses" << std::endl;
            sat = 0;

            Minisat::Clause &clause = ca[clauses[c]];

            for (int j = 0; j < clause.size(); ++j)
            {
                int cur_var = Minisat::var(clause[j]) + 1;

                if (Minisat::sign(clause[j]) == solution[cur_var])
                {
                    sat += 1;
                    break;
                }
                // else
                // {
                //     if (!formula_.fix[cur_var])
                //     {
                //         // std::cout << "cur_var " << cur_var << std::endl;
                //         if (std::find(formula_.get_degree_centrality_variables().begin(), formula_.get_degree_centrality_variables().end(), cur_var) != formula_.get_degree_centrality_variables().end())
                //             unsat_vars.push_back(cur_var);
                //     }
                // }
            }

            if (sat == 0)
            {
                // add all unsat variables to the solution
                // for (auto lit : unsat_vars)
                //     solution.add_unsatisfying_variable(lit);
                fitness++;
            }
            // else
            // {
            //     unsat_vars.clear();
            // }
        }

        memo.insert(solution.getSolution(), fitness);
        return fitness;
    }
}

// Evaluate the fitness of each solution in the population
void GeneticAlgorithm::evaluate_fitness()
{
    for (std::size_t i = 0; i < population_size_; ++i)
    {
        assert(population_[i].size() == formula_.getNumVariables() + 1);
        assert(i < population_.getPopulation().size());
        // Evaluate the fitness of each solution using the fitness function
        // std::cout << "fitness = " << population_.population[i].fitness << std::endl;
        population_[i].setFitness(fitness(population_[i]));
    }
    // Sort the population by fitness in descending order (best solutions first)
    // std::sort(population_.getPopulation().begin(), population_.getPopulation().end(), [](const Solution &a, const Solution &b)
    //           { return a.getFitness() < b.getFitness(); });
    population_.sort();
    // std::cout << "best fitness = " << population_.population[0].fitness << std::endl;
}
// Evaluate the fitness of the offspring
void GeneticAlgorithm::evaluate_fitness(std::vector<Solution> &offspring)
{
    for (auto &sol : offspring)
    {
        sol.setFitness(fitness(sol));
    }
}

// Select parents using tournament selection
std::vector<Solution> GeneticAlgorithm::select_parents_tournament(std::mt19937 rng)
{
    // Create a vector to store the selected parents
    std::vector<Solution> parents;
    parents.reserve(population_size_ / 2);
    // Create a uniform distribution for integers in [0, population_size_ - 1]
    std::uniform_int_distribution<int> dist(0, population_size_ - 1);

    for (std::size_t i = 0; i < (population_size_ / 2); ++i)
    {
        // Select two random solutions from the population as candidates for reproduction
        int index1 = dist(rng);
        int index2 = dist(rng);
        Solution &candidate1 = population_[index1];
        Solution &candidate2 = population_[index2];

        // Compare their fitness and select the fitter one as a parent
        if (candidate1.getFitness() < candidate2.getFitness())
        {
            parents.push_back(std::move(candidate1));
        }
        else
        {
            parents.push_back(std::move(candidate2));
        }
    }

    return parents;
}

std::vector<Solution> GeneticAlgorithm::select_parents_random(std::mt19937 rng)
{
    std::vector<Solution> parents;
    parents.reserve(population_size_ / 2);
    // Create a uniform distribution for integers in [0, population_size_ - 1]
    std::uniform_int_distribution<int> dist(0, population_size_ - 1);
    for (int i = 0; i < (population_size_ / 2); ++i)
    {
        int index = dist(rng);
        parents.push_back(std::move(population_[index]));
    }
    return parents;
}

// void GeneticAlgorithm::mutate(Solution &solution, std::mt19937 rng)
// {
//     // Create a uniform distribution for floats in [0.0, 1.0]
//     std::uniform_real_distribution<float> dist(0.0f, 1.0f);

//     if (solution.no_unsatisfying_variables())
//     {
//         std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
//         for (auto &var : centrality_vars)
//         {

//             if (dist(rng) < mutation_rate_ && !formula_.fix[var])
//             {
//                 solution[var] = 1 - solution[var];
//             }
//         }
//     }
//     else
//     {

//         std::vector<int> unsat_vars = solution.get_unsatisfying_variables();
//         for (auto &var : unsat_vars)
//         {
//             if (dist(rng) < mutation_rate_ && !formula_.fix[var])
//             {
//                 solution[var] = 1 - solution[var];
//             }
//         }
//     }
// }

// void GeneticAlgorithm::mutate(Solution &solution, std::mt19937 rng)
// {
//     // Create a uniform distribution for floats in [0.0, 1.0]
//     std::uniform_real_distribution<float> dist(0.0f, 1.0f);
//     std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
//     for (auto &var : centrality_vars)
//     {

//         if (dist(rng) < mutation_rate_ && !formula_.fix[var])
//         {
//             solution[var] = 1 - solution[var];
//         }
//     }
// }

// Create offspring through crossover and mutation
std::vector<Solution> GeneticAlgorithm::create_offspring(const std::vector<Solution> &parents, std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(parents.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);
    // Create a uniform distribution for integers in [0, parents.size() - 1]
    std::uniform_int_distribution<int> dist3(0, parents.size() - 1);

    for (std::size_t i = 0; i < parents.size() - 1; i += 2)
    {
        // Select two parents from the vector using their index
        Solution parent1 = parents[i];
        Solution parent2 = parents[i + 1];
        // Solution parent1 = parents[dist3(rng)];
        // Solution parent2 = parents[dist3(rng)];

        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point = dist2(rng);
            // std::cout << "point: " << point << std::endl;

            for (int j = 1; j <= point; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // Evaluate the fitness of the offspring to compute the unsatisfying variables
        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }

    return offspring;
}

Solution GeneticAlgorithm::select_parent(std::mt19937 rng)
{
    // Create a uniform distribution for integers in [0, population_size_ - 1]
    std::uniform_int_distribution<int> dist(0, population_size_ - 1);
    int index = dist(rng);
    return population_[index];
}

std::vector<Solution> GeneticAlgorithm::create_offspring(std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(population_.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);

    for (std::size_t i = 0; i < population_.size() / 2; i += 2)
    {
        // Select two parents from the vector using their index
        Solution parent1 = select_parent(rng);
        Solution parent2 = select_parent(rng);

        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point = dist2(rng);
            // std::cout << "point: " << point << std::endl;

            for (int j = 1; j <= point; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // // Evaluate the fitness of the offspring
        // child1.setFitness(fitness(child1));
        // child2.setFitness(fitness(child2));

        // // Perform mutation with a given probability
        // if (child1.no_unsatisfying_variables())
        // {
        //     int mut_var = dist2(rng);
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         if (!formula_.fix[mut_var])
        //         {
        //             child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     std::vector<std::pair<int, int>> unsat_vars = child1.get_unsatisfying_variables();
        //     assert(!child1.no_unsatisfying_variables());
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child1[unsat_var] = 1 - child1[unsat_var];
        //             break;
        //         }
        //     }
        //     // int mut_var = child1.getRandomUnsatisfyingVariable();
        //     // // std::cout << "mut_var: " << mut_var << std::endl;
        //     // assert(!formula_.fix[mut_var]);
        //     // child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        // }

        // if (child2.no_unsatisfying_variables())
        // {
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         int mut_var = dist2(rng);
        //         if (!formula_.fix[mut_var])
        //         {
        //             child2[mut_var] = 1 - child2[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     assert(!child2.no_unsatisfying_variables());

        //     std::vector<std::pair<int, int>> unsat_vars = child2.get_unsatisfying_variables();
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child2[unsat_var] = 1 - child2[unsat_var];
        //         }
        //     }
        // }
        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }

    return offspring;
}

std::vector<Solution> GeneticAlgorithm::create_offspring_two_points(std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(population_.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);

    for (size_t i = 0; i < population_.size() / 2; i++)
    {
        // Select two parents from the vector using their index
        Solution parent1 = select_parent(rng);
        Solution parent2 = select_parent(rng);

        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point1 = dist2(rng);
            int point2 = dist2(rng);
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }
            for (int j = 1; j <= point1; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
            for (int j = point2; j < solution_size_; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // Evaluate the fitness of the offspring

        // child1.setFitness(fitness(child1));
        // child2.setFitness(fitness(child2));

        // // Perform mutation with a given probability
        // if (child1.no_unsatisfying_variables())
        // {
        //     int mut_var = dist2(rng);
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         if (!formula_.fix[mut_var])
        //         {
        //             child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     std::vector<std::pair<int, int>> unsat_vars = child1.get_unsatisfying_variables();
        //     assert(!child1.no_unsatisfying_variables());
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child1[unsat_var] = 1 - child1[unsat_var];
        //             break;
        //         }
        //     }
        //     // int mut_var = child1.getRandomUnsatisfyingVariable();
        //     // // std::cout << "mut_var: " << mut_var << std::endl;
        //     // assert(!formula_.fix[mut_var]);
        //     // child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        // }

        // if (child2.no_unsatisfying_variables())
        // {
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         int mut_var = dist2(rng);
        //         if (!formula_.fix[mut_var])
        //         {
        //             child2[mut_var] = 1 - child2[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     assert(!child2.no_unsatisfying_variables());

        //     std::vector<std::pair<int, int>> unsat_vars = child2.get_unsatisfying_variables();
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child2[unsat_var] = 1 - child2[unsat_var];
        //         }
        //     }
        // }

        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }
    return offspring;
}

// Create offspring through two points crossover and mutation
std::vector<Solution> GeneticAlgorithm::create_offspring_two_points(const std::vector<Solution> &parents, std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(parents.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);
    // Create a uniform distribution for integers in [0, parents.size() - 1]
    std::uniform_int_distribution<int> dist3(0, parents.size() - 1);

    for (std::size_t i = 0; i < parents.size() - 1; i += 2)
    {
        // Select two parents from the vector using their index
        Solution parent1 = parents[i];
        Solution parent2 = parents[i + 1];
        // Solution parent1 = parents[dist3(rng)];
        // Solution parent2 = parents[dist3(rng)];

        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point1 = dist2(rng);
            int point2 = dist2(rng);
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }
            for (int j = 1; j <= point1; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
            for (int j = point2; j < solution_size_; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // // Evaluate the fitness of the offspring
        // child1.setFitness(fitness(child1));
        // child2.setFitness(fitness(child2));

        // // Perform mutation with a given probability
        // if (child1.no_unsatisfying_variables())
        // {
        //     int mut_var = dist2(rng);
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         if (!formula_.fix[mut_var])
        //         {
        //             child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     std::vector<std::pair<int, int>> unsat_vars = child1.get_unsatisfying_variables();
        //     assert(!child1.no_unsatisfying_variables());
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child1[unsat_var] = 1 - child1[unsat_var];
        //             break;
        //         }
        //     }
        //     // int mut_var = child1.getRandomUnsatisfyingVariable();
        //     // // std::cout << "mut_var: " << mut_var << std::endl;
        //     // assert(!formula_.fix[mut_var]);
        //     // child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        // }

        // if (child2.no_unsatisfying_variables())
        // {
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         int mut_var = dist2(rng);
        //         if (!formula_.fix[mut_var])
        //         {
        //             child2[mut_var] = 1 - child2[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     assert(!child2.no_unsatisfying_variables());

        //     std::vector<std::pair<int, int>> unsat_vars = child2.get_unsatisfying_variables();
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child2[unsat_var] = 1 - child2[unsat_var];
        //         }
        //     }
        // }
        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }

    return offspring;
}

// Create offspring through three points crossover and mutation
std::vector<Solution> GeneticAlgorithm::create_offspring_three_points(const std::vector<Solution> &parents, std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(parents.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);
    // Create a uniform distribution for integers in [0, parents.size() - 1]
    std::uniform_int_distribution<int> dist3(0, parents.size() - 1);

    for (std::size_t i = 0; i < parents.size() - 1; i += 2)
    {
        // Select two parents from the vector using their index
        Solution parent1 = parents[i];
        Solution parent2 = parents[i + 1];
        // Solution parent1 = parents[dist3(rng)];
        // Solution parent2 = parents[dist3(rng)];

        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point1 = dist2(rng);
            int point2 = dist2(rng);
            int point3 = dist2(rng);

            // sort the points
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }
            if (point2 > point3)
            {
                std::swap(point2, point3);
            }
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }

            for (int j = 1; j <= point1; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
            for (int j = point2; j <= point3; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // Evaluate the fitness of the offspring
        // child1.setFitness(fitness(child1));
        // child2.setFitness(fitness(child2));

        // Perform mutation with a given probability
        // if (child1.no_unsatisfying_variables())
        // {
        //     int mut_var = dist2(rng);
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         if (!formula_.fix[mut_var])
        //         {
        //             child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        // std::vector<std::pair<int, int>> unsat_vars = child1.get_unsatisfying_variables();
        // assert(!child1.no_unsatisfying_variables());
        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }
        // int mut_var = child1.getRandomUnsatisfyingVariable();
        // // std::cout << "mut_var: " << mut_var << std::endl;
        // assert(!formula_.fix[mut_var]);
        // child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        // }

        // if (child2.no_unsatisfying_variables())
        // {
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         int mut_var = dist2(rng);
        //         if (!formula_.fix[mut_var])
        //         {
        //             child2[mut_var] = 1 - child2[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        // assert(!child2.no_unsatisfying_variables());

        // std::vector<std::pair<int, int>> unsat_vars = child2.get_unsatisfying_variables();
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child2[unsat_var] = 1 - child2[unsat_var];
        //         }
        //     }
        // }
        // std::vector<int> centrality_vars = formula_.get_degree_centrality_variables();

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }

    return offspring;
}

// Create offspring through three points crossover and mutation
std::vector<Solution> GeneticAlgorithm::create_offspring_three_points(std::mt19937 rng)
{
    // Create a vector to store the offspring solutions
    std::vector<Solution> offspring;
    offspring.reserve(population_.size());

    // Create a uniform distribution for floats in [0.0, 1.0]
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    // Create a uniform distribution for integers in [1, solution_size_ - 2]
    std::uniform_int_distribution<int> dist2(1, solution_size_ - 2);

    for (std::size_t i = 0; i < population_.size() / 2; i += 2)
    {
        // Select two parents from the vector using their index
        Solution parent1 = select_parent(rng);
        Solution parent2 = select_parent(rng);
        Solution child1(parent1);
        Solution child2(parent2);

        // Perform crossover with a given probability
        if (dist(rng) < crossover_rate_)
        {
            // Select a random point to split the solution vector
            int point1 = dist2(rng);
            int point2 = dist2(rng);
            int point3 = dist2(rng);

            // sort the points
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }
            if (point2 > point3)
            {
                std::swap(point2, point3);
            }
            if (point1 > point2)
            {
                std::swap(point1, point2);
            }

            for (int j = 1; j <= point1; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
            for (int j = point2; j <= point3; ++j)
            {
                if (!formula_.fix[j])
                {
                    child1[j] = parent2[j];
                    child2[j] = parent1[j];
                }
            }
        }

        // Evaluate the fitness of the offspring
        // child1.setFitness(fitness(child1));
        // child2.setFitness(fitness(child2));

        // // Perform mutation with a given probability
        // if (child1.no_unsatisfying_variables())
        // {
        //     int mut_var = dist2(rng);
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         if (!formula_.fix[mut_var])
        //         {
        //             child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     std::vector<std::pair<int, int>> unsat_vars = child1.get_unsatisfying_variables();
        //     assert(!child1.no_unsatisfying_variables());
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child1[unsat_var] = 1 - child1[unsat_var];
        //             break;
        //         }
        //     }
        //     // int mut_var = child1.getRandomUnsatisfyingVariable();
        //     // // std::cout << "mut_var: " << mut_var << std::endl;
        //     // assert(!formula_.fix[mut_var]);
        //     // child1[mut_var] = 1 - child1[mut_var]; // flip the bit
        // }

        // if (child2.no_unsatisfying_variables())
        // {
        //     if (dist(rng) < mutation_rate_)
        //     {
        //         int mut_var = dist2(rng);
        //         if (!formula_.fix[mut_var])
        //         {
        //             child2[mut_var] = 1 - child2[mut_var]; // flip the bit
        //         }
        //     }
        // }
        // else
        // {

        //     assert(!child2.no_unsatisfying_variables());

        //     std::vector<std::pair<int, int>> unsat_vars = child2.get_unsatisfying_variables();
        //     for (auto &var : unsat_vars)
        //     {
        //         int unsat_var = var.first;
        //         assert(!formula_.fix[unsat_var]);
        //         if (dist(rng) < mutation_rate_)
        //         {
        //             child2[unsat_var] = 1 - child2[unsat_var];
        //         }
        //     }
        // }
        std::vector<unsigned> &centrality_vars = formula_.get_degree_centrality_variables();
        for (auto &var : centrality_vars)
        {
            // int unsat_var = var.first;
            // assert(!formula_.fix[var]);
            if (dist(rng) < mutation_rate_ && !formula_.fix[var])
            {
                child1[var] = 1 - child1[var];
                child2[var] = 1 - child1[var];
            }
        }

        // Add the offspring to the vector
        offspring.push_back(std::move(child1));
        offspring.push_back(std::move(child2));
    }

    return offspring;
}

// Select the best solutions to survive to the next generation
void GeneticAlgorithm::select_survivors(const std::vector<Solution> &offspring)
{
    // Combine the population and offspring
    std::vector<Solution> combined(population_.getPopulation());
    // combined.resize(combined.size() + offspring.size());
    combined.insert(combined.end(), offspring.begin(), offspring.end());
    // std::cout << "combined size before: " << combined.size() << std::endl;
    // Select the best solutions to survive
    size_t erasureCount = 0;
    size_t maxErasures = combined.size() - population_size_;
    if (maxErasures > 0)                                                                                                               // Maximum allowed erasures
        combined.erase(unique(combined.begin(), combined.end(), [&erasureCount, maxErasures](const Solution &s1, const Solution &s2) { // Lambda function to compare solutions
                           if (erasureCount < maxErasures)
                           {
                               ++erasureCount;
                               return s1 == s2;
                           }
                           else
                           {
                               return false;
                           }
                       }),
                       combined.end());

    // Sort the combined vector by fitness in ascending order
    std::sort(combined.begin(), combined.end(),
              [](const Solution &s1, const Solution &s2)
              { return s1.getFitness() < s2.getFitness(); });

    population_.setPopulation(std::vector<Solution>(combined.begin(), combined.begin() + population_size_));
    assert(population_.getPopulation().size() == population_size_);
    combined.clear();
    combined.shrink_to_fit();
}

// Select the best solutions to survive to the next generation
void GeneticAlgorithm::select_survivors_ellitist(const std::vector<Solution> &offspring)
{
    // Combine the population and offspring
    std::vector<Solution> combined(population_.getPopulation());
    // combined.resize(combined.size() + offspring.size());
    combined.insert(combined.end(), offspring.begin(), offspring.end());
    // std::cout << "combined size before: " << combined.size() << std::endl;
    // Select the best solutions to survive
    size_t erasureCount = 0;
    size_t maxErasures = combined.size() - population_size_;
    if (maxErasures > 0)                                                                                                               // Maximum allowed erasures
        combined.erase(unique(combined.begin(), combined.end(), [&erasureCount, maxErasures](const Solution &s1, const Solution &s2) { // Lambda function to compare solutions
                           if (erasureCount < maxErasures)
                           {
                               ++erasureCount;
                               return s1 == s2;
                           }
                           else
                           {
                               return false;
                           }
                       }),
                       combined.end());

    // Sort the combined vector by fitness in ascending order
    std::sort(combined.begin(), combined.end(),
              [](const Solution &s1, const Solution &s2)
              { return s1.getFitness() < s2.getFitness(); });

    // Select the 50% best and 50% worst solutions to survive
    size_t nbest = population_size_ / 2;
    size_t nworst = population_size_ - nbest;

    std::vector<Solution> survivors;
    survivors.reserve(population_size_);
    survivors.insert(survivors.end(), combined.begin(), combined.begin() + nbest);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(combined.begin() + nbest, combined.end(), g);
    survivors.insert(survivors.end(), combined.begin() + nbest, combined.begin() + nbest + nworst);
    // survivors.insert(survivors.end(), combined.end() - nworst, combined.end());
    population_.setPopulation(survivors);
    assert(population_.getPopulation().size() == population_size_);
    combined.clear();
    combined.shrink_to_fit();
    survivors.clear();
    survivors.shrink_to_fit();
}

// void GeneticAlgorithm::select_survivors(const std::vector<Solution> &offspring)
// {
//     // Sort the population and offspring by fitness
//     std::vector<Solution> combined;
//     combined.reserve(population_size_ + offspring.size());
//     combined.insert(combined.end(), population_.population.begin(), population_.population.end());
//     combined.insert(combined.end(), offspring.begin(), offspring.end());
//     std::sort(combined.begin(), combined.end(), [](const Solution &s1, const Solution &s2)
//               { return s1.fitness < s2.fitness; });

//     // Select the best solutions to survive
//     population_.population.clear();
//     for (int i = 0; i < population_size_; ++i)
//     {

//         population_.population.push_back(std::move(combined[i]));
//     }
// }

// Check if a satisfactory solution has been found
bool GeneticAlgorithm::solution_found()
{
    // Check if the fittest solution satisfies all the clauses
    Solution &fittest = population_[0];
    return fittest.getFitness() == 0;
}

// Get the fittest solution
Solution &GeneticAlgorithm::getBestSolution()
{
    return population_[0];
}

// Get the worst solution
Solution &GeneticAlgorithm::getWorstSolution()
{
    return population_[population_size_ - 1];
}

void SAGA::initialize_polarity(Solution &solution, Minisat::Solver &solver)
{
    int num_vars = solver.nVars();
    for (int i = 0; i < num_vars; ++i)
    {
        solver.setPolarity(i, solution[i + 1] ? true : false);
    }
}