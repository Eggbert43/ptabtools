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
	fprintf(out, "}\n");
}

void ly_write_position(FILE *out, struct ptb_position *pos)
{
	GList *gl = pos->linedatas;
	int l = g_list_length(pos->linedatas);

	if(l == 0) {/* Rest */
		fprintf(out, " r%d ", pos->length);
		/* FIXME */
	}

	/* Multiple notes */
	if(l > 1) fprintf(out, " << ");

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

		j = abs(note / 12);
		if(note < 0) {
			for(i = j; i < 1; i++) fprintf(out, ",");
		} else if(note > 0) {
			for(i = 1; i < j; i++) fprintf(out, "'");
		}

		/* String */
		fprintf(out, "%d\\%d ", pos->length,string+1);
		gl = gl->next;
	}

	/* Multiple notes */
	if(l > 1) fprintf(out, " >> ");
}

void ly_write_staff(FILE *out, struct ptb_staff *s) 
{
	GList *gl;

	
	fprintf(out, "\t\t\\context StaffGroup <<\n");
	fprintf(out, "\t\t\t\\context Staff <<\n"
			"\t\t\t\t\\clef \"G_8\"\n");
	fprintf(out, "\t\t\\notes {\n");
	fprintf(out, "\t\t\t");
	gl = s->positions1;
	while(gl) {
		ly_write_position(out, (struct ptb_position *)gl->data);
		gl = gl->next;
	}
	fprintf(out, "\n");
	fprintf(out, "\t\t}\n");

	fprintf(out, "\t\t\t>>\n");
	fprintf(out, "\t\t\t\\context TabStaff <<\n");
	fprintf(out, "\t\t\\notes {\n");
	fprintf(out, "\t\t\t");
	gl = s->positions1;
	while(gl) {
		ly_write_position(out, (struct ptb_position *)gl->data);
		gl = gl->next;
	}
	fprintf(out, "\n");
	fprintf(out, "\t\t}\n");


	fprintf(out,"\t\t\t>>\n");
	fprintf(out, "\t\t>>\n");
}

void ly_write_section(FILE *out, struct ptb_section *s) 
{
	GList *gl = s->staffs;
	fprintf(out, "\t << \n");
	while(gl) {
		ly_write_staff(out, (struct ptb_staff *)gl->data);
		gl = gl->next;
	}
	fprintf(out, "\t >> \n");
}

int ly_write_lyrics(FILE *out, struct ptbf *ret)
{
	if(ret->hdr.classification != CLASSIFICATION_SONG || !ret->hdr.class_info.song.lyrics) return 0;
	fprintf(out, "text = \\lyrics {\n");
	fprintf(out, "%s\n", ret->hdr.class_info.song.lyrics);
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
	int c;
	char *output = NULL;
	poptContext pc;
	struct poptOption options[] = {
		POPT_AUTOHELP
		{"debug", 'd', POPT_ARG_NONE, &debugging, 0, "Turn on debugging output" },
		{"outputfile", 'o', POPT_ARG_STRING, &output, 0, "Write to specified file", "FILE" },
		{"regular", 'r', POPT_ARG_NONE, &instrument, 0, "Write tabs for regular guitar" },
		{"bass", 'b', POPT_ARG_NONE, &instrument, 1, "Write tabs for bass guitar"},
		POPT_TABLEEND
	};

	pc = poptGetContext(argv[0], argc, argv, options, 0);
	poptSetOtherOptionHelp(pc, "file.ptb");
	while((c = poptGetNextOpt(pc)) >= 0);
			
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
		
	fprintf(out, "\\version \"1.9.8\"\n\n");

	ly_write_header(out, ret);
	have_lyrics = ly_write_lyrics(out, ret);

	fprintf(out, "\\score {\n");
	fprintf(out, "       <<\n");
    if(have_lyrics) {
		fprintf(out, "    \\addlyrics\n"
    	  "		\\context Staff = one {\n"
       	  "		\\property Staff.autoBeaming = ##f\n"
      	  "		}\n"
		  " \\context Lyrics \\text\n");
	}

	gl = ret->instrument[instrument].sections;
	while(gl) {
		ly_write_section(out, (struct ptb_section *)gl->data);
		gl = gl->next;
	}

	fprintf(out, "   >>\n");

	fprintf(out, "\\paper { }\n");
	fprintf(out, "\\midi { }\n");
	fprintf(out, "}\n");

	if(output)fclose(out);
	
	return (ret?0:1);
}
