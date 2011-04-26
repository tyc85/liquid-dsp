/*
 * Copyright (c) 2011 Joseph Gaeddert
 * Copyright (c) 2011 Virginia Polytechnic Institute & State University
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIQUID_BENCHMARKGEN_H__
#define __LIQUID_BENCHMARKGEN_H__

typedef struct benchmarkgen_s * benchmarkgen;

// create benchmark generator object
benchmarkgen benchmarkgen_create();

void benchmarkgen_parse(benchmarkgen _q,
                        char * _filename);

void benchmarkgen_print(benchmarkgen _q);

void benchmarkgen_destroy(benchmarkgen _q);

#endif // __LIQUID_BENCHMARKGEN_H__
