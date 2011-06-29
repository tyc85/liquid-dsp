/*
 * Copyright (c) 2007, 2008, 2009, 2010 Joseph Gaeddert
 * Copyright (c) 2007, 2008, 2009, 2010 Virginia Polytechnic
 *                                      Institute & State University
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

//
//
//

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "liquid.internal.h"

// generic modulatio function
//  _q          :   modem object
//  _symbol_in  :   input symbol
//  _y          :   output sample
void modem_modulate(modem _q,
                    unsigned int _symbol_in,
                    float complex * _y)
{
    // validate input
    if (_symbol_in >= _q->M) {
        fprintf(stderr,"error: modem_modulate(), input symbol exceeds constellation size\n");
        exit(1);
    }

    _q->modulate_func(_q, _symbol_in, _y);
}

void modem_modulate_ask(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    symbol_in = gray_decode(symbol_in);
    *y = (2*(int)symbol_in - (int)(_mod->M) + 1) * _mod->alpha;
}

void modem_modulate_qam(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    unsigned int s_i, s_q;
    s_i = symbol_in >> _mod->m_q;
    s_q = symbol_in & ( (1<<_mod->m_q)-1 );

    s_i = gray_decode(s_i);
    s_q = gray_decode(s_q);

    *y = (2*(int)s_i - (int)(_mod->M_i) + 1) * _mod->alpha +
         (2*(int)s_q - (int)(_mod->M_q) + 1) * _mod->alpha * _Complex_I;
}

void modem_modulate_psk(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    symbol_in = gray_decode(symbol_in);
    ///\todo: combine into single statement
    float theta = symbol_in * 2 * _mod->alpha;
    *y = cexpf(_Complex_I*theta);
}

void modem_modulate_bpsk(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    *y = symbol_in ? -1.0f : 1.0f;
}

void modem_modulate_qpsk(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    *y  =             (symbol_in & 0x01) ? -0.707106781f : 0.707106781f;
    *y += _Complex_I*((symbol_in & 0x02) ? -0.707106781f : 0.707106781f);
}

// modulate symbol using on/off keying
void modem_modulate_ook(modem _q,
                        unsigned int symbol_in,
                        float complex *y)
{
    *y = symbol_in ? 0.0f : 1.41421356237310f;
}

// modulate symbol with 'square' 32-QAM
void modem_modulate_sqam32(modem _q,
                           unsigned int _symbol_in,
                           float complex * _y)
{
    //*y = symbol_in ? 0.0f : 1.41421356237310f;

    // strip off most-significant two bits (quadrant)
    unsigned int quad = (_symbol_in >> 3) & 0x03;
    float complex r = 1.0f;
    switch (quad) {
    case 0: r =  1.0f;          break;  // rotate by  0
    case 1: r = -_Complex_I;    break;  // rotate by -pi/2
    case 2: r =  _Complex_I;    break;  // rotate by +pi/2
    case 3: r = -1.0f;          break;  // rotate by  pi
    default:
        // should never get to this point
        fprintf(stderr,"error: modem_modulate_sqam32(), logic error\n");
        exit(1);
    }

    // strip off least-significant 3 bits
    unsigned int s = _symbol_in & 0x07;
    
    // return symbol...
    *_y = _q->symbol_map[s] * r;
}

// modulate symbol with 'square' 128-QAM
void modem_modulate_sqam128(modem _q,
                            unsigned int _symbol_in,
                            float complex * _y)
{
    //*y = symbol_in ? 0.0f : 1.41421356237310f;

    // strip off most-significant two bits (quadrant)
    unsigned int quad = (_symbol_in >> 5) & 0x03;
    float complex r = 1.0f;
    switch (quad) {
    case 0: r =  1.0f;          break;  // rotate by  0
    case 1: r = -_Complex_I;    break;  // rotate by -pi/2
    case 2: r =  _Complex_I;    break;  // rotate by +pi/2
    case 3: r = -1.0f;          break;  // rotate by  pi
    default:
        // should never get to this point
        fprintf(stderr,"error: modem_modulate_sqam128(), logic error\n");
        exit(1);
    }

    // strip off least-significant 5 bits
    unsigned int s = _symbol_in & 0x1f;
    
    // return symbol...
    *_y = _q->symbol_map[s] * r;
}

void modem_modulate_dpsk(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    symbol_in = gray_decode(symbol_in);
    float d_theta = symbol_in * 2 * _mod->alpha;
    _mod->state_theta += d_theta;
    _mod->state_theta -= (_mod->state_theta > 2*M_PI) ? 2*M_PI : 0.0f;
    ///\todo: combine into single statement
    *y = cexpf(_Complex_I*(_mod->state_theta));

    _mod->state = *y;
    //printf("mod: state_theta = %f\n", _mod->state_theta);
}

void modem_modulate_apsk(modem _mod,
                         unsigned int symbol_in,
                         float complex *y)
{
    if (symbol_in >= _mod->M) {
        fprintf(stderr,"error: modem_modulate_apsk(), input symbol exceeds maximum\n");
        return;
    }

    // map input symbol to constellation symbol
    unsigned int i;
    unsigned int s = _mod->apsk_symbol_map[symbol_in];

    // determine in which level the symbol is located
    unsigned int p=0;   // level
    unsigned int t=0;   // accumulated number of points per level
    for (i=0; i<_mod->apsk_num_levels; i++) {
        if (s < t + _mod->apsk_p[i]) {
            p = i;
            break;
        }
        t += _mod->apsk_p[i];
    }
    unsigned int s0 = s - t;
    unsigned int s1 = _mod->apsk_p[p];

#if 0
    printf("  s : %3u -> %3u in level %3u (t = %3u) [symbol %3u / %3u]\n", symbol_in, s, p, t, s0,s1);
#endif

    // map symbol to constellation point
    float r = _mod->apsk_r[p];
    float phi = _mod->apsk_phi[p] + (float)(s0)*2.0f*M_PI / (float)(s1);

    *y = r * cexpf(_Complex_I*phi);
}

void modem_modulate_arb(
    modem _mod,
    unsigned int symbol_in,
    float complex *y)
{
    if (symbol_in >= _mod->M) {
        fprintf(stderr,"error: modulate_arb(), input symbol exceeds maximum\n");
        exit(1);
    }

    *y = _mod->symbol_map[symbol_in]; 
}

