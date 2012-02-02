/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include "load.h"
#include "synth.h"

/* Based on the HSC File Format Spec, by Simon Peter <dn.tlp@gmx.net>
 * 
 * "Although the format is most commonly known through the HSC-Tracker by
 *  Electronic Rats, it was originally developed by Hannes Seifert of NEO
 *  Software for use in their commercial game productions in the time of
 *  1991 - 1994. ECR just ripped his player and coded an editor around it."
 */


static int hsc_test (FILE *, char *, const int);
static int hsc_load (struct xmp_context *, FILE *, const int);

struct xmp_loader_info hsc_loader = {
    "HSC",
    "HSC-Tracker",
    hsc_test,
    hsc_load
};

static int hsc_test(FILE *f, char *t, const int start)
{
    int p, i, r, c;
    uint8 buf[1200];

    fseek(f, 128 * 12, SEEK_CUR);

    if (fread(buf, 1, 51, f) != 51)
	return -1;

    for (p = i = 0; i < 51; i++) {
	if (buf[i] == 0xff)
	    break;
	if (buf[i] > p)
	    p = buf[i];
    }
    if (!i || !p || i > 50 || p > 50)		/* Test number of patterns */
	return -1;		

    for (i = 0; i < p; i++) {
	fread(buf, 1, 64 * 9 * 2, f);
	for (r = 0; r < 64; r++) {
	    for (c = 0; c < 9; c++) {
		uint8 n = buf[r * 9 * 2 + c * 2];
		uint8 m = buf[r * 9 * 2 + c * 2 + 1];
		if (m > 0x06 && m < 0x10 && n != 0x80)	/* Test effects 07..0f */
		    return -1;
		if (MSN(m) > 6 && MSN(m) < 10)	/* Test effects 7x .. 9x */
		    return -1;
	    }
	}
    }

    read_title(f, t, 0);

    return 0;
}

static int hsc_load(struct xmp_context *ctx, FILE *f, const int start)
{
    struct xmp_mod_context *m = &ctx->m;
    int pat, i, r, c;
    struct xmp_event *event;
    uint8 *x, *sid, e[2], buf[128 * 12];

    LOAD_INIT();

    fread(buf, 1, 128 * 12, f);

    x = buf;
    for (i = 0; i < 128; i++, x += 12) {
	if (x[9] & ~0x3 || x[10] & ~0x3)	/* Test waveform register */
	    break;
	if (x[8] & ~0xf)			/* Test feedback & algorithm */
	    break;
    }

    m->mod.ins = i;

    fseek(f, start + 0, SEEK_SET);

    m->mod.chn = 9;
    m->mod.bpm = 135;
    m->mod.tpo = 6;
    m->mod.smp = 0;
    m->mod.flg = XXM_FLG_LINEAR;

    set_type(m, "HSC (HSC-Tracker)");

    MODULE_INFO();

    /* Read instruments */
    INSTRUMENT_INIT();

    fread (buf, 1, 128 * 12, f);
    sid = buf;
    for (i = 0; i < m->mod.ins; i++, sid += 12) {
	xmp_cvt_hsc2sbi((char *)sid);

	m->mod.xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), 1);
	m->mod.xxi[i].nsm = 1;
	m->mod.xxi[i].sub[0].vol = 0x40;
	m->mod.xxi[i].sub[0].fin = (int8)sid[11] / 4;
	m->mod.xxi[i].sub[0].pan = 0x80;
	m->mod.xxi[i].sub[0].xpo = 0;
	m->mod.xxi[i].sub[0].sid = i;
	m->mod.xxi[i].rls = LSN(sid[7]) * 32;	/* carrier release */

	load_patch(ctx, f, i, XMP_SMP_ADLIB, NULL, (char *)sid);
    }

    /* Read orders */
    for (pat = i = 0; i < 51; i++) {
	fread (&m->mod.xxo[i], 1, 1, f);
	if (m->mod.xxo[i] & 0x80)
	    break;			/* FIXME: jump line */
	if (m->mod.xxo[i] > pat)
	    pat = m->mod.xxo[i];
    }
    fseek(f, 50 - i, SEEK_CUR);
    m->mod.len = i;
    m->mod.pat = pat + 1;
    m->mod.trk = m->mod.pat * m->mod.chn;

    _D(_D_INFO "Module length: %d", m->mod.len);
    _D(_D_INFO "Instruments: %d", m->mod.ins);
    _D(_D_INFO "Stored patterns: %d", m->mod.pat);

    PATTERN_INIT();

    /* Read and convert patterns */
    for (i = 0; i < m->mod.pat; i++) {
	int ins[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	PATTERN_ALLOC (i);
	m->mod.xxp[i]->rows = 64;
	TRACK_ALLOC (i);
        for (r = 0; r < m->mod.xxp[i]->rows; r++) {
            for (c = 0; c < 9; c++) {
	        fread (e, 1, 2, f);
	        event = &EVENT (i, c, r);
		if (e[0] & 0x80) {
		    ins[c] = e[1] + 1;
		} else if (e[0] == 0x7f) {
		    event->note = XMP_KEY_OFF;
		} else if (e[0] > 0) {
		    event->note = e[0] + 13;
		    event->ins = ins[c];
		}

		event->fxt = 0;
		event->fxp = 0;

		if (e[1] == 0x01) {
		    event->fxt = 0x0d;
		    event->fxp = 0;
		}
	    }
	}
    }

    for (i = 0; i < m->mod.chn; i++) {
	m->mod.xxc[i].pan = 0x80;
	m->mod.xxc[i].flg = XXM_CHANNEL_SYNTH;
    }

    m->synth = &synth_adlib;

    return 0;
}
