#!/bin/bash

#SYM="$1"
#shift

SIZE_POP="$1"
shift

MAX_GEN="$1"
shift

MUT_RATE="$1"
shift

CROSS_RATE="$1"
shift

CNF="$1"
shift

SOLVER="$(dirname 0)/minisat_static"

# Verify SYM
#if [ "$SYM" != "symmetry" ] && [ "$SYM" != "no-symmetry" ]; then
#  echo "Error: Invalid SYM value. Valid values are 'symmetry' or 'no-symmetry'."
#  exit 1
#fi

# Verify SIZE_POP, MAX_GEN, MUT_RATE, CROSS_RATE
if ! [[ "$SIZE_POP" =~ ^[0-9]+$ ]]; then
  echo "Error: SIZE_POP must be a positive integer."
  exit 1
fi

if ! [[ "$MAX_GEN" =~ ^[0-9]+$ ]]; then
  echo "Error: MAX_GEN must be a positive integer."
  exit 1
fi

if ! [[ "$MUT_RATE" =~ ^[0-9]*\.?[0-9]+$ ]]; then
  echo "Error: MUT_RATE must be a decimal number."
  exit 1
fi

if ! [[ "$CROSS_RATE" =~ ^[0-9]*\.?[0-9]+$ ]]; then
  echo "Error: CROSS_RATE must be a decimal number."
  exit 1
fi

# Verify CNF file existence
if [ ! -f "$CNF" ]; then
  echo "Error: CNF file '$CNF' does not exist."
  exit 1
fi


$SOLVER -saga-pol-init -pop-size="$SIZE_POP" -max-generations="$MAX_GEN" -mutation-rate="$MUT_RATE" -crossover-rate="$CROSS_RATE"  "$CNF"

