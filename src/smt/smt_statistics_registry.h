/******************************************************************************
 * Top contributors (to current version):
 *   Tim King
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Accessor for the SolverEngine's StatisticsRegistry.
 */

#include "cvc5_private.h"

#pragma once

#include "util/statistics_registry.h"
#include "util/statistics_stats.h"

namespace cvc5 {

/**
 * This returns the StatisticsRegistry attached to the currently in scope
 * SolverEngine. This is a synonym for
 * smt::SolverEngineScope::currentStatisticsRegistry().
 */
StatisticsRegistry& smtStatisticsRegistry();

}  // namespace cvc5
