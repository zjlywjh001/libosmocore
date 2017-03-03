/* IuUP (Iu User Plane) according to 3GPP TS 25.415 */
#include <osmocom/core/crc8gen.h>
#include <osmocom/core/crc16gen.h>
#include <osmocom/gsm/protocol/iu_up.h>

/***********************************************************************
 * CRC Calculation
 ***********************************************************************/

/* Section 6.6.3.8 Header CRC */
const struct osmo_crc8gen_code iuup_hdr_crc_code = {
	.bits = 6,
	.poly = 47,
	.init = 0,
	.remainder = 0,
};

/* Section 6.6.3.9 Payload CRC */
const struct osmo_crc16gen_code iuup_data_crc_code = {
	.bits = 10,
	.poly = 563,
	.init = 0,
	.remainder = 0,
};

/***********************************************************************
 * Primitives towars the upper layers at the RNL SAP
 ***********************************************************************/

/* 3GPP TS 25.415 Section 7.2.1 */
enum osmo_rnl_iuup_prim_type {
	OSMO_RNL_IUUP_DATA,
	OSMO_RNL_IUUP_STATUS,
	OSMO_RNL_IUUP_UNIT_DATA,
};

struct osmo_rnl_iuup_data {
	uint8_t rfci;
	uint8_t frame_nr;
	uint8_t fqc;
	/* ?? */
	uint8_t *data;
	uint8_t data_len;
};

struct osmo_rnl_iuup_status {
	enum iuup_procedure procedure;
	union {
		struct {
			cause;
			distance;
		} error_event;
		struct {
		} initialization;
		struct {
		} rate_control;
		struct {
		} time_alignment;
	} u;
};

struct osmo_rnl_iuup_prim {
	struct osmo_prim_hdr oph;
	union {
		struct osmo_rnl_iuup_data data;
		struct osmo_rnl_iuup_status status;
		struct osmo_rnl_iuup_unitdata unitdata;
	} u;
};


/***********************************************************************
 * Internal State / FSM (Annex B)
 ***********************************************************************/

struct osmo_timer_nt {
	uint32_t t_ms;	/* time in ms */
	uint32_t n;	/* number of repetitons */
	struct osmo_timer timer;
};

struct osmo_iuup_instance {
	struct osmo_fsm_inst *fi;
	struct {
		struct osmo_timer_nt init;
		struct osmo_timer_nt ta;
		struct osmo_timer_nt rc;
	} timer;
};

enum iuup_fsm_state {
	IUUP_FSM_ST_NULL,
	IUUP_FSM_ST_INIT,
	IUUP_FSM_ST_TrM_DATA_XFER_READY,
	IUUP_FSM_ST_SMpSDU_DATA_XFER_READY,
};

enum iuup_fsm_event {
	IUUP_FSM_EVT_IUUP_CONFIG_REQ,
	IUUP_FSM_EVT_IUUP_DATA_REQ,
	IUUP_FSM_EVT_IUUP_DATA_IND,
	IUUP_FSM_EVT_SSASAR_UNITDATA_REQ,
	IUUP_FSM_EVT_SSASAR_UNITDATA_IND,
	IUUP_FSM_EVT_IUUP_UNITDATA_REQ,
	IUUP_FSM_EVT_IUUP_UNITDATA_IND,
	IUUP_FSM_EVT_INIT,
	IUUP_FSM_EVT_LAST_INIT_ACK,
	IUUP_FSM_EVT_INIT_NACK,
};

static const struct value_string iuup_fsm_event_names[] = {
	{ IUUP_FSM_EVT_IUUP_CONFIG_REQ, 	"IuUP-CONFIG.req" },
	{ IUUP_FSM_EVT_IUUP_DATA_REQ, 		"IuUP-DATA.req" },
	{ IUUP_FSM_EVT_IUUP_DATA_IND, 		"IuUP-DATA.ind" },
	{ IUUP_FSM_EVT_SSASAR_UNITDATA_REQ, 	"SSSAR-UNITDATA.req" },
	{ IUUP_FSM_EVT_SSASAR_UNITDATA_IND, 	"SSSAR-UNITDATA.ind" },
	{ IUUP_FSM_EVT_IUUP_UNITDATA_REQ,	"IuUP-UNITDATA.req" },
	{ IUUP_FSM_EVT_IUUP_UNITDATA_IND,	"IuUP-UNITDATA.ind" },
	{ IUUP_FSM_EVT_INIT,			"INIT" },
	{ IUUP_FSM_EVT_LAST_INIT_ACK,		"LAST_INIT_ACK" },
	{ IUUP_FSM_EVT_INIT_NACK,		"INIT_NACK" },
	{ 0, NULL }
};

static void iuup_fsm_null(struct osmo_fsm_instance *fi, uint32_t event, void *data)
{
	switch (event) {
	case IUUP_FSM_EVT_IUUP_CONFIG_REQ:
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_TrM_DATA_XFER_READY, 0, 0);
		/* or */
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_INIT, 0, 0);
		break;
	}
}

static void iuup_fsm_trm_data(struct osmo_fsm_instance *fi, uint32_t event, void *data)
{
	switch (event) {
	case IUUP_FSM_EVT_IUUP_CONFIG_REQ:
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_NULL, 0, 0);
		break;
	case IUUP_FSM_EVT_IUUP_DATA_REQ:
	case IUUP_FSM_EVT_IUUP_DATA_IND:
	case IUUP_FSM_EVT_IUUP_UNITDATA_REQ:
	case IUUP_FSM_EVT_IUUP_UNITDATA_IND:
	case IUUP_FSM_EVT_SSASAR_UNITDATA_REQ:
	case IUUP_FSM_EVT_SSASAR_UNITDATA_IND:
		/* no state change */
		break;
	}
}

static void iuup_fsm_init(struct osmo_fsm_instance *fi, uint32_t event, void *data)
{
	switch (event) {
	case IUUP_FSM_EVT_IUUP_CONFIG_REQ:
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_NULL, 0, 0);
		break;
	case IUUP_FSM_EVT_LAST_INIT_ACK:
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_TrM_DATA_XFER_READY, 0, 0);
		break;
	}
}

static void iuup_fsm_smpsdu_data(struct osmo_fsm_instance *fi, uint32_t event, void *data)
{
	switch (event) {
	case IUUP_FSM_EVT_IUUP_CONFIG_REQ:
		osmo_fsm_inst_state_chg(fi, IUUP_FSM_ST_NULL, 0, 0);
		break;
	case IUUP_FSM_EVT_IUUP_DATA_REQ:
	case IUUP_FSM_EVT_IUUP_DATA_IND:
	case IUUP_FSM_EVT_IUUP_UNITDATA_REQ:
	case IUUP_FSM_EVT_IUUP_UNITDATA_IND:
	case IUUP_FSM_EVT_SSASAR_UNITDATA_REQ:
	case IUUP_FSM_EVT_SSASAR_UNITDATA_IND:
		/* no state change */
		break;
	}
}

static const struct osmo_fsm_state iuup_fsm_states[] = {
	[IUUP_FSM_ST_NULL] = {
		.in_event_mask = S(IUUP_FSM_EVT_IUUP_CONFIG_REQ),
		.out_state_mask = S(IUUP_FSM_ST_INIT) |
				  S(IUUP_FSM_ST_TrM_DATA_XFER_READY),
		.naem = "NULL",
		.action = iuup_fsm_null,
	},
	[IUUP_FSM_ST_TrM_DATA_XFER_READY] = {
		.in_event_mask = S(IUUP_FSM_EVT_IUUP_CONFIG_REQ) |
				 S(IUUP_FSM_EVT_IUUP_DATA_REQ) |
				 S(IUUP_FSM_EVT_IUUP_DATA_IND) |
				 S(IUUP_FSM_EVT_IUUP_UNITDATA_REQ) |
				 S(IUUP_FSM_EVT_IUUP_UNITDATA_IND) |
				 S(IUUP_FSM_EVT_SSASAR_UNITDATA_REQ) |
				 S(IUUP_FSM_EVT_SSASAR_UNITDATA_IND),
		.out_state_mask = S(IUUP_FSM_ST_NULL),
		.name = "TrM Data Transfer Ready",
		.action = iuup_fsm_trm_data,
	},
	[IUUP_FSM_ST_INIT] = {
		.in_event_mask = ,
		.out_state_mask = S(IUUP_FSM_ST_NULL) |
				  S(IUUP_FSM_ST_SMpSDU_DATA_XFER_READY),
		.name = "Initialisation",
		.action = iuup_fsm_init,
	},
	[IUUP_FSM_ST_SMpSDU_DATA_XFER_READY] = {
		.in_event_mask = ,
		.out_state_mask = S(IUUP_FSM_ST_NULL) |
				  S(IUUP_FSM_ST_INIT),
		.name = "SMpSDU Data Transfer Ready",
		.action = iuup_fsm_smpsdu_data,
	},
};

struct osmo_fsm iuup_fsm = {
	.name = "IuUP",
	.states = iuup_fsm_states,
	.num_states = ARRAY_SIZE(iuup_fsm_states),
	.log_subsys = FIXME,
	.event_names = iuup_fsm_event_names,
};
