/****************************************************************************************[Dimacs.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Minisat_Dimacs_h
#define Minisat_Dimacs_h

#include <stdio.h>

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"
#include "core/saga.h"

namespace Minisat
{
    std::vector<unsigned> get_sorted_variables_by_occurrence(std::vector<unsigned> &vars_occ_cnt)
    {
        // Create a vector of variables
        size_t num_vars_to_keep = vars_occ_cnt.size() * 0.1;
        std::vector<unsigned> variables(vars_occ_cnt.size());
        for (int i = 0; i < variables.size(); ++i)
            variables[i] = i;

        // Sort the variables based on the number of clauses they appear in
        std::sort(variables.begin(), variables.end(), [&vars_occ_cnt](int a, int b)
                  { return vars_occ_cnt[a] > vars_occ_cnt[b]; });

        std::vector<unsigned> variables_to_keep(variables.begin(), variables.begin() + num_vars_to_keep);
        return variables;
    }

    //=================================================================================================
    // DIMACS Parser:

    template <class B, class Solver>
    static void readClause(B &in, Solver &S, vec<Lit> &lits)
    {
        int parsed_lit, var;
        lits.clear();
        for (;;)
        {
            parsed_lit = parseInt(in);
            if (parsed_lit == 0)
                break;
            var = abs(parsed_lit) - 1;
            while (var >= S.nVars())
                S.newVar();
            lits.push((parsed_lit > 0) ? mkLit(var) : ~mkLit(var));
        }
    }

    template <class B, class Solver>
    static void parse_DIMACS_main(B &in, Solver &S)
    {
        vec<Lit> lits;
        int vars = 0;
        int clauses = 0;
        int cnt = 0;
        for (;;)
        {
            skipWhitespace(in);
            if (*in == EOF)
                break;
            else if (*in == 'p')
            {
                if (eagerMatch(in, "p cnf"))
                {
                    vars = parseInt(in);
                    clauses = parseInt(in);
                    // SATRACE'06 hack
                    // if (clauses > 4000000)
                    //     S.eliminate(true);
                }
                else
                {
                    printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
                }
            }
            else if (*in == 'c' || *in == 'p')
                skipLine(in);
            else
            {
                cnt++;
                readClause(in, S, lits);
                S.addClause_(lits);
            }
        }
        if (vars != S.nVars())
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
        if (cnt != clauses)
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    }

    template <class B, class Solver>
    static void readClause(B &in, Solver &S, vec<Lit> &lits, SAGA::Formula &formula, std::vector<unsigned> &vars_occ_cnt)
    {
        int parsed_lit, var;
        lits.clear();

        for (;;)
        {
            parsed_lit = parseInt(in);
            if (parsed_lit == 0)
                break;
            var = abs(parsed_lit) - 1;
            while (var >= S.nVars())
                S.newVar();
            lits.push((parsed_lit > 0) ? mkLit(var) : ~mkLit(var));
            vars_occ_cnt[var + 1]++;
            // add to formula
            // clause.push_back(SAGA::lit(ncl + 1, abs(parsed_lit), (parsed_lit > 0) ? 1 : 0));

            // if unit clause
            if (lits.size() == 1)
            {
                // formula.clause_delete[cnt] = true;
                formula.fixed_vars[abs(parsed_lit)] = (parsed_lit > 0) ? true : false;
                formula.fix[abs(parsed_lit)] = true;
            }
            // formula.clause_lit_count[ncl]++;
            // formula.var_lit_count[abs(parsed_lit)]++;
        }
    }

    template <class B, class Solver>
    static void parse_DIMACS_main(B &in, Solver &S, SAGA::Formula &formula)
    {
        vec<Lit> lits;
        // std::vector<SAGA::lit> clause;
        int vars = 0;
        int clauses = 0;
        int cnt = 0;
        std::vector<unsigned> vars_occ_cnt;
        for (;;)
        {
            skipWhitespace(in);
            if (*in == EOF)
                break;
            else if (*in == 'p')
            {
                if (eagerMatch(in, "p cnf"))
                {
                    vars = parseInt(in);
                    clauses = parseInt(in);
                    // SATRACE'06 hack
                    // if (clauses > 4000000)
                    //     S.eliminate(true);
                    formula.setNumClauses(clauses);
                    formula.setNumVariables(vars);
                    vars_occ_cnt.resize(vars + 1, 0);
                }
                else
                {
                    printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
                }
            }
            else if (*in == 'c' || *in == 'p')
                skipLine(in);
            else
            {
                readClause(in, S, lits, formula, vars_occ_cnt);
                S.addClause_(lits);
                // formula.addClause(clause, cnt);

                cnt++;
            }
        }
        std::vector<unsigned> sorted_variables = get_sorted_variables_by_occurrence(vars_occ_cnt);
        formula.set_degree_centrality_variables(sorted_variables);
        if (vars != S.nVars())
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
        if (cnt != clauses)
            fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    }

    // Inserts problem into solver.
    //
    template <class Solver>
    static void parse_DIMACS(gzFile input_stream, Solver &S)
    {
        StreamBuffer in(input_stream);
        parse_DIMACS_main(in, S);
    }
    template <class Solver>
    static void parse_DIMACS(gzFile input_stream, Solver &S, SAGA::Formula &formula)
    {
        StreamBuffer in(input_stream);
        parse_DIMACS_main(in, S, formula);
    }

    //=================================================================================================
}

#endif
