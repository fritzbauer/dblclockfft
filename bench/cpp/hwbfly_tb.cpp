////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	hwbfly_tb.cpp
// {{{
// Project:	A General Purpose Pipelined FFT Implementation
//
// Purpose:	A test-bench for the hardware butterfly subfile of the generic
//		pipelined FFT.  This file may be run autonomously.  If so,
//	the last line output will either read "SUCCESS" on success, or some
//	other failure message otherwise.
//
//	This file depends upon verilator to both compile, run, and therefore
//	test hwbfly.v
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2015-2021, Gisselquist Technology, LLC
// {{{
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
// }}}
// License:	GPL, v3, as defined and found on www.gnu.org,
// {{{
//		http://www.gnu.org/licenses/gpl.html
//
////////////////////////////////////////////////////////////////////////////////
// }}}
#include <stdio.h>
#include <stdint.h>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vhwbfly.h"
#include "twoc.h"
#include "fftsize.h"

#ifdef	ROOT_VERILATOR

#include "Vhwbfly___024root.h"

#define	VVAR(A)	rootp->hwbfly__DOT_ ## A

#elif	defined(NEW_VERILATOR)
#define	VVAR(A)	hwbfly__DOT_ ## A
#else
#define	VVAR(A)	v__DOT_ ## A
#endif

#define	IWIDTH	TST_BUTTERFLY_IWIDTH
#define	CWIDTH	TST_BUTTERFLY_CWIDTH
#define	OWIDTH	TST_BUTTERFLY_OWIDTH

class	HWBFLY_TB {
public:
	Vhwbfly		*m_bfly;
	VerilatedVcdC	*m_trace;
	unsigned long	m_left[64], m_right[64];
	bool		m_aux[64];
	int		m_addr, m_lastaux, m_offset;
	bool		m_syncd;
	uint64_t	m_tickcount;

	HWBFLY_TB(void) {
		Verilated::traceEverOn(true);
		m_trace = NULL;
		m_bfly = new Vhwbfly;
		m_addr = 0;
		m_syncd = 0;
		m_tickcount = 0;
		m_bfly->i_reset = 1;
		m_bfly->i_clk = 0;
		m_bfly->eval();
		m_bfly->i_reset = 0;
	}

	void	opentrace(const char *vcdname) {
		if (!m_trace) {
			m_trace = new VerilatedVcdC;
			m_bfly->trace(m_trace, 99);
			m_trace->open(vcdname);
		}
	}

	void	closetrace(void) {
		if (m_trace) {
			m_trace->close();
			delete	m_trace;
			m_trace = NULL;
		}
	}

	void	tick(void) {
		m_tickcount++;

		m_lastaux = m_bfly->o_aux;
		m_bfly->i_clk = 0;
		m_bfly->eval();
		if (m_trace) m_trace->dump((vluint64_t)(10ul*m_tickcount-2));
		m_bfly->i_clk = 1;
		m_bfly->eval();
		if (m_trace) m_trace->dump((vluint64_t)(10ul*m_tickcount));
		m_bfly->i_clk = 0;
		m_bfly->eval();
		if (m_trace) {
			m_trace->dump((vluint64_t)(10ul*m_tickcount+5));
			m_trace->flush();
		}

		if ((!m_syncd)&&(m_bfly->o_aux))
			m_offset = m_addr;
		m_syncd = (m_syncd) || (m_bfly->o_aux);
	}

	void	cetick(void) {
		int	ce = m_bfly->i_ce, nkce;

		tick();

		nkce = (rand()&1);
#ifdef	FFT_CKPCE
		nkce += FFT_CKPCE;
#endif

		if ((ce)&&(nkce > 0)) {
			m_bfly->i_ce = 0;
			for(int kce=0; kce<nkce-1; kce++)
				tick();
		}

		m_bfly->i_ce = ce;
	}

	void	reset(void) {
		m_bfly->i_ce    = 0;
		m_bfly->i_reset = 1;
		m_bfly->i_coef  = 0l;
		m_bfly->i_left  = 0;
		m_bfly->i_right = 0;
		tick();
		m_bfly->i_reset = 0;
		m_bfly->i_ce  = 1;
		//
		// Let's run a RESET test here, forcing the whole butterfly
		// to be filled with aux=1.  If the reset works right,
		// we'll never get an aux=1 output.
		//
		m_bfly->i_reset = 1;
		m_bfly->i_aux = 1;
		m_bfly->i_ce  = 1;
		for(int i=0; i<200; i++)
			cetick();

		// Now here's the RESET line, so let's see what the test does
		m_bfly->i_reset = 1;
		m_bfly->i_ce  = 1;
		m_bfly->i_aux = 1;
		cetick();
		m_bfly->i_reset = 0;
		m_syncd = 0;
	}

	void	test(const int n, const int k, const unsigned long cof,
			const unsigned lft, const unsigned rht, const int aux) {

		m_bfly->i_coef  = ubits(cof, 2*TST_BUTTERFLY_CWIDTH);
		m_bfly->i_left  = ubits(lft, 2*TST_BUTTERFLY_IWIDTH);
		m_bfly->i_right = ubits(rht, 2*TST_BUTTERFLY_IWIDTH);
		m_bfly->i_aux   = aux & 1;

		m_bfly->i_ce = 1;
		cetick();

		if ((m_bfly->o_aux)&&(!m_lastaux))
			printf("\n");
		printf("n,k=%d,%3d: COEF=%010lx, LFT=%08x, RHT=%08x, A=%d, OLFT =%09lx, ORHT=%09lx, AUX=%d",
			n,k,
			m_bfly->i_coef & (~(-1l<<40)),
			m_bfly->i_left,
			m_bfly->i_right,
			m_bfly->i_aux,
			m_bfly->o_left,
			m_bfly->o_right,
			m_bfly->o_aux);
#if (FFT_CKPCE == 1)
		printf(", p1 = 0x%08lx p2 = 0x%08lx, p3 = 0x%08lx",
#define	rp_one		VVAR(_CKPCE_ONE__DOT__rp_one)
#define	rp_two		VVAR(_CKPCE_ONE__DOT__rp_two)
#define	rp_three	VVAR(_CKPCE_ONE__DOT__rp_three)
			m_bfly->rp_one,
			m_bfly->rp_two,
			m_bfly->rp_three);
#elif (FFT_CKPCE == 2)
#define	rp_one		VVAR(_genblk1__DOT__CKPCE_TWO__DOT__rp2_one)
#define	rp_two		VVAR(_genblk1__DOT__CKPCE_TWO__DOT__rp_two)
#define	rp_three	VVAR(_genblk1__DOT__CKPCE_TWO__DOT__rp_three)
		printf(", p1 = 0x%08lx p2 = 0x%08lx, p3 = 0x%08lx",
			m_bfly->rp_one,
			m_bfly->rp_two,
			m_bfly->rp_three);
#else
		printf("CKPCE = %d\n", FFT_CKPCE);
#endif

		printf("\n");

		if ((m_syncd)&&(m_left[(m_addr-m_offset)&(64-1)] != m_bfly->o_left)) {
			printf("WRONG O_LEFT! (%lx(exp) != %lx(sut)\n",
				m_left[(m_addr-m_offset)&(64-1)],
				m_bfly->o_left);
			exit(EXIT_FAILURE);
		}

		if ((m_syncd)&&(m_right[(m_addr-m_offset)&(64-1)] != m_bfly->o_right)) {
			printf("WRONG O_RIGHT! (%lx(exp) != %lx(sut))\n",
				m_right[(m_addr-m_offset)&(64-1)], m_bfly->o_right);
			exit(EXIT_FAILURE);
		}

		if ((m_syncd)&&(m_aux[(m_addr-m_offset)&(64-1)] != m_bfly->o_aux)) {
			printf("FAILED AUX CHANNEL TEST (i.e. the SYNC)\n");
			exit(EXIT_FAILURE);
		}

		if ((m_addr > 22)&&(!m_syncd)) {
			printf("NO SYNC PULSE!\n");
			exit(EXIT_FAILURE);
		}

		// Now, let's calculate an "expected" result ...
		long	rlft, ilft;

		// Extract left and right values ...
		rlft = sbits(m_bfly->i_left >> IWIDTH, IWIDTH);
		ilft = sbits(m_bfly->i_left          , IWIDTH);

		// Now repeat for the right hand value ...
		long	rrht, irht;
		// Extract left and right values ...
		rrht = sbits(m_bfly->i_right >> IWIDTH, IWIDTH);
		irht = sbits(m_bfly->i_right          , IWIDTH);

		// and again for the coefficients
		long	rcof, icof;
		// Extract left and right values ...
		rcof = sbits(m_bfly->i_coef >> CWIDTH, CWIDTH);
		icof = sbits(m_bfly->i_coef          , CWIDTH);

		// Now, let's do the butterfly ourselves ...
		long sumi, sumr, difi, difr;
		sumr = rlft + rrht;
		sumi = ilft + irht;
		difr = rlft - rrht;
		difi = ilft - irht;

	/*
		printf("L=%5lx+%5lx,R=%5lx+%5lx,S=%5lx+%5lx,D=%5lx+%5lx, ",
			rlft & 0x02ffffl,
			ilft & 0x02ffffl,
			rrht & 0x02ffffl,
			irht & 0x02ffffl,
			sumr & 0x02ffffl,
			sumi & 0x02ffffl,
			difr & 0x02ffffl,
			difi & 0x02ffffl);
	*/
		long p1, p2, p3, mpyr, mpyi;
		p1 = difr * rcof;
		p2 = difi * icof;
		p3 = (difr + difi) * (rcof + icof);

		mpyr = p1-p2;
		mpyi = p3-p1-p2;

		mpyr = rndbits(mpyr, (IWIDTH+2)+(CWIDTH+1), OWIDTH+4);
		mpyi = rndbits(mpyi, (IWIDTH+2)+(CWIDTH+1), OWIDTH+4);

	/*
		printf("RC=%lx, IC=%lx, ", rcof, icof);
		printf("P1=%lx,P2=%lx,P3=%lx, ", p1,p2,p3);
		printf("MPYr = %lx, ", mpyr);
		printf("MPYi = %lx, ", mpyi);
	*/

		long	o_left_r, o_left_i, o_right_r, o_right_i;
		unsigned long	o_left, o_right;

		o_left_r = rndbits(sumr<<(CWIDTH-2), CWIDTH+IWIDTH+3, OWIDTH+4);
			o_left_r = ubits(o_left_r, OWIDTH);
		o_left_i = rndbits(sumi<<(CWIDTH-2), CWIDTH+IWIDTH+3, OWIDTH+4);
			o_left_i = ubits(o_left_i, OWIDTH);
		o_left = (o_left_r << OWIDTH) | (o_left_i);

		o_right_r = ubits(mpyr, OWIDTH);
		o_right_i = ubits(mpyi, OWIDTH);
		o_right = (o_right_r << OWIDTH) | (o_right_i);
	/*
		printf("oR_r = %lx, ", o_right_r);
		printf("oR_i = %lx\n", o_right_i);
	*/

		m_left[ m_addr&(64-1)] = o_left;
		m_right[m_addr&(64-1)] = o_right;
		m_aux[  m_addr&(64-1)] = aux;

		m_addr++;
	}
};

int	main(int argc, char **argv, char **envp) {
	Verilated::commandArgs(argc, argv);
	HWBFLY_TB	*bfly = new HWBFLY_TB;
	int16_t		ir0, ii0, lstr, lsti;
	int32_t		sumr, sumi, difr, difi;
	int32_t		smr, smi, dfr, dfi;
	int		rnd = 0;

	const int	TESTSZ = 256;

	// bfly->opentrace("hwbfly.vcd");

	bfly->reset();

	bfly->test(9,0,0x4000000000l,0x7fff0000,0x7fff0000, 1);
	bfly->test(9,1,0x4000000000l,0x7fff0000,0x80010000, 0);
	bfly->test(9,2,0x4000000000l,0x00007fff,0x00008001, 0);
	bfly->test(9,3,0x4000000000l,0x00007fff,0x00007fff, 0);

	bfly->test(8,0,0x4000000000l,0x80010000,0x80010000, 1);
	bfly->test(8,1,0x4000000000l,0x00008001,0x00008001, 0);

	bfly->test(9,0,0x4000000000l,0x40000000,0xc0000000, 1);
	bfly->test(9,1,0x4000000000l,0x40000000,0x40000000, 0);
	bfly->test(9,2,0x4000000000l,0x00004000,0x0000c000, 0);
	bfly->test(9,3,0x4000000000l,0x00004000,0x00004000, 0);

	bfly->test(9,0,0x4000000000l,0x20000000,0xe0000000, 1);
	bfly->test(9,1,0x4000000000l,0x20000000,0x20000000, 0);
	bfly->test(9,2,0x4000000000l,0x00002000,0x0000e000, 0);
	bfly->test(9,3,0x4000000000l,0x00002000,0x00002000, 0);

	bfly->test(9,0,0x4000000000l,0x00080000,0xfff80000, 1);
	bfly->test(9,1,0x4000000000l,0x00080000,0x00080000, 0);
	bfly->test(9,2,0x4000000000l,0x00000008,0x0000fff8, 0);
	bfly->test(9,3,0x4000000000l,0x00000008,0x00000008, 0);

	bfly->test(7,0,0x3fffbff9b9l,0xfffe0000,0x00000000, 1);
	bfly->test(7,1,0x3ffd4fed28l,0xfffc0000,0x00020000, 0);
	bfly->test(7,2,0x3ff85fe098l,0xfff80000,0x00060000, 0);
	bfly->test(7,3,0x3ff0efd409l,0xfff00000,0x000e0000, 0);
	bfly->test(7,4,0x3fe70fc77cl,0xffe60000,0x00180000, 0);
	bfly->test(7,5,0x3fdabfbaf1l,0xffda0000,0x00240000, 0);
	bfly->test(7,6,0x3fcbefae69l,0xffca0000,0x00340000, 0);
	bfly->test(7,7,0x3fbaafa1e4l,0xffba0000,0x00440000, 0);

	/*
	// Special tests
	bfly->test(9,0,0x4000000000l,0x00010000,0xffff0000, 1);
	bfly->test(9,1,0x4000000000l,0x00010000,0x00010000, 0);
	bfly->test(9,2,0x4000000000l,0x00000001,0x0000ffff, 0);
	bfly->test(9,3,0x4000000000l,0x00000001,0x00000001, 0);
	*/

	for(int n=0; n<4; n++) for(int k=0; k<TESTSZ; k++) {
		long	iv, rv;
		unsigned long	lft, rht, cof;
		double	c, s, W;
		bool	inv = 1;
		int	aux;

		W = ((inv)?-1:1) * 2.0 * M_PI * (2*k) / TESTSZ * 64;
		c = cos(W); s = sin(W);
		rv = (long)((double)(1l<<(16-2-n))*c+0.5);
		iv = (long)((double)(1l<<(16-2-n))*s+0.5);

		rv = (rv << 16) | (iv & (~(-1<<16)));
		lft = rv;

		W = ((inv)?-1:1) * 2.0 * M_PI * (2*k+1) / TESTSZ * 64;
		c = cos(W); s = sin(W);
		rv = (long)((double)(1l<<(16-2-n))*c+0.5);
		iv = (long)((double)(1l<<(16-2-n))*s+0.5);

		rv = (rv << 16) | (iv & (~(-1<<16)));
		rht = rv;


		// Switch the sign of W
		W = ((inv)?1:-1) * 2.0 * M_PI * (2*k) / TESTSZ;
		c = cos(W); s = sin(W);
		rv = (long)((double)(1l<<(20-2))*c+0.5); // Keep 20-2 bits for
		iv = (long)((double)(1l<<(20-2))*s+0.5); // coefficients

		rv = (rv << 20) | (iv & (~(-1<<20)));
		cof = rv;

		aux = ((k&(TESTSZ-1))==0);

		bfly->test(n,k, cof, lft, rht, aux);
	}

	delete	bfly;

	printf("SUCCESS!\n");
	exit(0);
}
