/*
 * Copyright (c) 2007, 2009 Joseph Gaeddert
 * Copyright (c) 2007, 2009 Virginia Polytechnic Institute & State University
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
// convolutional code (macros)
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if HAVE_FEC_H  // (config.h)
#include "fec.h"

#define VERBOSE_FEC_CONV    0

fec FEC_CONV(_create)(void * _opts)
{
    fec q = (fec) malloc(sizeof(struct fec_s));

    q->scheme = FEC_CONV(_mode);
    q->rate = fec_get_rate(q->scheme);

    q->encode_func = &FEC_CONV(_encode);
    q->decode_func = &FEC_CONV(_decode);

    // convolutional-specific decoding
    q->num_framebits = 0;
    q->dec_bits = NULL;
    q->enc_bits = NULL;
    q->vp = NULL;

    return q;
}

void FEC_CONV(_destroy)(fec _q)
{
    // delete viterbi decoder
    if (_q->vp != NULL)
        delete_viterbi(_q->vp);

    free(_q);
}

void FEC_CONV(_encode)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char *_msg_dec,
                       unsigned char *_msg_enc)
{
    unsigned int i,j,r; // bookkeeping
    unsigned int sr=0;  // convolutional shift register
    unsigned int n=0;   // output bit counter

    unsigned char bit;
    unsigned char byte_in;
    unsigned char byte_out=0;

    for (i=0; i<_dec_msg_len; i++) {
        byte_in = _msg_dec[i];

        // break byte into individual bits
        for (j=0; j<8; j++) {
            // shift bit starting with most significant
            bit = (byte_in >> (7-j)) & 0x01;
            sr = (sr << 1) | bit;

            // compute parity bits for each polynomial
            for (r=0; r<FEC_CONV(_R); r++) {
                byte_out = (byte_out<<1) | parity(sr & convpoly[r]);
                _msg_enc[n/8] = byte_out;
                n++;
            }
        }
    }

    // tail bits
    for (i=0; i<FEC_CONV(_K); i++) {
        // shift register: push zeros
        sr = (sr << 1);

        // compute parity bits for each polynomial
        for (r=0; r<FEC_CONV(_R); r++) {
            byte_out = (byte_out<<1) | parity(sr & convpoly[r]);
            _msg_enc[n/8] = byte_out;
            n++;
        }
    }

    // ensure even number of bytes
    while (n%8) {
        // shift zeros
        byte_out <<= 1;
        _msg_enc[n/8] = byte_out;
        n++;
    }

    assert(n == 8*fec_get_enc_msg_length(FEC_CONV(_mode),_dec_msg_len));
}

//unsigned int
void FEC_CONV(_decode)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char *_msg_enc,
                       unsigned char *_msg_dec)
{
    // re-allocate resources if necessary
    FEC_CONV(_setlength)(_q, _dec_msg_len);

    // unpack bytes
    unsigned int num_written;
    unpack_bytes(_msg_enc,          // encoded message (bytes)
                 _dec_msg_len*2+2,  // encoded message length (#bytes)
                 _q->enc_bits,      // encoded messsage (bits)
                 16*_dec_msg_len+16,// encoded message length (#bits)
                 &num_written);

#if VERBOSE_FEC_CONV
    unsigned int i;
    printf("msg encoded (bits):\n");
    for (i=0; i<16*(_dec_msg_len)+6; i++) {
        printf("%1u", _q->enc_bits[i]);
        if (((i+1)%8)==0)
            printf(" ");
    }
    printf("..\n");
#endif

    // invoke hard-decision scaling
    unsigned int k;
    for (k=0; k<16*_dec_msg_len+8; k++)
        _q->enc_bits[k] *= 255;

    // run decoder
    init_viterbi(_q->vp,0);
    update_viterbi_blk(_q->vp, _q->enc_bits, _q->num_framebits+6);
    chainback_viterbi(_q->vp,  _msg_dec,     _q->num_framebits, 0);

#if VERBOSE_FEC_CONV
    for (i=0; i<_dec_msg_len; i++)
        printf("%.2x ", _msg_dec[i]);
    printf("\n");
#endif
}

void FEC_CONV(_setlength)(fec _q, unsigned int _dec_msg_len)
{
    // re-allocate resources as necessary
    unsigned int num_framebits = 8*_dec_msg_len;

    // return if length has not changed
    if (num_framebits == _q->num_framebits)
        return;

#if VERBOSE_FEC_CONV
    printf("creating viterbi decoder, %u frame bits\n", num_framebits);
#endif

    // reset number of framebits
    _q->num_framebits = num_framebits;

    // delete old decoder if necessary
    if (_q->vp != NULL)
        delete_viterbi(_q->vp);

    // re-create / re-allocate memory buffers
    _q->vp = create_viterbi(_q->num_framebits);
    _q->enc_bits = (unsigned char*) realloc(_q->enc_bits, (16*_dec_msg_len+16)*sizeof(unsigned char));
    _q->dec_bits = (unsigned char*) realloc(_q->dec_bits, (_dec_msg_len)*sizeof(unsigned char));
}



#else   // HAVE_FEC_H (config.h)

fec FEC_CONV(_create)(void * _opts)
{
    return NULL;
}

void FEC_CONV(_destroy)(fec _q)
{
}

void FEC_CONV(_encode)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char *_msg_dec,
                       unsigned char *_msg_enc)
{
}

//unsigned int
void FEC_CONV(_decode)(fec _q,
                       unsigned int _dec_msg_len,
                       unsigned char *_msg_enc,
                       unsigned char *_msg_dec)
{
}

#endif  // HAVE_FEC_H (config.h)

