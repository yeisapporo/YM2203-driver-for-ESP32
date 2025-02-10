/*

 YM2203C driver for ESP22

Copyright (c) 2025 Kazuteru Yamada(yeisapporo).  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

 #ifndef _HPP_VGM_HPP_
#define _HPP_VGM_HPP_

typedef struct {
	unsigned char vgm_ident[4];
	unsigned int eof_offset;
	unsigned int version;
	unsigned int sn76489_clock;
	unsigned int ym2413_clock;
	unsigned int gd3_offset;
	unsigned int total_samples_num;
	unsigned int loop_offset;
	unsigned int loop_samples_num;
	unsigned int rate;
	unsigned short sn_fb;
	unsigned char snw;
	unsigned char sf;
	unsigned int ym2612_clock;
	unsigned int ym2151_clock;
	unsigned int vgm_data_offset;
	unsigned int sega_pcm_clock;
	unsigned int spcm_interface;
	unsigned int rf5c68_clock;
	unsigned int ym2203_clock;
	unsigned int ym2608_clock;
	unsigned int ym2610_b_clock;
	unsigned int ym3812_clock;
	unsigned int ym3526_clock;
	unsigned int y8950_clock;
	unsigned int ymf262_clock;
	unsigned int ymf278b_clock;
	unsigned int ymf271_clock;
	unsigned int ymz280b_clock;
	unsigned int rf5c164_clock;
	unsigned int pwm_clock;
	unsigned int ay8910_clock;
	unsigned char ayt;
	unsigned char ay_flags[3];
	unsigned char vm;
	unsigned char reserved_7d;
	unsigned char lb;
	unsigned char lm;
	unsigned int bg_dmg_clock;
	unsigned int nes_apu_clock;
	unsigned int multipcm_clock;
	unsigned int upd7759_clock;
	unsigned int okim6258_clock;	
	unsigned char of;
	unsigned char kf;
	unsigned char cf;
	unsigned char reserved_97;
	unsigned int okim6295_clock;
	unsigned int k051649_clock;
	unsigned int k054539_clock;
	unsigned int huc6280_clock;
	unsigned int c140_clock;
	unsigned int k053260_clock;
	unsigned int pokey_clock;
	unsigned int qsound_clock;
	unsigned int scsp_clock;
	unsigned int extra_hdr_ofs;
	unsigned int wonderswan_clock;
	unsigned int vsu_clock;
	unsigned int saa1099_clock;
	unsigned int es5503_clock;
	unsigned int es5506_clock;
	unsigned short es_chns;
	unsigned char cd;
	unsigned char reserved_d7;
	unsigned int x1_010_clock;
	unsigned int c352_clock;
// actual vgm body starts at 0xe0, so delete #if block below.
#if 0
	unsigned int ga20_clock;
	
	unsigned int reserved_e4;
	unsigned int reserved_e8;
	unsigned int reserved_ec;
	unsigned int reserved_f0;
	unsigned int reserved_f4;
	unsigned int reserved_f8;
	unsigned int reserved_fc;
#endif
} VGM_HEADER;

typedef struct {
	int read_ptr;
	int current_write_num;
	int current_read_num;
	bool locked;
#define FBUFF_EMPTY   (1)
#define FBUFF_FULL    (2)
#define FBUFF_READING (4)
#define FBUFF_WRITING (8)
#define FBUFF_SZ (256)
#define FBUFF_NUM (9)
	int state[FBUFF_NUM];
	unsigned char dat[FBUFF_NUM][FBUFF_SZ];
} FILE_BUFF;

#endif /* _HPP_VGM_HPP_ */
