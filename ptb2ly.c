/*
	(c) 2004-2006: Jelmer Vernooij <jelmer@samba.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* Logic:
 * Write one section at a time:
 *  - One identifier for chords
 *  - One identifier per staff
 * Will sort by offset when multiple things are involved
 */

#include <stdio.h>
#include <errno.h>
#include <popt.h>
#include <string.h>
#include "dlinklist.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "ptb.h"

#define LILYPOND_VERSION "2.4.0"

int warn_unsupported = 0;

const char *num_to_string(int num, char *data)
{
	int i;
	strcpy(data, "");
	
	i = num;
	do 
	{
		data[strlen(data)+1] = '\0';
		data[strlen(data)] = 'A' + (i % 26);
		i /= 26;
	} while (i > 0);


	return data;
}

const char *get_staff_name(int sec_num, int staff_num)
{
	static char name[30], num[2][30];
	sprintf(name, "staff%sx%s", num_to_string(sec_num, num[0]), num_to_string(staff_num, num[1]));
	return name;
}

char *note_names[12] = {
	 "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis", "a", "ais", "b"
};

char *ly_escape(char *data)
{
	static char tmp[2000];
	int i, j = 0;
	for(i = 0; data[i]; i++) {
		switch(data[i]) {
		case '&':
			tmp[j] = '\\'; j++;
		default:
			tmp[j] = data[i];j++;
		}
	}
	tmp[j] = '\0';
	return tmp;
}

void ly_write_header(FILE *out, struct ptbf *ret) 
{
	fprintf(out, "\\header {\n");
	if(ret->hdr.classification == CLASSIFICATION_SONG) {
		if(ret->hdr.class_info.song.title) 	fprintf(out, "  title = \"%s\"\n", ly_escape(ret->hdr.class_info.song.title));
		if(ret->hdr.class_info.song.music_by) fprintf(out, "  composer = \"%s\"\n", ly_escape(ret->hdr.class_info.song.music_by));
		if(ret->hdr.class_info.song.words_by) fprintf(out, "  poet = \"%s\"\n", ly_escape(ret->hdr.class_info.song.words_by));
		if(ret->hdr.class_info.song.copyright) fprintf(out, "  copyright = \"%s\"\n", ly_escape(ret->hdr.class_info.song.copyright));
		if(ret->hdr.class_info.song.guitar_transcribed_by) fprintf(out, "  arranger = \"%s\"\n", ly_escape(ret->hdr.class_info.song.guitar_transcribed_by));
		if(ret->hdr.class_info.song.artist) fprintf(out, "  subtitle = \"As recorded by %s\"\n", ly_escape(ret->hdr.class_info.song.artist));
		if(ret->hdr.class_info.song.release_type == RELEASE_TYPE_PR_AUDIO &&
		   ret->hdr.class_info.song.release_info.pr_audio.album_title) fprintf(out, "  subsubtitle = \"From the %d album %s\"\n", ret->hdr.class_info.song.release_info.pr_audio.year, ly_escape(ret->hdr.class_info.song.release_info.pr_audio.album_title));
	} else if(ret->hdr.classification == CLASSIFICATION_LESSON) {
		if(ret->hdr.class_info.lesson.title) 	fprintf(out, "  title = \"%s\"\n", ly_escape(ret->hdr.class_info.lesson.title));
		if(ret->hdr.class_info.lesson.artist) fprintf(out, "  composer = \"%s\"\n", ly_escape(ret->hdr.class_info.lesson.artist));
		if(ret->hdr.class_info.lesson.author) fprintf(out, "  arranger = \"%s\"\n", ly_escape(ret->hdr.class_info.lesson.author));
		if(ret->hdr.class_info.lesson.copyright) fprintf(out, "  copyright = \"%s\"\n", ly_escape(ret->hdr.class_info.lesson.copyright));
	}
	fprintf(out, "  tagline = \"Engraved by lilypond, generated by ptb2ly\"\n");
	fprintf(out, "}\n");
}

void ly_write_chordname_full(FILE *out, uint8_t base, uint8_t properties, uint8_t additions, int len)
{
	fprintf(out, "%s", ptb_get_tone_full(base));
	if(len) {
		int i, newl = 0, dots = 0;
		for(i = 1; i < 64; i*=2) {
			if(len >= i && len < i*2) {
				newl = i;
				dots = 0;
				if(newl * 1.5 == len) dots = 1;
				if(newl * 1.75 == len) dots = 2;
				break;
			}
		}
		fprintf(out, "%d", newl);
		for(i = 0; i < dots; i++)fprintf(out, ".");
	}

	if(properties & CHORDTEXT_PROPERTY_FORMULA_MAJ7 ||
	   properties & CHORDTEXT_PROPERTY_FORMULA_M ||
	   additions & CHORDTEXT_ADD_9) {
		fprintf(out, ":");
		if(additions & CHORDTEXT_ADD_9)
			fprintf(out, "9");

		if(properties & CHORDTEXT_PROPERTY_FORMULA_MAJ7)fprintf(out, "maj7");
		if(properties & CHORDTEXT_PROPERTY_FORMULA_M)fprintf(out, "m");
	}
}

void ly_write_chordtext_helper(FILE *out, struct ptb_chordtext *name, int length)
{
	if(name->properties & CHORDTEXT_PROPERTY_NOCHORD) {
		//FIXME:fprintf(out, "N.C.");
	} 

	/*
	if(name->properties & CHORDTEXT_PROPERTY_PARENTHESES) {
		fprintf(out, "(");
	}*/

	if(!(name->properties & CHORDTEXT_PROPERTY_NOCHORD) || 
	   (name->properties & CHORDTEXT_PROPERTY_PARENTHESES)) { 
		ly_write_chordname_full(out, name->name[1], name->properties, name->additions, length);

		/* Different bass note */
		if(name->name[0] != name->name[1] && name->name[0] >= 16 && name->name[0] <= 28) {
			fprintf(out, "/+");
			ly_write_chordname_full(out, name->name[0], 0, 0, 0);
		}

	} /*
	if(name->properties & CHORDTEXT_PROPERTY_PARENTHESES) {
		fprintf(out, ")");
	} */

	
	fprintf(out, " ");

}

void ly_write_chordtext(FILE *out, struct ptb_section *section, struct ptb_chordtext *name, struct ptb_chordtext *next)
{
	/* Length:
	 *  - Figure out the offset of the next chord, if any
	 *  - Get the length of the notes between the next chord and this chord
	 *  - We've got our length!
	 */

	int bars, length, i;
	ptb_get_position_difference(section, name->offset, next?next->offset:0xffff, &bars, &length);
	for(i = 0; i < bars; i++) 
		ly_write_chordtext_helper(out, name, 1);
	if(length)
		ly_write_chordtext_helper(out, name, length);
}

static double previous = 0.0;

void ly_write_position(FILE *out, struct ptb_guitar *gtr, struct ptb_position *pos)
{
	double this = 0.0;
	char print_length = 0;
	int tied = 0;
	struct ptb_linedata *d;

	this = pos->length * (pos->dots & POSITION_DOTS_1?1.5:1.0) 
		* (pos->dots & POSITION_DOTS_2?1.5*1.5:1.0);
	
	if(this != previous) {
		print_length = 1;
		previous = this;
	}

	fprintf(out, " ");

	/* Triplet */
	if (pos->fermenta & POSITION_FERMENTA_TRIPLET_1) {
		fprintf(out, "\\times 2/3 { ");
	}

	/* Rest */
	if(!pos->linedatas) {
		fprintf(out, " r");
	}

 	for (d = pos->linedatas; d; d = d->next ) {
		if (d->properties & LINEDATA_PROPERTY_TIE) tied = 1;
	}

//	if (tied) 
//			fprintf(out, "~ ");

	/* Multiple notes */
	if((pos->linedatas && pos->linedatas->next) || print_length) fprintf(out, " <");

	for (d = pos->linedatas; d; d = d->next) {
		int i;
		int octave;

		octave = ptb_get_octave(gtr, d->detailed.string, d->detailed.fret);
		
		fprintf(out, "%s", note_names[ptb_get_step(gtr, d->detailed.string, d->detailed.fret)]);

		for(i = octave; i < 4; i++) fprintf(out, ",");
		for(i = 4; i < octave; i++) fprintf(out, "'");

		if(pos->palm_mute & POSITION_STACCATO) 
			fprintf(out, "-.");

		if(pos->palm_mute & POSITION_ACCENT)
			fprintf(out, "->");

		if(warn_unsupported && pos->palm_mute & POSITION_PALM_MUTE) {
			fprintf(stderr, "Warning: Ignoring Palm Mute\n");
		}

		fprintf(out, "\\%d", d->detailed.string+1);
		if(d->next) fprintf(out, " ");
	}

	/* Multiple notes */
	if((pos->linedatas && pos->linedatas->next) || print_length) 
		fprintf(out, ">");

	/* String */
	if(print_length) {
		fprintf(out, "%d", pos->length);
		if(pos->dots & POSITION_DOTS_1) fprintf(out, ".");
		if(pos->dots & POSITION_DOTS_2) fprintf(out, "..");
	}

	/*
	if(pos->properties & POSITION_PROPERTY_FIRST_IN_BEAM)
		fprintf(out, "[");


	if(pos->properties & POSITION_PROPERTY_LAST_IN_BEAM)
		fprintf(out, "]");*/

	if (pos->fermenta & POSITION_FERMENTA_TRIPLET_3) {
		fprintf(out, "} ");
	}
}

void ly_write_staff_identifier(FILE *out, struct ptb_staff *s, struct ptb_section *section, int section_num, int staff_num, struct ptb_guitar *gtr) 
{
	int o;
	int i;

	fprintf(out, "\n%% Notes for section %d, staff %d\n", section_num, staff_num);
	fprintf(out, "%s = {\n", get_staff_name(section_num, staff_num));
	fprintf(out, "\t");
	previous = 0.0;
	for(o = 0; o < 0x100; o++) {
		for (i = 0; i < 2; i++) {
			struct ptb_position *p = s->positions[i];
			while(p) {
				if (p->offset == o) ly_write_position(out, gtr, p);
				p = p->next;
			}
		}
	}
	fprintf(out, "\n");
	fprintf(out, "}\n");
}

void ly_write_chords_identifier(FILE *out, struct ptb_section *s, int section_num)
{
	int bars, length, i;
	char num[20];

	struct ptb_chordtext *ct = s->chordtexts;

	fprintf(out, "\n%% Chords for section %d\n", section_num);
	fprintf(out, "chords%s = \\chords {", num_to_string(section_num, num));
	if (ct) {
		ptb_get_position_difference(s, 0, ct->offset, &bars, &length);
		for(i = 0; i < bars; i++) fprintf(out, "r1 ");
		if(length) fprintf(out, "r%d ", length);

		while(ct) {
			ly_write_chordtext(out, s, ct, ct->next?ct->next:NULL);
			ct = ct->next;
		}
	}

	fprintf(out, "}\n");
}



void ly_write_section_identifier(FILE *out, struct ptb_section *s, int section_num, struct ptb_guitar *gtr) 
{
	int staff_num = 0;
	struct ptb_staff *st = s->staffs;



	if (s->description) {
		fprintf(out, "\n%% %c: %s\n", s->letter, s->description);
	}

	if (s->end_mark != 0) 
	{
		fprintf(out, "%% endmark: \\bar \"");
		if (s->end_mark & END_MARK_TYPE_DOUBLELINE) {
			fprintf(out, "|");
		}

		if (s->end_mark & END_MARK_TYPE_REPEAT) {
			fprintf(out, ":");
		}

		fprintf(out, "\" %d times\n", s->end_mark
				&~ END_MARK_TYPE_DOUBLELINE 
				&~ END_MARK_TYPE_REPEAT);
	}

	if (s->meter_type & METER_TYPE_COMMON)
	{
		fprintf(out, "%% \\time 4/4\n");
	} 

	if (s->meter_type & METER_TYPE_CUT)
	{
		fprintf(out, "%% \\time 2/2\n");
	}

	if (warn_unsupported && (s->meter_type & METER_TYPE_BEAM_2)) 
	{
		fprintf(stderr, "Warning: METER_TYPE_BEAM_2 ignored\n");
	}

	if (warn_unsupported && (s->meter_type & METER_TYPE_BEAM_3)) 
	{
		fprintf(stderr, "Warning: METER_TYPE_BEAM_3 ignored\n");
	}

	if (warn_unsupported && (s->meter_type & METER_TYPE_BEAM_4)) 
	{
		fprintf(stderr, "Warning: METER_TYPE_BEAM_4 ignored\n");
	}

	if (warn_unsupported && (s->meter_type & METER_TYPE_BEAM_5)) 
	{
		fprintf(stderr, "Warning: METER_TYPE_BEAM_5 ignored\n");
	}

	if (warn_unsupported && (s->meter_type & METER_TYPE_BEAM_6)) 
	{
		fprintf(stderr, "Warning: METER_TYPE_BEAM_6 ignored\n");
	}

	if (warn_unsupported && s->rhythmslashes) {
		fprintf(stderr, "Warning: Ignoring rhythmslashes information\n");
	}

	if (warn_unsupported && s->directions) {
		fprintf(stderr, "Warning: Ignoring directions information\n");
	}

	if (warn_unsupported) {
		fprintf(stderr, "Warning: Ignoring properties\n");
	}

	ly_write_chords_identifier(out, s, section_num);

	while(st) {
		ly_write_staff_identifier(out, st, s, section_num, staff_num, gtr);
		st = st->next;
		staff_num++;
	}

	if (warn_unsupported && s->musicbars) {
		fprintf(stderr, "Warning: Ignoring musicbars\n");
	}
}

void ly_write_tabstaff(FILE *out, struct ptb_staff *s, struct ptb_section *section, int section_num, int staff_num) 
{
	fprintf(out, "\t\t\t\t\\%s\n", get_staff_name(section_num, staff_num));
}

void ly_write_staff(FILE *out, struct ptb_staff *s, struct ptb_section *section, int section_num, int staff_num) 
{
	if(s->properties & STAFF_TYPE_BASS_KEY)
		fprintf(out, "\t\t\t\t\\clef F\n");
	else
		fprintf(out, "\t\t\t\t\\clef \"G_8\"\n");

	fprintf(out, "\t\t\t\t\\%s\n", get_staff_name(section_num, staff_num));
}

void ly_write_chords(FILE *out, struct ptb_section *s, int section_num)
{
	char num[20];
	fprintf(out, "\t\t\\chords%s\n", num_to_string(section_num, num));
}


int ly_write_lyrics(FILE *out, struct ptbf *ret)
{
	if(ret->hdr.classification != CLASSIFICATION_SONG || !ret->hdr.class_info.song.lyrics) return 0;
	fprintf(out, "text = \\lyrics {\n");
	fprintf(out, "\t%s\n", ret->hdr.class_info.song.lyrics);
	fprintf(out, "%s\n", "}\n");
	return 1;
}

int ly_write_tempomarker(FILE *out, struct ptb_tempomarker *ret)
{
	fprintf(out, "%% Tempomarker: %s\n", ret->description);
	fprintf(out, "\\tempo 4 = %d\n", ret->bpm);
	return 1;
}

int ly_write_chorddiagram(FILE *out, struct ptb_chorddiagram *ret)
{
	int i;
	fprintf(out, "%% \\markup \\fret-diagram #\"");

	/* FIXME: Chord name 
	 * ptb_chord name[2];
	 * */

	/* FIXME: Fret offset
		uint8_t frets;
	 */

	/* FIXME: Type
	uint8_t type;
	 */

	for (i = 0; i < ret->nr_strings; i++) {
		fprintf(out, "%d-", i+1);
		if (ret->tones[i] == 0xFE) {
			fprintf(out, "x");
		} else if (ret->tones[i] == 0) {
			fprintf(out, "o");
		} else {
			fprintf(out, "%d", ret->tones[i]);
		} 
		fprintf(out, ";");
	}

	fprintf(out, "\"\n");
	return 1;
}

int ly_write_chorddiagrams_identifiers(FILE *out, struct ptb_instrument *instrument)
{
	struct ptb_chorddiagram *cd = instrument->chorddiagrams;

	while(cd)
	{
		ly_write_chorddiagram(out, cd);
		cd = cd->next;
	}

	return 1;
}


int ly_write_book_section(FILE *out, struct ptb_section *s, int section_num)
{
	int staff_num = 0;
	struct ptb_staff *st = s->staffs;

	if (s->description) {
		fprintf(out, "\t\\header { \n");
		fprintf(out, "\t\tpiece = \"%c: %s\"\n", s->letter, s->description);
		fprintf(out, "\t}\n");
	}
	fprintf(out, "\\score { << \n");
	fprintf(out, "\t\\context ChordNames {\n");
	ly_write_chords(out, s, section_num);
	fprintf(out, "\t}\n");

	while(st) {
		fprintf(out, "\t\\context StaffGroup = \"Staff%d\" <<\n", staff_num);
		fprintf(out, "\t\t\\context Staff { \n");
		ly_write_staff(out, st, s, section_num, staff_num);
		fprintf(out, "\t\t}\n");
		fprintf(out, "\t\\context TabStaff { \n");
		ly_write_tabstaff(out, st, s, section_num, staff_num);
		fprintf(out, "\t\t}\n");
		fprintf(out, "\t>>\n");
		st = st->next;
		staff_num++;
	}

	fprintf(out, "\t>>\n");
	fprintf(out, "} \n");
	return 1;
}

int ly_write_main_book(FILE *out, struct ptb_instrument *instrument)
{
	struct ptb_section *s = instrument->sections;
	int i = 0;
	fprintf(out, "\\book {\n");

	while(s) {
		ly_write_book_section(out, s, i);
		s = s->next;
		i++;
	}

	fprintf(out, "\t\\paper { } \n");
	fprintf(out, "}\n");
	return 1;
}

int ly_write_main_single(FILE *out, struct ptb_instrument *instrument)
{
	fprintf(out, "%%FIXME\n");
	return 0;
}	

int main(int argc, const char **argv) 
{
	FILE *out;
	int have_lyrics;
	struct ptbf *ret;
	int debugging = 0;
	int instrument = 0;
	int c, i = 0;
	int version = 0;
	int singlepiece = 0;
	int quiet = 0;
	const char *input;
	struct ptb_section *section;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"regular", 'r', POPT_ARG_NONE, &instrument, 0, "Write tabs for regular guitar" },
		{"warn-unsupported", 'u', POPT_ARG_NONE, &warn_unsupported, 1, "Warn about unsupported PTB elements" },
		{"bass", 'b', POPT_ARG_NONE, &instrument, 1, "Write tabs for bass guitar"},
		{"quiet", 'q', POPT_ARG_NONE, &quiet, 1, "Be quiet (no output to stderr)" },
		{"single", 's', POPT_ARG_NONE, &singlepiece, 1, "Write single piece instead of \\book (experimental)" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2ly Version "PACKAGE_VERSION"\n");
			printf("(C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
			exit(0);
			break;
		}
	}
			
	ptb_set_debug(debugging);
	
	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}
	input = poptGetArg(pc);
	
	if (!quiet) fprintf(stderr, "Parsing %s... \n", input);
					
	ret = ptb_read_file(input);
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	if(!output) {
		int baselength = strlen(input);
		if (!strcmp(input + strlen(input) - 4, ".ptb")) {
			baselength -= 4;
		}
		output = malloc(baselength + 5);
		strncpy(output, input, baselength);
		strcpy(output + baselength, ".ly");
	}

	if (!quiet) fprintf(stderr, "Generating lilypond file in %s...\n", output);

	if (!strcmp(output, "-")) {
		out = stdout; 
	} else {
		out = fopen(output, "w+");
		if(!out) {
			perror("open");
			return -1;
		}
	}
	
	fprintf(out, "%% Generated by ptb2ly (C) 2004-2006 Jelmer Vernooij <jelmer@samba.org>\n");
	fprintf(out, "%% See http://jelmer.vernstok.nl/oss/ptabtools/ for more info\n\n");
	fprintf(out, "\\version \""LILYPOND_VERSION"\"\n");
		
	ly_write_header(out, ret);
	have_lyrics = ly_write_lyrics(out, ret);

	if (warn_unsupported && ret->instrument[instrument].guitars) {
		fprintf(stderr, "Warning: Ignoring guitar information\n");
	} 

	if (warn_unsupported && ret->instrument[instrument].guitarins) {
		fprintf(stderr, "Warning: Ignoring guitar in information\n");
	} 

	if (warn_unsupported && ret->instrument[instrument].tempomarkers) {
		fprintf(stderr, "Warning: Ignoring tempomarkers\n");
	} 

	if (warn_unsupported && ret->instrument[instrument].dynamics) {
		fprintf(stderr, "Warning: Ignoring dynamics\n");
	}

	if (warn_unsupported && ret->instrument[instrument].floatingtexts) {
		fprintf(stderr, "Warning: Ignoring floating texts\n");
	}

	if (warn_unsupported && ret->instrument[instrument].sectionsymbols) {
		fprintf(stderr, "Warning: Ignoring section symbols\n");
	}

	ly_write_chorddiagrams_identifiers(out, &ret->instrument[instrument]);
	
	i = 1;
	section = ret->instrument[instrument].sections;
	while(section) {
		ly_write_section_identifier(out, section, i-1, ret->instrument[instrument].guitars); /* FIXME: We currently assume the tuning for all guitars to be the same as the first one... */
		section = section->next;
		i++;
	}

	/* Do the main typesetting */
	if (!singlepiece) {
		/* typeset using \book */

		ly_write_main_book(out, &ret->instrument[instrument]);
	} else {
	/* OR:
	 * - define 3 staffs and a chordnames occurring simultaneously
	 * - walk through all of the sections /per/ staff, adding R1's where staffs are not used.
	 */
	 	ly_write_main_single(out, &ret->instrument[instrument]);
	}
	
	if(output)fclose(out);

	ptb_free(ret); ret = NULL;
	
	return 0;
}
