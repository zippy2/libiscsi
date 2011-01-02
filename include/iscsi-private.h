/*
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>

#ifndef discard_const
#define discard_const(ptr) ((void *)((intptr_t)(ptr)))
#endif

#define ISCSI_RAW_HEADER_SIZE			48
#define ISCSI_DIGEST_SIZE			 4

#define ISCSI_HEADER_SIZE (ISCSI_RAW_HEADER_SIZE	\
  + (iscsi->header_digest == ISCSI_HEADER_DIGEST_NONE?0:ISCSI_DIGEST_SIZE))


struct iscsi_in_pdu {
	struct iscsi_in_pdu *next;

	long long hdr_pos;
	unsigned char hdr[ISCSI_RAW_HEADER_SIZE + ISCSI_DIGEST_SIZE];

	long long data_pos;
	unsigned char *data;
};
void iscsi_free_iscsi_in_pdu(struct iscsi_in_pdu *in);
void iscsi_free_iscsi_inqueue(struct iscsi_in_pdu *inqueue);

enum iscsi_initial_r2t {
	ISCSI_INITIAL_R2T_NO  = 0,
	ISCSI_INITIAL_R2T_YES = 1
};

enum iscsi_immediate_data {
	ISCSI_IMMEDIATE_DATA_NO  = 0,
	ISCSI_IMMEDIATE_DATA_YES = 1
};

struct iscsi_context {
	const char *initiator_name;
	const char *target_name;
	const char *alias;

	const char *user;
	const char *passwd;

	enum iscsi_session_type session_type;
	unsigned char isid[6];
	uint32_t itt;
	uint32_t cmdsn;
	uint32_t statsn;
	enum iscsi_header_digest want_header_digest;
	enum iscsi_header_digest header_digest;

	char *error_string;

	int fd;
	int is_connected;

	int current_phase;
	int next_phase;
#define ISCSI_LOGIN_SECNEG_PHASE_OFFER_CHAP         0
#define ISCSI_LOGIN_SECNEG_PHASE_SELECT_ALGORITHM   1
#define ISCSI_LOGIN_SECNEG_PHASE_SEND_RESPONSE      2
	int secneg_phase;
	int login_attempts;
	int is_loggedin;

	int chap_a;
	int chap_i;
	char *chap_c;

	iscsi_command_cb socket_status_cb;
	void *connect_data;

	struct iscsi_pdu *outqueue;
	struct iscsi_pdu *waitpdu;

	struct iscsi_in_pdu *incoming;
	struct iscsi_in_pdu *inqueue;

	uint32_t max_burst_length;
	uint32_t first_burst_length;
	uint32_t max_recv_data_segment_length;
	enum iscsi_initial_r2t want_initial_r2t;
	enum iscsi_initial_r2t use_initial_r2t;
	enum iscsi_initial_r2t want_immediate_data;
	enum iscsi_initial_r2t use_immediate_data;
};

#define ISCSI_PDU_IMMEDIATE		       0x40

#define ISCSI_PDU_TEXT_FINAL		       0x80
#define ISCSI_PDU_TEXT_CONTINUE		       0x40

#define ISCSI_PDU_LOGIN_TRANSIT		       0x80
#define ISCSI_PDU_LOGIN_CONTINUE	       0x40
#define ISCSI_PDU_LOGIN_CSG_SECNEG	       0x00
#define ISCSI_PDU_LOGIN_CSG_OPNEG	       0x04
#define ISCSI_PDU_LOGIN_CSG_FF		       0x0c
#define ISCSI_PDU_LOGIN_NSG_SECNEG	       0x00
#define ISCSI_PDU_LOGIN_NSG_OPNEG	       0x01
#define ISCSI_PDU_LOGIN_NSG_FF		       0x03

#define ISCSI_PDU_SCSI_FINAL		       0x80
#define ISCSI_PDU_SCSI_READ		       0x40
#define ISCSI_PDU_SCSI_WRITE		       0x20
#define ISCSI_PDU_SCSI_ATTR_UNTAGGED	       0x00
#define ISCSI_PDU_SCSI_ATTR_SIMPLE	       0x01
#define ISCSI_PDU_SCSI_ATTR_ORDERED	       0x02
#define ISCSI_PDU_SCSI_ATTR_HEADOFQUEUE	       0x03
#define ISCSI_PDU_SCSI_ATTR_ACA		       0x04

#define ISCSI_PDU_DATA_FINAL		       0x80
#define ISCSI_PDU_DATA_ACK_REQUESTED	       0x40
#define ISCSI_PDU_DATA_BIDIR_OVERFLOW  	       0x10
#define ISCSI_PDU_DATA_BIDIR_UNDERFLOW         0x08
#define ISCSI_PDU_DATA_RESIDUAL_OVERFLOW       0x04
#define ISCSI_PDU_DATA_RESIDUAL_UNDERFLOW      0x02
#define ISCSI_PDU_DATA_CONTAINS_STATUS	       0x01

enum iscsi_opcode {
	ISCSI_PDU_NOP_OUT         = 0x00,
	ISCSI_PDU_SCSI_REQUEST    = 0x01,
	ISCSI_PDU_LOGIN_REQUEST   = 0x03,
	ISCSI_PDU_TEXT_REQUEST    = 0x04,
	ISCSI_PDU_LOGOUT_REQUEST  = 0x06,
	ISCSI_PDU_NOP_IN          = 0x20,
	ISCSI_PDU_SCSI_RESPONSE   = 0x21,
	ISCSI_PDU_LOGIN_RESPONSE  = 0x23,
	ISCSI_PDU_TEXT_RESPONSE   = 0x24,
	ISCSI_PDU_DATA_IN         = 0x25,
	ISCSI_PDU_LOGOUT_RESPONSE = 0x26
};

struct iscsi_pdu {
	struct iscsi_pdu *next;

	uint32_t itt;
	uint32_t cmdsn;
	enum iscsi_opcode response_opcode;

	iscsi_command_cb callback;
	void *private_data;

	int written;
	struct iscsi_data outdata;
	struct iscsi_data indata;

	struct iscsi_scsi_cbdata *scsi_cbdata;
};

void iscsi_free_scsi_cbdata(struct iscsi_scsi_cbdata *scsi_cbdata);

struct iscsi_pdu *iscsi_allocate_pdu(struct iscsi_context *iscsi,
				     enum iscsi_opcode opcode,
				     enum iscsi_opcode response_opcode);
void iscsi_free_pdu(struct iscsi_context *iscsi, struct iscsi_pdu *pdu);
void iscsi_pdu_set_pduflags(struct iscsi_pdu *pdu, unsigned char flags);
void iscsi_pdu_set_immediate(struct iscsi_pdu *pdu);
void iscsi_pdu_set_ttt(struct iscsi_pdu *pdu, uint32_t ttt);
void iscsi_pdu_set_cmdsn(struct iscsi_pdu *pdu, uint32_t cmdsn);
void iscsi_pdu_set_lun(struct iscsi_pdu *pdu, uint32_t lun);
void iscsi_pdu_set_expstatsn(struct iscsi_pdu *pdu, uint32_t expstatsnsn);
void iscsi_pdu_set_expxferlen(struct iscsi_pdu *pdu, uint32_t expxferlen);
int iscsi_pdu_add_data(struct iscsi_context *iscsi, struct iscsi_pdu *pdu,
		       unsigned char *dptr, int dsize);
int iscsi_queue_pdu(struct iscsi_context *iscsi, struct iscsi_pdu *pdu);
int iscsi_add_data(struct iscsi_context *iscsi, struct iscsi_data *data,
		   unsigned char *dptr, int dsize, int pdualignment);

struct scsi_task;
void iscsi_pdu_set_cdb(struct iscsi_pdu *pdu, struct scsi_task *task);

int iscsi_get_pdu_data_size(const unsigned char *hdr);
int iscsi_process_pdu(struct iscsi_context *iscsi, struct iscsi_in_pdu *in);

int iscsi_process_login_reply(struct iscsi_context *iscsi,
			      struct iscsi_pdu *pdu,
			      struct iscsi_in_pdu *in);
int iscsi_process_text_reply(struct iscsi_context *iscsi,
			     struct iscsi_pdu *pdu,
			     struct iscsi_in_pdu *in);
int iscsi_process_logout_reply(struct iscsi_context *iscsi,
			       struct iscsi_pdu *pdu,
			       struct iscsi_in_pdu *in);
int iscsi_process_scsi_reply(struct iscsi_context *iscsi,
			     struct iscsi_pdu *pdu,
			     struct iscsi_in_pdu *in);
int iscsi_process_scsi_data_in(struct iscsi_context *iscsi,
			       struct iscsi_pdu *pdu,
			       struct iscsi_in_pdu *in,
			       int *is_finished);
int iscsi_process_nop_out_reply(struct iscsi_context *iscsi,
				struct iscsi_pdu *pdu,
				struct iscsi_in_pdu *in);

void iscsi_set_error(struct iscsi_context *iscsi, const char *error_string,
		     ...);

unsigned long crc32c(char *buf, int len);

void iscsi_cbdata_steal_scsi_task(struct scsi_task *task);
