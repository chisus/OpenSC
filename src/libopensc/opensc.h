/* Copyright (C) 2001  Juha Yrj�l� <juha.yrjola@iki.fi>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */

#ifndef _SC_H
#define _SC_H

#include <winscard.h>

#define SC_ERROR_MIN				-1000
#define SC_ERROR_UNKNOWN			-1000
#define SC_ERROR_CMD_TOO_SHORT			-1001
#define SC_ERROR_CMD_TOO_LONG			-1002
#define SC_ERROR_NOT_SUPPORTED			-1003
#define SC_ERROR_TRANSMIT_FAILED		-1004
#define SC_ERROR_FILE_NOT_FOUND			-1005
#define SC_ERROR_INVALID_ARGUMENTS		-1006
#define SC_ERROR_PKCS15_CARD_NOT_FOUND		-1007
#define SC_ERROR_REQUIRED_PARAMETER_NOT_FOUND	-1008
#define SC_ERROR_OUT_OF_MEMORY			-1009
#define SC_ERROR_NO_READERS_FOUND		-1010
#define SC_ERROR_OBJECT_NOT_VALID		-1011
#define SC_ERROR_UNKNOWN_RESPONSE		-1012
#define SC_ERROR_PIN_CODE_INCORRECT		-1013
#define SC_ERROR_SECURITY_STATUS_NOT_SATISFIED	-1014
#define SC_ERROR_CONNECTING_TO_RES_MGR		-1015
#define SC_ERROR_INVALID_ASN1_OBJECT		-1016
#define SC_ERROR_BUFFER_TOO_SMALL		-1017

#define SC_APDU_CASE_NONE		0
#define SC_APDU_CASE_1                  1
#define SC_APDU_CASE_2_SHORT            2
#define SC_APDU_CASE_3_SHORT            3
#define SC_APDU_CASE_4_SHORT            4
#define SC_APDU_CASE_2_EXT              5
#define SC_APDU_CASE_3_EXT              6
#define SC_APDU_CASE_4_EXT              7

#define SC_ISO7816_4_SELECT_FILE	0xA4
#define SC_ISO7816_4_GET_RESPONSE	0xC0
#define SC_ISO7616_4_READ_BINARY	0xB0
#define SC_ISO7616_4_VERIFY		0x20
#define SC_ISO7616_4_UPDATE_BINARY	0xD6
#define SC_ISO7616_4_ERASE_BINARY	0x0E

#define SC_SELECT_FILE_RECORD_FIRST	0x00
#define SC_SELECT_FILE_RECORD_LAST	0x01
#define SC_SELECT_FILE_RECORD_NEXT	0x02
#define SC_SELECT_FILE_RECORD_PREVIOUS	0x03

#define SC_SELECT_FILE_BY_FILE_ID	0x00
#define SC_SELECT_FILE_BY_DF_NAME	0x01
#define SC_SELECT_FILE_BY_PATH		0x02

#define SC_FILE_MAGIC			0x10203040

#define SC_FILE_TYPE_DF			0x07
#define SC_FILE_TYPE_INTERNAL_EF	0x01
#define SC_FILE_TYPE_WORKING_EF		0x00

#define SC_MAX_READERS			4
#define SC_MAX_PATH_SIZE		16
#define SC_MAX_PIN_SIZE			16

#define SC_ASN1_MAX_OBJECT_ID_OCTETS  16

typedef unsigned char u8;

struct sc_object_id {
	int value[SC_ASN1_MAX_OBJECT_ID_OCTETS];
};

struct sc_path {
	u8 value[SC_MAX_PATH_SIZE];
	int len;
};

struct sc_file {
	struct sc_path path;
	u8 name[16];
	int namelen;

	int type;
	int size, id;
	unsigned int magic;
};

struct sc_card {
	int class;
	struct sc_context *context;
	SCARDHANDLE pcsc_card;
};

struct sc_context {
	SCARDCONTEXT pcsc_ctx;
	char *readers[SC_MAX_READERS];
	int reader_count;
};

struct sc_apdu {
	int cse;		/* APDU case */
	u8 cla, ins, p1, p2;
	int lc, le;
	const u8 *data;		/* C-APDU */
	int datalen;		/* length of C-APDU */
	u8 *resp;		/* R-APDU */
	int resplen;		/* length of R-APDU */
};

struct sc_security_env {
	int algorithm_ref;
	struct sc_path app_df_path;
	struct sc_path key_file_id;
	/* signature=1 ==> digital signing, signature=0 ==> authentication */
	int signature;
	int key_ref;
};

/* ASN.1 parsing functions */
const u8 *sc_asn1_find_tag(const u8 * buf, int buflen, int tag, int *taglen);
const u8 *sc_asn1_verify_tag(const u8 * buf, int buflen, int tag, int *taglen);
const u8 *sc_asn1_skip_tag(const u8 ** buf, int *buflen, int tag, int *taglen);

/* ASN.1 printing functions */
void sc_asn1_print_tags(const u8 * buf, int buflen);

/* ASN.1 object decoding functions */
int sc_asn1_utf8string_to_ascii(const u8 * buf,
				int buflen, u8 * outbuf, int outlen);
int sc_asn1_decode_bit_string(const u8 * inbuf,
			      int inlen, void *outbuf, int outlen);
/* non-inverting version */
int sc_asn1_decode_bit_string_ni(const u8 * inbuf,
			      int inlen, void *outbuf, int outlen);
int sc_asn1_decode_integer(const u8 * inbuf, int inlen, int *out);
int sc_asn1_decode_object_id(const u8 * inbuf, int inlen,
			     struct sc_object_id *id);

/* Base64 converter functions */
int sc_base64_convert_to(const u8 *in, int inlen, u8 *out, int outlen,
			 int linelength);

/* internal functions */
int sc_check_apdu(const struct sc_apdu *apdu);
int sc_transmit_apdu(struct sc_card *card, struct sc_apdu *apdu);
int sc_format_apdu(struct sc_card *card,
		   struct sc_apdu *apdu, u8 cse, u8 ins, u8 p1, u8 p2);

int sc_establish_context(struct sc_context **ctx);
int sc_destroy_context(struct sc_context *ctx);
int sc_connect_card(struct sc_context *ctx,
		    int reader, struct sc_card **card);
int sc_disconnect_card(struct sc_card *card);
int sc_detect_card(struct sc_context *ctx, int reader);
/* timeout of -1 means forever, reader of -1 means all readers */
int sc_wait_for_card(struct sc_context *ctx, int reader, int timeout);

int sc_lock(struct sc_card *card);
int sc_unlock(struct sc_card *card);

int sc_select_file(struct sc_card *card,
		   struct sc_file *file,
		   const struct sc_path *path, int pathtype);
int sc_read_binary(struct sc_card *card, int idx, u8 * buf, int count);

int sc_restore_security_env(struct sc_card *card, int se_num);
int sc_set_security_env(struct sc_card *card,
			const struct sc_security_env *env);
int sc_decipher(struct sc_card *card,
		const u8 * crgram, int crgram_len, u8 * out, int outlen);
int sc_compute_signature(struct sc_card *card,
			 const u8 * data,
			 int data_len, u8 * out, int outlen);
int sc_get_random(struct sc_card *card, u8 * rndout, int len);

const char *sc_strerror(int error);

int sc_debug;

#endif
