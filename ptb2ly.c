/*
	(c) 2004: Jelmer Vernooij <jelmer@samba.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <errno.h>
#include <popt.h>
#include "ptb.h"

char *note_names[12] = {
	"a", "ais", "b", "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis"
};

void ly_write_header(FILE *out, struct ptbf *ret) 
{
	fprintf(out, "\\header {\n");
	if(ret->hdr.classification == CLASSIFICATION_SONG) {
		if(ret->hdr.class_info.song.title) 	fprintf(out, "  title = \"%s\"\n", ret->hdr.class_info.song.title);
		if(ret->hdr.class_info.song.music_by) fprintf(out, "  composer = \"%s\"\n", ret->hdr.class_info.song.music_by);
		if(ret->hdr.class_info.song.words_by) fprintf(out, "  poet = \"%s\"\n", ret->hdr.class_info.song.words_by);
		if(ret->hdr.class_info.song.copyright) fprintf(out, "  copyright = \"%s\"\n", ret->hdr.class_info.song.copyright);
		if(ret->hdr.class_info.song.guitar_transcribed_by) fprintf(out, "  arranger = \"%s\"\n", ret->hdr.class_info.song.guitar_transcribed_by);
		if(ret->hdr.class_info.song.release_type == RELEASE_TYPE_PR_AUDIO &&
		   ret->hdr.class_info.song.release_info.pr_audio.album_title) fprintf(out, "  xtitle = \"%s\"\n", ret->hdr.class_info.song.release_info.pr_audio.album_title);
	} else if(ret->hdr.classification == CLASSIFICATION_LESSON) {
		if(ret->hdr.class_info.lesson.title) 	fprintf(out, "  title = \"%s\"\n", ret->hdr.class_info.lesson.title);
		if(ret->hdr.class_info.lesson.artist) fprintf(out, "  composer = \"%s\"\n", ret->hdr.class_info.lesson.artist);
		if(ret->hdr.class_info.lesson.author) fprintf(out, "  arranger = \"%s\"\n", ret->hdr.class_info.lesson.author);
		if(ret->hdr.class_info.lesson.copyright) fprintf(out, "  copyright = \"%s\"\n", ret->hdr.class_info.lesson.copyright);
	}
	fprintf(out, "tagline = \"Engraved by lilypond, generated by ptb2ly\"\n");
	fprintf(out, "}\n");
}

void ly_write_chordtext(FILE *out, struct ptb_chordtext *name)
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
		if(name->name[0] == name->name[1] || name->name[0] < 16 || name->name[0] > 28) 
			fprintf(out, "%s", ptb_get_tone_full(name->name[1]));
		else 
			fprintf(out, "%s/%s",
				ptb_get_tone_full(name->name[1]), 
				ptb_get_tone_full(name->name[0]));
	}
/*
	if(name->properties & CHORDTEXT_PROPERTY_PARENTHESES) {
		fprintf(out, ")");
	} */
	fprintf(out, " ");
}

void ly_write_position(FILE *out, struct ptb_position *pos)
{
	static int previous = 0;
	char print_length = 0;
	GList *gl = pos->linedatas;
	int l = g_list_length(pos->linedatas);

	if(pos->length != previous) {
		print_length = 1;
		previous = pos->length;
	}

	fprintf(out, " ");

	if(l == 0) {/* Rest */
		fprintf(out, " r");
	}

	/* Multiple notes */
	if(l > 1 || print_length) fprintf(out, " <");

	while(gl) {
		struct ptb_linedata *d = gl->data;
		int string = d->tone / 0x20;
		int note = d->tone % 0x20;
		int i;
		int j;
		
		switch(string) {
		case 0: note+= 19; break;
		case 1: note+= 14; break;
		case 2: note+= 10; break;
		case 3: note+= 5; break;
		case 4: note+= 0; break;
		case 5: note+= -7; break;
		}
		
		fprintf(out, "%s", note_names[abs(note)%12]);

		j = abs((note+9) / 12);
		if(note < 0) {
			for(i = j; i < 1; i++) fprintf(out, ",");
		} else if(note > 0) {
			for(i = 1; i < j; i++) fprintf(out, "'");
		}

		fprintf(out, "\\%d", string+1);
		gl = gl->next;
		if(gl) fprintf(out, " ");
	}

	/* Multiple notes */
	if(l > 1 || print_length) fprintf(out, ">");

	/* String */
	if(print_length) {
		fprintf(out, "%d", pos->length);
	}
}

void ly_write_staff(FILE *out, struct ptb_staff *s) 
{
	GList *gl;

	
	fprintf(out, "\t\t\\context StaffGroup <<\n");
	fprintf(out, "\t\t\t\\context Staff <<\n"
			"\t\t\t\t\\clef \"G_8\"\n");
	fprintf(out, "\t\t\t\t\\notes {\n");
	fprintf(out, "\t\t\t\t\t\t");
	gl = s->positions1;
	while(gl) {
		ly_write_position(out, (struct ptb_position *)gl->data);
		gl = gl->next;
	}
	fprintf(out, "\n");
	fprintf(out, "\t\t\t\t\t}\n");

	fprintf(out, "\t\t\t>>\n");
	fprintf(out, "\t\t\t\\context TabStaff <<\n");
	fprintf(out, "\t\t\t\t\\notes {\n");
	fprintf(out, "\t\t\t\t\t");
	gl = s->positions1;
	while(gl) {
		ly_write_position(out, (struct ptb_position *)gl->data);
		gl = gl->next;
	}
	fprintf(out, "\n");
	fprintf(out, "\t\t\t\t}\n");


	fprintf(out,"\t\t\t>>\n");
	fprintf(out, "\t\t>>\n");
}

void ly_write_section(FILE *out, struct ptb_section *s) 
{
	GList *gl = s->staffs;
	fprintf(out, "\t\\score { \\simultaneous { \n");
	while(gl) {
		ly_write_staff(out, (struct ptb_staff *)gl->data);
		gl = gl->next;
	}


	fprintf(out, "\t\t\\context ChordNames \\chords {\n");
	fprintf(out, "\t\t\t");
	gl = s->chordtexts;
	while(gl) {
		ly_write_chordtext(out, (struct ptb_chordtext *)gl->data);
		gl = gl->next;
	}


	fprintf(out, "\n\t\t}\n");
	fprintf(out, "\t}\n");

	fprintf(out, "\t\\paper { }\n");
	fprintf(out, "\t\\midi { }\n");
	fprintf(out, "} \n");
}

int ly_write_lyrics(FILE *out, struct ptbf *ret)
{
	if(ret->hdr.classification != CLASSIFICATION_SONG || !ret->hdr.class_info.song.lyrics) return 0;
	fprintf(out, "text = \\lyrics {\n");
	fprintf(out, "\t%s\n", ret->hdr.class_info.song.lyrics);
	fprintf(out, "%s\n", "}\n");
	return 1;
}

int ly_write_chords(FILE *out, struct ptbf *ret)
{
	/* FIXME:
	 * acc =  \chords {
    % why don't \skip, s4 work?
        c2 c f c
        f c g:7 c
    g f c  g:7 % urg, bug!
        g f c  g:7
    % copy 1-8
        c2 c f c
        f c g:7 c
}
*/

	return 0;
}

int main(int argc, const char **argv) 
{
	FILE *out = stdout;
	int have_lyrics;
	struct ptbf *ret;
	int debugging = 0;
	GList *gl;
	int instrument = 0;
	int c, i = 0;
	int version = 0;
	int num_sections = 0;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"regular", 'r', POPT_ARG_NONE, &instrument, 0, "Write tabs for regular guitar" },
		{"bass", 'b', POPT_ARG_NONE, &instrument, 1, "Write tabs for bass guitar"},
		{"sections", 's', POPT_ARG_INT, &num_sections, 0, "Write only X sections (0 for all)", "SECTIONS" },
		{"version", 'v', POPT_ARG_NONE, &version, 'v', "Show version information" },
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0) {
		switch(c) {
		case 'v':
			printf("ptb2ly Version "PTB_VERSION"\n");
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
	ret = ptb_read_file(poptGetArg(pc));
	
	if(!ret) {
		perror("Read error: ");
		return -1;
	} 

	if(output) {
		out = fopen(output, "w+");
		if(!out) {
			perror("open");
			return -1;
		}
	} 
	
	fprintf(out, "%% Generated by ptb2ly (C) 2004 Jelmer Vernooij <jelmer@samba.org>\n");
	fprintf(out, "%% See http://jelmer.vernstok.nl/oss/ptabtools/ for more info\n\n");
		
	ly_write_header(out, ret);
	have_lyrics = ly_write_lyrics(out, ret);
	
	gl = ret->instrument[instrument].sections;
	while(gl) {
		if(++i > num_sections && num_sections) break;
		ly_write_section(out, (struct ptb_section *)gl->data);
		gl = gl->next;
	}


	if(output)fclose(out);
	
	return (ret?0:1);
}
