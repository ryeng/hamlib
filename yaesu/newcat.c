/*
 * hamlib - (C) Frank Singleton 2000 (javabear at users.sourceforge.net)
 *              and the Hamlib Group (hamlib-developer at lists.sourceforge.net)
 *
 * newcat.c - (C) Nate Bargmann 2007 (n0nb at arrl.net)
 *            (C) Stephane Fillod 2008
 *
 * This shared library provides an API for communicating
 * via serial interface to any newer Yaesu radio using the
 * "new" text CAT interface.
 *
 * Models this code aims to support are FTDX-9000*, FT-2000,
 * FT-950, FT-450.  Much testing remains.  -N0NB
 *
 *
 * $Id: newcat.c,v 1.18 2008-12-13 21:23:31 mrtembry Exp $
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */

#include "hamlib/rig.h"
#include "iofunc.h"
#include "newcat.h"

/* global variables */
static char cat_term = ';';             /* Yaesu command terminator */

/*
 * The following table defines which commands are valid for any given
 * rig supporting the "new" CAT interface.
 */

typedef struct _yaesu_newcat_commands {
    char                *command;
    ncboolean           ft450;
    ncboolean           ft950;
    ncboolean           ft2000;
    ncboolean           ft9000;
} yaesu_newcat_commands_t;

/*
 * NOTE: The following table must be in alphabetical order by the
 * command.  This is because it is searched using a binary search
 * to determine whether or not a command is valid for a given rig.
 *
 * The list of supported commands is obtained from the rig's operator's
 * or CAT programming manual.
 *
 */
yaesu_newcat_commands_t valid_commands[] = {
                        /*   Command    FT-450  FT-950  FT-2000 FT-9000 */
                            {"AB",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"AC",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"AG",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"AI",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"AM",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"AN",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"BC",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"BD",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"BI",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"BP",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"BS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"BU",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"BY",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"CH",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"CN",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"CO",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"CS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"CT",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"DA",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"DN",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"DP",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"DS",      TRUE,   FALSE,  TRUE,   FALSE   },
                            {"ED",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"EK",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"EU",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"EX",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"FA",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"FB",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"FK",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"FR",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"FS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"FT",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"GT",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"ID",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"IF",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"IS",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"KM",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"KP",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"KR",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"KS",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"KY",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"LK",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"LM",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MA",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"MC",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MD",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MG",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MK",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"ML",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MR",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"MW",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"MX",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"NA",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"NB",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"NL",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"NR",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"OI",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"OS",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"PA",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"PB",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"PC",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"PL",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"PR",      FALSE,  TRUE,   TRUE,   TRUE    },
                            {"PS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"QI",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"QR",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"QS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"RA",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"RC",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"RD",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"RF",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"RG",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"RI",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"RL",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"RM",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"RO",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"RP",      TRUE,   TRUE,   FALSE,  FALSE   },
                            {"RS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"RT",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"RU",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"SC",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"SD",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"SF",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"SH",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"SM",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"SQ",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"ST",      TRUE,   FALSE,  FALSE,  FALSE   },
                            {"SV",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"TS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"TX",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"UL",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"UP",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"VD",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"VF",      FALSE,  TRUE,   TRUE,   FALSE   },
                            {"VG",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"VM",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"VR",      TRUE,   FALSE,  FALSE,  FALSE   },
                            {"VS",      TRUE,   TRUE,   TRUE,   FALSE   },
                            {"VV",      TRUE,   FALSE,  FALSE,  FALSE   },
                            {"VX",      TRUE,   TRUE,   TRUE,   TRUE    },
                            {"XT",      FALSE,  TRUE,   TRUE,   TRUE    },
};
int                     valid_commands_count = sizeof(valid_commands) / sizeof(yaesu_newcat_commands_t);

/*
 * future - private data
 *
 * FIXME: Does this need to be exposed to the application/frontend through
 * rig_caps.priv?  I'm guessing not since it's private to the backend.  -N0NB
 */

struct newcat_priv_data {
    unsigned int        read_update_delay;              /* depends on pacing value */
    vfo_t               current_vfo;                    /* active VFO from last cmd */
    char                cmd_str[NEWCAT_DATA_LEN];       /* command string buffer */
    char                ret_data[NEWCAT_DATA_LEN];      /* returned data--max value, most are less */
    unsigned char       current_mem;                    /* private memory channel number */
};


/* NewCAT Internal Functions */
ncboolean newcat_is_rig(RIG * rig, rig_model_t model);
int newcat_get_txvfo(RIG * rig, vfo_t * txvfo);
int newcat_set_txvfo(RIG * rig, vfo_t txvfo);
int newcat_get_rxvfo(RIG * rig, vfo_t * rxvfo);
int newcat_set_rxvfo(RIG * rig, vfo_t rxvfo);
int newcat_set_vfo_from_alias(RIG * rig, vfo_t * vfo);
int newcat_scale_float(int scale, float fval);


/*
 * ************************************
 *
 * Hamlib API functions
 *
 * ************************************
 */

/*
 * rig_init
 *
 */

int newcat_init(RIG *rig) {
    struct newcat_priv_data *priv;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)malloc(sizeof(struct newcat_priv_data));
    if (!priv)                                  /* whoops! memory shortage! */
        return -RIG_ENOMEM;

    /* TODO: read pacing from preferences */
//    priv->pacing = NEWCAT_PACING_DEFAULT_VALUE; /* set pacing to minimum for now */
    priv->read_update_delay = NEWCAT_DEFAULT_READ_TIMEOUT; /* set update timeout to safe value */

//    priv->current_vfo =  RIG_VFO_MAIN;          /* default to whatever */
	priv->current_vfo = RIG_VFO_A;
	  
    rig->state.priv = (void *)priv;

    return RIG_OK;
}


/*
 * rig_cleanup
 *
 * the serial port is closed by the frontend
 *
 */

int newcat_cleanup(RIG *rig) {

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    if (rig->state.priv)
        free(rig->state.priv);
    rig->state.priv = NULL;

    return RIG_OK;
}


/*
 * rig_open
 *
 * New CAT does not support pacing
 *
 */

int newcat_open(RIG *rig) {
    struct rig_state *rig_s;
//    struct newcat_priv_data *priv;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

//    priv = (struct newcat_priv_data *)rig->state.priv;
    rig_s = &rig->state;

    rig_debug(RIG_DEBUG_TRACE, "%s: write_delay = %i msec\n",
              __func__, rig_s->rigport.write_delay);
    rig_debug(RIG_DEBUG_TRACE, "%s: post_write_delay = %i msec\n",
              __func__, rig_s->rigport.post_write_delay);

    return RIG_OK;
}


/*
 * rig_close
 *
 */

int newcat_close(RIG *rig) {

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    return RIG_OK;
}


/*
 * rig_set_freq
 *
 * Set frequency for a given VFO
 * RIG_TARGETABLE_VFO
 * Does not SET priv->current_vfo
 *
 */

int newcat_set_freq(RIG *rig, vfo_t vfo, freq_t freq) {
    const struct rig_caps *caps;
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char c;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)rig->state.priv;
    caps = rig->caps;
    state = &rig->state;

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed freq = %"PRIfreq" Hz\n", __func__, freq);

    /* additional debugging */
    rig_debug(RIG_DEBUG_TRACE, "%s: R2 minimum freq = %"PRIfreq" Hz\n", __func__, caps->rx_range_list2[0].start);
    rig_debug(RIG_DEBUG_TRACE, "%s: R2 maximum freq = %"PRIfreq" Hz\n", __func__, caps->rx_range_list2[0].end);

    if (freq < caps->rx_range_list1[0].start || freq > caps->rx_range_list1[0].end ||
        freq < caps->rx_range_list2[0].start || freq > caps->rx_range_list2[0].end)
        return -RIG_EINVAL;

    //if (vfo == RIG_VFO_CURR) {
    //    vfo = priv->current_vfo;        /* from previous vfo cmd */
    //    rig_debug(RIG_DEBUG_TRACE, "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    //} else if (vfo != priv->current_vfo) {
    //    /* force a VFO change if requested vfo value differs from stored value */
    //    err = newcat_set_vfo(rig, vfo);
    //    if (err != RIG_OK)
    //        return err;
    //}

    err = newcat_set_vfo_from_alias(rig, &vfo);
    if (err < 0)
        return err;

    switch (vfo) {
    case RIG_VFO_A:
        c = 'A';
        break;
    case RIG_VFO_B:
        c = 'B';
        break;
    default:
        return -RIG_ENIMPL;             /* Only VFO_A or VFO_B are valid */
    }

 // W1HKJ
 // creation of the priv structure guarantees that the string can be NEWCAT_DATA_LEN
 // bytes in length.  the snprintf will only allow (NEWCAT_DATA_LEN - 1) chars
 // followed by the NULL terminator.
 // CAT command string for setting frequency requires that 8 digits be sent
 // including leading fill zeros

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "F%c%08d%c", c, (int)freq, cat_term);
#if 0
    if (!newcat_valid_command(rig, command))
        return -RIG_ENAVAIL;
#endif

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    return RIG_OK;
}


/*
 * rig_get_freq
 *
 * Return Freq for a given VFO
 * RIG_TARGETABLE_FREQ
 * Does not SET priv->current_vfo
 *
 */

int newcat_get_freq(RIG *rig, vfo_t vfo, freq_t *freq) {
    char command[3];
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char c;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    //if (vfo == RIG_VFO_CURR) {
    //    err = newcat_get_vfo(rig, &priv->current_vfo);
    //    if (err != RIG_OK)
    //        return err;

    //    vfo = priv->current_vfo;    /* from previous get_vfo cmd */
    //    rig_debug(RIG_DEBUG_TRACE,
    //              "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    //}

    err = newcat_set_vfo_from_alias(rig, &vfo);
    if (err < 0)
        return err;

    switch(vfo) {
    case RIG_VFO_A:
    case RIG_VFO_VFO:
        c = 'A';
        break;
    case RIG_VFO_B:
        c = 'B';
        break;
//    case RIG_VFO_MEM:
//    case RIG_VFO_MAIN:
//        break;
    default:
        return -RIG_EINVAL;         /* sorry, unsupported VFO */
    }

    /* Build the command string */
    snprintf(command, sizeof(command), "F%c", c);
    if (!newcat_valid_command(rig, command))
        return -RIG_ENAVAIL;
    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", command, cat_term);

    rig_debug(RIG_DEBUG_TRACE, "cmd_str = %s\n", priv->cmd_str);

    /* get freq */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
                      &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
              __func__, err, priv->ret_data);

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {

        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n",
                  __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    /* convert the read frequency string into freq_t and store in *freq */
    sscanf(priv->ret_data+2, "%"SCNfreq, freq);

    rig_debug(RIG_DEBUG_TRACE,
              "%s: freq = %"PRIfreq" Hz for vfo 0x%02x\n", __func__, freq, vfo);

    return RIG_OK;
}


int newcat_set_mode(RIG *rig, vfo_t vfo, rmode_t mode, pbwidth_t width)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
	char cmdstr[] = "MD0x;";


    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

	if (vfo != RIG_VFO_CURR)
		return -RIG_ENTARGET;

	rig_debug(RIG_DEBUG_VERBOSE,"ft450: generic mode = %x \n", mode);

	switch(mode) {
		case RIG_MODE_LSB:
			cmdstr[3] = '1';
			break;
		case RIG_MODE_USB:
			cmdstr[3] = '2';
			break;
		case RIG_MODE_CW:
			cmdstr[3] = '3';
			break;
		case RIG_MODE_WFM:
			cmdstr[3] = '4';
			break;
		case RIG_MODE_AM:
			cmdstr[3] = '5';
			break;
		case RIG_MODE_RTTY:
			cmdstr[3] = '6';
			break;
		case RIG_MODE_CWR:
			cmdstr[3] = '7';
			break;
		case RIG_MODE_PKTLSB:
			cmdstr[3] = '8';
			break;
		case RIG_MODE_RTTYR:
			cmdstr[3] = '9';
			break;
		case RIG_MODE_FM:
			cmdstr[3] = 'B';
			break;
		case RIG_MODE_PKTUSB:
			cmdstr[3] = 'C';
			break;
		default:
			return -RIG_EINVAL;
	}

    err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
    if (err != RIG_OK)
        return err;

    return RIG_OK;
}

int newcat_get_mode(RIG *rig, vfo_t vfo, rmode_t *mode, pbwidth_t *width)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char c;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    /* Build the command string */
    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "MD0;");

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get VFO */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
                      &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n",
                  __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
              __func__, err, priv->ret_data);

    /*
     * The current mode value is a digit '0' ... 'C'
     * embedded at ret_data[3] in the read string.
     */
    c = priv->ret_data[3];

    switch (c) {
	    case '1':
    		*mode = RIG_MODE_LSB;
        	break;
		case '2':
			*mode = RIG_MODE_USB;
			break;
		case '3':
			*mode = RIG_MODE_CW;
			break;
		case '4':
			*mode = RIG_MODE_WFM;
			break;
		case '5':
			*mode = RIG_MODE_AM;
			break;
		case '6':
			*mode = RIG_MODE_RTTY;
			break;
		case '7':
			*mode = RIG_MODE_CWR;
			break;
		case '8':
			*mode = RIG_MODE_PKTLSB;
			break;
		case '9':
			*mode = RIG_MODE_RTTYR;
			break;
		case 'B':
			*mode = RIG_MODE_FM;
			break;
		case 'C':
			*mode = RIG_MODE_PKTUSB;
			break;
	    default:
    	    return -RIG_EINVAL;
    }

    *width = 2400;      /* dummy */

    return RIG_OK;

}

/*
 * rig_set_vfo
 *
 * set vfo and store requested vfo for later RIG_VFO_CURR
 * requests.
 *
 */

int newcat_set_vfo(RIG *rig, vfo_t vfo) {
    char command[3];
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char c;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    err = newcat_set_vfo_from_alias(rig, &vfo);   /* passes RIG_VFO_MEM, RIG_VFO_A, RIG_VFO_B */
    if (err < 0)
        return err;

    /* FIXME: Include support for RIG_VFO_MAIN, RIG_VFO_MEM */
    switch(vfo) {
    case RIG_VFO_A:
        c = '0';
        break;
    case RIG_VFO_B:
        c = '1';
        break;
//    case RIG_VFO_MEM:
        /* reset to memory channel stored by previous get_vfo
         * The recall mem channel command uses 0x01 though 0x20
         */
//        err = newcat_send_dynamic_cmd(rig, FT450_NATIVE_RECALL_MEM,
//                                     (priv->current_mem + 1), 0, 0, 0);
//        if (err != RIG_OK)
//            return err;

//        priv->current_vfo = vfo;

//        rig_debug(RIG_DEBUG_TRACE, "%s: set mem channel = 0x%02x\n",
//                  __func__, priv->current_mem);
//        return RIG_OK;
    default:
        return -RIG_ENIMPL;         /* sorry, VFO not implemented */
    }

    /* Build the command string */
    snprintf(command, sizeof(command), "VS");

    if (!newcat_valid_command(rig, command)) {
        err = newcat_set_rxvfo(rig, vfo);       /* Try set with "FR" */
        if (err != RIG_OK)
            return err;

        return RIG_OK;
    }

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c%c", command, c, cat_term);

    rig_debug(RIG_DEBUG_TRACE, "cmd_str = %s\n", priv->cmd_str);

    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    priv->current_vfo = vfo;    /* if set_vfo worked, set current_vfo */ 
    rig_debug(RIG_DEBUG_TRACE, "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);

    return RIG_OK;
}


/*
 * rig_get_vfo
 *
 * get current RX vfo/mem and store requested vfo for
 * later RIG_VFO_CURR requests plus pass the tested vfo/mem
 * back to the frontend.
 *
 * TODO: determine memory status if possible
 */

int newcat_get_vfo(RIG *rig, vfo_t *vfo) {
    char command[3];
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char c;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    /* Build the command string */
    snprintf(command, sizeof(command), "VS");

    if (!newcat_valid_command(rig, command)) {
        err = newcat_get_rxvfo(rig, vfo);       /* Try get with "FR" */
        if (err != RIG_OK)
            return err;

        return RIG_OK;
    }

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s;", command);

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get VFO */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
                      &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n",
                  __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, VFO value = %c\n",
              __func__, err, priv->ret_data, priv->ret_data[2]);

    /*
     * The current VFO value is a digit ('0' or '1' ('A' or 'B' respectively))
     * embedded at ret_data[2] in the read string.
     */
    if (strcmp(priv->ret_data, "?;") == 0) {
    	rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, setting VFO to A\n");
    	*vfo = RIG_VFO_A;
    	priv->current_vfo = RIG_VFO_A;
    	return RIG_OK;
	}
	
    c = priv->ret_data[2];

    switch (c) {
    case '0':
        *vfo = RIG_VFO_A;
        break;
    case '1':
        *vfo = RIG_VFO_B;
        break;
    default:
//        switch (stat_mem) {
//        case SF_MT:
//        case SF_MR:
//            *vfo = RIG_VFO_MEM;
//            priv->current_vfo = RIG_VFO_MEM;

            /*
             * Per Hamlib policy capture and store memory channel number
             * for future set_vfo command.
             */
//            err = newcat_get_update_data(rig, FT450_NATIVE_MEM_CHNL,
//                                        FT450_MEM_CHNL_LENGTH);
//            if (err != RIG_OK)
//                return err;

//            priv->current_mem = priv->update_data[FT450_SUMO_MEM_CHANNEL];

//            rig_debug(RIG_DEBUG_TRACE, "%s: stored mem channel = 0x%02x\n",
//                      __func__, priv->current_mem);
//            break;
//        default:                      /* Oops! */
//            return -RIG_EINVAL;         /* sorry, wrong current VFO */
//        }
        return -RIG_EINVAL;         /* sorry, wrong current VFO */
    }
    rig_debug(RIG_DEBUG_TRACE, "%s: set vfo = 0x%02x\n", __func__, *vfo);

    priv->current_vfo = *vfo;       /* set current_vfo now */

    return RIG_OK;

}


int newcat_set_ptt(RIG *rig, vfo_t vfo, ptt_t ptt)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
	char txon[] = "TX1;";
	char txoff[] = "TX0;";
	
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

	switch(ptt) {
		case RIG_PTT_ON:
			err = write_block(&state->rigport, txon, strlen(txon));
			break;
		case RIG_PTT_OFF:
			err = write_block(&state->rigport, txoff, strlen(txoff));
			break;
		default:
			return -RIG_EINVAL;
	}
    return err;
}


int newcat_get_ptt(RIG * rig, vfo_t vfo, ptt_t * ptt)
{
    char c;
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", "TX", cat_term);

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get PTT */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, PTT value = %c\n", __func__, err, priv->ret_data, priv->ret_data[2]);

    if (strcmp(priv->ret_data, "?;") == 0) {
        rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting PTT\n");
        return RIG_OK;
    }

    c = priv->ret_data[2];
    switch (c) {
        case '0':                 /* FT-950 "TX OFF", Original Release Firmware */
            *ptt = RIG_PTT_OFF;
            break;
        case '1' :                /* Just because,    what the CAT Manual Shows */
        case '2' :                /* FT-950 Radio:    Mic, Dataport, CW "TX ON" */
        case '3' :                /* FT-950 CAT port: Radio in "TX ON" mode     [Not what the CAT Manual Shows] */
            *ptt = RIG_PTT_ON;
            break;
        default:
            return -RIG_EINVAL;
    }

    return RIG_OK;
}


int newcat_get_dcd(RIG * rig, vfo_t vfo, dcd_t * dcd)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_rptr_shift(RIG * rig, vfo_t vfo, rptr_shift_t rptr_shift)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_rptr_shift(RIG * rig, vfo_t vfo, rptr_shift_t * rptr_shift)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_rptr_offs(RIG * rig, vfo_t vfo, shortfreq_t offs)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_rptr_offs(RIG * rig, vfo_t vfo, shortfreq_t * offs)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_split_freq(RIG * rig, vfo_t vfo, freq_t tx_freq)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_split_freq(RIG * rig, vfo_t vfo, freq_t * tx_freq)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_split_mode(RIG * rig, vfo_t vfo, rmode_t tx_mode, pbwidth_t tx_width)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_split_mode(RIG * rig, vfo_t vfo, rmode_t * tx_mode, pbwidth_t * tx_width)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_split_vfo(RIG * rig, vfo_t vfo, split_t split, vfo_t tx_vfo)
{
    int err;
    vfo_t rxvfo;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    err = newcat_get_vfo(rig, &rxvfo);  /* sync to rig current vfo */
    if (err != RIG_OK)
        return err;

    err = newcat_set_vfo_from_alias(rig, &vfo);
    if (err < 0)
        return err;

    switch (split) {
        case RIG_SPLIT_OFF:
            err = newcat_set_txvfo(rig, vfo);
            if (err != RIG_OK)
                return err;

            if (rxvfo != vfo) {
                err = newcat_set_vfo(rig, vfo);
                if (err != RIG_OK)
                    return err;
            }
            break;
        case RIG_SPLIT_ON:
            err = newcat_set_txvfo(rig, tx_vfo);
            if (err != RIG_OK)
                return err;

            if (rxvfo != vfo) {
            err = newcat_set_vfo(rig, vfo);
            if (err != RIG_OK)
                return err;
            }
            break;
        default:
            return -RIG_EINVAL;
    }

    return RIG_OK;
}


int newcat_get_split_vfo(RIG * rig, vfo_t vfo, split_t * split, vfo_t *tx_vfo)
{
    int err;
    vfo_t rvfo;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    err = newcat_get_vfo(rig, &rvfo);
    if (err != RIG_OK)
        return err;

    err = newcat_get_txvfo(rig, tx_vfo);
    if (err != RIG_OK)
        return err;

    if (*tx_vfo != rvfo)
        *split = RIG_SPLIT_ON;
    else 
       *split = RIG_SPLIT_OFF;

    return RIG_OK;
}


int newcat_set_rit(RIG * rig, vfo_t vfo, shortfreq_t rit)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    if (abs(rit) > rig->caps->max_rit)
        return -RIG_EINVAL;

    if (rit == 0)
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cRT0%c", cat_term, cat_term);
    else if (rit < 0)
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cRD%04d%cRT1%c", cat_term, abs(rit), cat_term, cat_term);
    else
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cRU%04d%cRT1%c", cat_term, abs(rit), cat_term, cat_term);

    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    return RIG_OK;
}


int newcat_get_rit(RIG * rig, vfo_t vfo, shortfreq_t * rit)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char *retval;
    char rit_on;
    int err;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    *rit = 0;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", "IF", cat_term);

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get RIT */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, RIT value = %c\n", __func__, err, priv->ret_data, priv->ret_data[18]);

    if (strcmp(priv->ret_data, "?;") == 0) {
        rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting RIT\n");
        return RIG_OK;
    }
    retval = priv->ret_data + 13;
    rit_on = retval[5];
    retval[5] = '\0';

    if (rit_on == '1')
        *rit = (shortfreq_t) atoi(retval);

    return RIG_OK;
}


int newcat_set_xit(RIG * rig, vfo_t vfo, shortfreq_t xit)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    if (!newcat_valid_command(rig, "XT"))
        return -RIG_ENAVAIL;

    if (abs(xit) > rig->caps->max_xit)
        return -RIG_EINVAL;

    if (xit == 0)
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cXT0%c", cat_term, cat_term);
    else if (xit < 0)
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cRD%04d%cXT1%c", cat_term, abs(xit), cat_term, cat_term);
    else
        snprintf(priv->cmd_str, sizeof(priv->cmd_str), "RC%cRU%04d%cXT1%c", cat_term, abs(xit), cat_term, cat_term);

    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    return RIG_OK;
}


int newcat_get_xit(RIG * rig, vfo_t vfo, shortfreq_t * xit)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    char *retval;
    char xit_on;
    int err;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    if (!newcat_valid_command(rig, "XT"))
        return -RIG_ENAVAIL;

    *xit = 0;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", "IF", cat_term);

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get XIT */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, XIT value = %c\n", __func__, err, priv->ret_data, priv->ret_data[19]);

    if (strcmp(priv->ret_data, "?;") == 0) {
        rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting XIT\n");
        return RIG_OK;
    }
    retval = priv->ret_data + 13;
    xit_on = retval[6];
    retval[5] = '\0';

    if (xit_on == '1')
        *xit = (shortfreq_t) atoi(retval);

    return RIG_OK;
}


int newcat_set_ts(RIG * rig, vfo_t vfo, shortfreq_t ts)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_ts(RIG * rig, vfo_t vfo, shortfreq_t * ts)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_dcs_code(RIG * rig, vfo_t vfo, tone_t code)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_dcs_code(RIG * rig, vfo_t vfo, tone_t * code)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_tone(RIG * rig, vfo_t vfo, tone_t tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_tone(RIG * rig, vfo_t vfo, tone_t * tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_ctcss_tone(RIG * rig, vfo_t vfo, tone_t tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_ctcss_tone(RIG * rig, vfo_t vfo, tone_t * tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_dcs_sql(RIG * rig, vfo_t vfo, tone_t code)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_dcs_sql(RIG * rig, vfo_t vfo, tone_t * code)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_tone_sql(RIG * rig, vfo_t vfo, tone_t tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_tone_sql(RIG * rig, vfo_t vfo, tone_t * tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_ctcss_sql(RIG * rig, vfo_t vfo, tone_t tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_ctcss_sql(RIG * rig, vfo_t vfo, tone_t * tone)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_power2mW(RIG * rig, unsigned int *mwpower, float power, freq_t freq, rmode_t mode)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (newcat_is_rig(rig, RIG_MODEL_FT950)) {
        *mwpower = power * 100000;              /* 0..100 Linear scale */
    } else if (newcat_is_rig(rig, RIG_MODEL_FT450)) {
        *mwpower = power * 100000;      /* FIXME: 0..255 scale... Linear or Not */ 
    } else 
        return -RIG_ENAVAIL;
    
    return RIG_OK;
}


int newcat_mW2power(RIG * rig, float *power, unsigned int mwpower, freq_t freq, rmode_t mode)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);
    
    if (newcat_is_rig(rig, RIG_MODEL_FT950)) {
        *power = mwpower / 100000.;              /* 0..100 Linear scale */
    } else if (newcat_is_rig(rig, RIG_MODEL_FT450)) {
        *power = mwpower / 100000.;     /* FIXME: 0..255 scale... Linear or Not */ 
    } else  
        return -RIG_ENAVAIL;
    
    return RIG_OK;
}


int newcat_set_powerstat(RIG * rig, powerstat_t status)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_powerstat(RIG * rig, powerstat_t * status)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_reset(RIG * rig, reset_t reset)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_ant(RIG * rig, vfo_t vfo, ant_t ant)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    char command[] = "AN";
    char which_ant;
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    if (!newcat_valid_command(rig, command))
        return -RIG_ENAVAIL;

    /* TODO: ADD RX only antenna */ 

    switch (ant) {
        case RIG_ANT_1:
            which_ant = '1';
            break;
         case RIG_ANT_2:
            which_ant = '2';
            break;
         case RIG_ANT_3:
            if (newcat_is_rig(rig, RIG_MODEL_FT950)) /* FT2000 also */
                return -RIG_EINVAL;
            which_ant = '3';
            break;
         case RIG_ANT_4:
            if (newcat_is_rig(rig, RIG_MODEL_FT950))
                return -RIG_EINVAL;
            which_ant = '4';
            break;
         default:
            return -RIG_EINVAL;
    }

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s0%c%c", command, which_ant, cat_term);
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));

    return err;
}


int newcat_get_ant(RIG * rig, vfo_t vfo, ant_t * ant)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    char c;
    char command[] = "AN";
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!newcat_valid_command(rig, command))
        return -RIG_ENAVAIL;

    /* TODO: ADD RX only antenna */ 

    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s0%c", command, cat_term);
    /* Get ANT */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, ANT value = %c\n", __func__, err, priv->ret_data, priv->ret_data[3]);

    if (strcmp(priv->ret_data, "?;") == 0) {
        rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting ANT\n");
        return RIG_OK;
    }

    c = priv->ret_data[3];
    switch (c) {
        case '1':
            *ant = RIG_ANT_1;
            break;
        case '2' :
            *ant = RIG_ANT_2;
            break;
        case '3' :
            *ant = RIG_ANT_3;
            break;
        case '4' :
            *ant = RIG_ANT_4;
            break;
        default:
            return -RIG_EINVAL;
    }

    return RIG_OK;
}


int newcat_set_level(RIG * rig, vfo_t vfo, setting_t level, value_t val)
{
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	int i;
	char cmdstr[16];
	int scale;
	int fpf;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!rig)
		return -RIG_EINVAL;

	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	switch (level) {
		case RIG_LEVEL_RFPOWER:
			scale = (newcat_is_rig(rig, RIG_MODEL_FT950)) ? 100 : 255;
			// sprintf(cmdstr, "PC%03d%c", (int)(scale * val.f), cat_term);
			fpf = newcat_scale_float(scale, val.f);
			sprintf(cmdstr, "PC%03d%c", fpf, cat_term);
			break;
		case RIG_LEVEL_AF:
			// sprintf(cmdstr, "AG0%03d%c", (int)(255*val.f), cat_term);
			fpf = newcat_scale_float(255, val.f);
			sprintf(cmdstr, "AG0%03d%c", fpf, cat_term);
			break;
		case RIG_LEVEL_AGC:
			switch (val.i) {
				case RIG_AGC_OFF: strcpy(cmdstr, "GT00;"); break;
				case RIG_AGC_FAST: strcpy(cmdstr, "GT01;"); break;
				case RIG_AGC_MEDIUM: strcpy(cmdstr, "GT02;"); break;
				case RIG_AGC_SLOW: strcpy(cmdstr, "GT03;"); break;
				case RIG_AGC_AUTO: strcpy(cmdstr, "GT04;"); break;
				default: return -RIG_EINVAL;
			}
			break;
		case RIG_LEVEL_IF:
			if (abs(val.i) > rig->caps->max_ifshift)
				return -RIG_EINVAL; 
			sprintf(cmdstr, "IS0%+.4d;", val.i);	/* format problem with %+04d */
			break;
		case RIG_LEVEL_CWPITCH:
			if (val.i < 300)
				i = 300;
			else if (val.i > 1050)
				i = 1050;
			else
				i = val.i;
			sprintf(cmdstr, "KP%02d;", 2*((i+50-300)/100));
			break;
		case RIG_LEVEL_KEYSPD:
			sprintf(cmdstr, "KS%03d;", val.i);
			break;
		case RIG_LEVEL_MICGAIN:
			// sprintf(cmdstr, "MG%03d;", (int)(255*val.f));
			fpf = newcat_scale_float(255, val.f);
			sprintf(cmdstr, "MG%03d;", fpf);
			break;
		case RIG_LEVEL_METER:
			switch (val.i) {
				case RIG_METER_ALC: strcpy(cmdstr, "MS1;"); break;
				case RIG_METER_PO:  strcpy(cmdstr, "MS2;"); break;
				case RIG_METER_SWR: strcpy(cmdstr, "MS3;"); break;
				default: return -RIG_EINVAL;
			}
			break;
		case RIG_LEVEL_PREAMP:
			if (val.i == 0) {
				strcpy(cmdstr, "PA00;");
				break;
			}
			cmdstr[0] = '\0';
			for (i=0; state->preamp[i] != RIG_DBLST_END; i++)
				if (state->preamp[i] == val.i) {
					sprintf(cmdstr, "PA0%d;", i+1);
					break;
				}
			if (strlen(cmdstr) != 0)
				break;

			return -RIG_EINVAL;
		case RIG_LEVEL_ATT:
			if (val.i == 0) {
				strcpy(cmdstr, "RA00;");
				break;
			}
			cmdstr[0] = '\0';
			for (i=0; state->attenuator[i] != RIG_DBLST_END; i++)
				if (state->attenuator[i] == val.i) {
					sprintf(cmdstr, "RA0%d;", i+1);
					break;  /* for loop */
				}
			if (strlen(cmdstr) != 0)
				break;

			return -RIG_EINVAL;
		case RIG_LEVEL_RF:
			// sprintf(cmdstr, "RG0%03d;", (int)(255*val.f));
			fpf = newcat_scale_float(255, val.f);
			sprintf(cmdstr, "RG0%03d;", fpf);
			break;
		case RIG_LEVEL_NR:
			if (newcat_is_rig(rig, RIG_MODEL_FT450)) {
				// sprintf(cmdstr, "RL0%02d;", (int)(10*val.f)+1);
				fpf = newcat_scale_float(10, val.f);
				sprintf(cmdstr, "RL0%02d;", fpf+1);
			} else {
				// sprintf(cmdstr, "RL0%02d;", (int)(100*val.f));
				fpf = newcat_scale_float(100, val.f);
				sprintf(cmdstr, "RL0%02d;", fpf);
				if (fpf < 1 || fpf > 15)
					return -RIG_EINVAL;
			}
			break;
		case RIG_LEVEL_COMP:
			scale = (newcat_is_rig(rig, RIG_MODEL_FT950)) ? 100 : 255;
			// sprintf(cmdstr, "PL%03d;", (int)(scale * val.f));
			fpf = newcat_scale_float(scale, val.f);
			sprintf(cmdstr, "PL%03d;", fpf);
			break;
		case RIG_LEVEL_BKINDL:
			/* FIXME: should be tenth of dots, newcat expects ms */
			/* GUI GRIG2 expects ms so keep FT450 and FT950 BKIN DELAY in ms for now */
			if (newcat_is_rig(rig, RIG_MODEL_FT950) || newcat_is_rig(rig, RIG_MODEL_FT450)) {
				if (((val.i > 0) && (val.i < 30)) || (val.i > 3000))	/* 0000, is rejected by FT950 */
					return -RIG_EINVAL;
				sprintf(cmdstr, "SD%04d;", val.i);
			} else
				sprintf(cmdstr, "SD%04d;", val.i);
			break;
		case RIG_LEVEL_SQL:
			// sprintf(cmdstr, "SQ0%03d;", (int)(255*val.f));
			fpf = newcat_scale_float(255, val.f);
			sprintf(cmdstr, "SQ0%03d;", fpf);
			break;
		case RIG_LEVEL_VOX:
			/* VOX delay, arg int (tenth of seconds), expects ms */
			/* GUI GRIG2 expects ms so keep FT950 and FT450 VOX delay in ms */
			if (newcat_is_rig(rig, RIG_MODEL_FT950)) {
				if (((val.i >= 0) && (val.i < 30)) || (val.i > 3000))
					return -RIG_EINVAL;
				sprintf(cmdstr, "VD%04d;", val.i);
			} else if (newcat_is_rig(rig, RIG_MODEL_FT450)) {
				if (((val.i >= 0) && (val.i < 100)) || (val.i > 3000))
					return -RIG_EINVAL;
				sprintf(cmdstr, "VD%04d;", val.i);
			} else
				sprintf(cmdstr, "VD%04d;", val.i*100);
			break;
		case RIG_LEVEL_VOXGAIN:
			scale = (newcat_is_rig(rig, RIG_MODEL_FT950)) ? 100 : 255;
			// sprintf(cmdstr, "VG%03d;", (int)(scale * val.f));
			fpf = newcat_scale_float(scale, val.f);
			sprintf(cmdstr, "VG%03d;", fpf);
			break;
		case RIG_LEVEL_ANTIVOX:
			if (newcat_is_rig(rig, RIG_MODEL_FT950)) {
				// sprintf(cmdstr, "EX117%03d;", (int)(100 * val.f));
				fpf = newcat_scale_float(100, val.f);
				sprintf(cmdstr, "EX117%03d;", fpf);
			} else
				return -RIG_EINVAL;
			break;
		default:
			return -RIG_EINVAL;
	}

	err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
	if (err != RIG_OK)
		return err;

	return RIG_OK;
}


int newcat_get_level(RIG * rig, vfo_t vfo, setting_t level, value_t * val)
{
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	int ret_data_len;
	const char *cmdstr;
	char *retlvl;
	float scale;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!rig)
		return -RIG_EINVAL;

	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	switch (level) {
		case RIG_LEVEL_RFPOWER:
			cmdstr = "PC;";
			break;
		case RIG_LEVEL_PREAMP:
			cmdstr = "PA0;";
			break;
		case RIG_LEVEL_AF:
			cmdstr = "AG0;";
			break;
		case RIG_LEVEL_AGC:
			cmdstr = "GT0;";
			break;
		case RIG_LEVEL_IF:
			cmdstr = "IS0;";
			break;
		case RIG_LEVEL_CWPITCH:
			cmdstr = "KP;";
			break;
		case RIG_LEVEL_KEYSPD:
			cmdstr = "KS;";
			break;
		case RIG_LEVEL_MICGAIN:
			cmdstr = "MG;";
			break;
		case RIG_LEVEL_METER:
			cmdstr = "MS;";
			break;
		case RIG_LEVEL_ATT:
			cmdstr = "RA0;";
			break;
		case RIG_LEVEL_RF:
			cmdstr = "RG0;";
			break;
		case RIG_LEVEL_COMP:
			cmdstr = "PL;";
			break;
		case RIG_LEVEL_NR:
			cmdstr = "RL0;";
			break;
		case RIG_LEVEL_BKINDL:
			/* FIXME: should be tenth of dots, newcat expects ms */
			cmdstr = "SD;";
			break;
		case RIG_LEVEL_SQL:
			cmdstr = "SQ0;";
			break;
		case RIG_LEVEL_VOX:
			/* VOX delay, arg int (tenth of seconds), expects ms */
			cmdstr = "VD;";
			break;
		case RIG_LEVEL_VOXGAIN:
			cmdstr = "VG;";
			break;
			/*
			 * Read only levels
			 */
		case RIG_LEVEL_RAWSTR:
			cmdstr = "SM0;";
			break;
		case RIG_LEVEL_SWR:
			cmdstr = "RM6;";
			break;
		case RIG_LEVEL_ALC:
			cmdstr = "RM4;";
			break;
		case RIG_LEVEL_ANTIVOX:
			if (newcat_is_rig(rig, RIG_MODEL_FT950))
				cmdstr = "EX117;";
			else
				return -RIG_EINVAL;
			break;
		default:
			return -RIG_EINVAL;
	}

	err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
	if (err != RIG_OK)
		return err;

	err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
			&cat_term, sizeof(cat_term));
	if (err < 0)
		return err;

	rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
			__func__, err, priv->ret_data);

	ret_data_len = strlen(priv->ret_data);
	if (ret_data_len <= strlen(cmdstr) ||
			priv->ret_data[ret_data_len-1] != ';')
		return -RIG_EPROTO;

	/* skip command */
	retlvl = priv->ret_data + strlen(cmdstr)-1;
	/* chop term */
	priv->ret_data[ret_data_len-1] = '\0';

	switch (level) {
		case RIG_LEVEL_RFPOWER:
		case RIG_LEVEL_VOXGAIN:
		case RIG_LEVEL_COMP:
		case RIG_LEVEL_ANTIVOX:
			scale = (newcat_is_rig(rig, RIG_MODEL_FT950)) ? 100. : 255.;
			val->f = (float)atoi(retlvl)/scale;
			break;
		case RIG_LEVEL_AF:
		case RIG_LEVEL_MICGAIN:
		case RIG_LEVEL_RF:
		case RIG_LEVEL_SQL:
		case RIG_LEVEL_SWR:
		case RIG_LEVEL_ALC:
			val->f = (float)atoi(retlvl)/255.;
			break;
		case RIG_LEVEL_BKINDL: /* FIXME FT950 FT450 works with GUI GRIG2; */
		case RIG_LEVEL_RAWSTR:
		case RIG_LEVEL_KEYSPD:
		case RIG_LEVEL_IF:
			val->i = atoi(retlvl);
			break;
		case RIG_LEVEL_NR:
			if (newcat_is_rig(rig, RIG_MODEL_FT450))
				val->f = (float)(atoi(retlvl)-1)/10.;
			else
				val->f = (float)(atoi(retlvl))/100.;
			break;
		case RIG_LEVEL_VOX:
			if (newcat_is_rig(rig, RIG_MODEL_FT950) || newcat_is_rig(rig, RIG_MODEL_FT450)) {
				val->i = atoi(retlvl);       /* ms, works with GRIG2 */
			} else {
				/* VOX delay, arg int (tenth of seconds) */
				val->i = atoi(retlvl)/100;
			}
			break;
		case RIG_LEVEL_PREAMP:
			if (retlvl[0] < '0' || retlvl[0] > '9')
				return -RIG_EPROTO;
			val->i = (retlvl[0] == '0') ? 0 : state->preamp[retlvl[0]-'1'];
			break;
		case RIG_LEVEL_ATT:
			if (retlvl[0] < '0' || retlvl[0] > '9')
				return -RIG_EPROTO;
			val->i = (retlvl[0] == '0') ? 0 : state->attenuator[retlvl[0]-'1'];
			break;
		case RIG_LEVEL_AGC:
			switch (retlvl[0]) {
				case '0': val->i = RIG_AGC_OFF; break;
				case '1': val->i = RIG_AGC_FAST; break;
				case '2': val->i = RIG_AGC_MEDIUM; break;
				case '3': val->i = RIG_AGC_SLOW; break;
				case '4': 
				case '5':
				case '6':
					  val->i = RIG_AGC_AUTO; break;
				default: return -RIG_EINVAL;
			}
			break;
		case RIG_LEVEL_CWPITCH:
			val->i = (atoi(retlvl)/2)*100+300;
			break;
		case RIG_LEVEL_METER:
			switch (retlvl[0]) {
				case '1': val->i = RIG_METER_ALC; break;
				case '2': val->i = RIG_METER_PO; break;
				case '3': val->i = RIG_METER_SWR; break;
				default: return -RIG_EINVAL;
			}
			break;
		default:
			return -RIG_EINVAL;
    }

    return RIG_OK;
}


int newcat_set_func(RIG * rig, vfo_t vfo, setting_t func, int status)
{
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char cmdstr[16];

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!rig)
		return -RIG_EINVAL;

	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	switch (func) {
		case RIG_FUNC_ANF:
			sprintf(cmdstr, "BC0%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_MN:
			sprintf(cmdstr, "BP00%03d;", status ? 1 : 0);
			break;
		case RIG_FUNC_FBKIN:
			sprintf(cmdstr, "BI%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_TONE:
			sprintf(cmdstr, "CT0%d;", status ? 2 : 0);
			break;
		case RIG_FUNC_TSQL:
			sprintf(cmdstr, "CT0%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_LOCK:
			sprintf(cmdstr, "LK%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_MON:
			sprintf(cmdstr, "ML0%03d;", status ? 1 : 0);
			break;
		case RIG_FUNC_NB:
			sprintf(cmdstr, "NB0%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_NR:
			sprintf(cmdstr, "NR0%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_COMP:
			sprintf(cmdstr, "PR%d;", status ? 1 : 0);
			break;
		case RIG_FUNC_VOX:
			sprintf(cmdstr, "VX%d;", status ? 1 : 0);
			break;
		default:
			return -RIG_EINVAL;
	}

	err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
	if (err != RIG_OK)
		return err;

	return RIG_OK;
}


int newcat_get_func(RIG * rig, vfo_t vfo, setting_t func, int *status)
{
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	int ret_data_len;
	const char *cmdstr;
	char *retfunc;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!rig)
		return -RIG_EINVAL;

	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	switch (func) {
		case RIG_FUNC_ANF:
			cmdstr = "BC0;";
			break;
		case RIG_FUNC_MN:
			cmdstr = "BP00;";
			break;
		case RIG_FUNC_FBKIN:
			cmdstr = "BI;";
			break;
		case RIG_FUNC_TONE:
			cmdstr = "CT0;";
			break;
		case RIG_FUNC_TSQL:
			cmdstr = "CT0;";
			break;
		case RIG_FUNC_LOCK:
			cmdstr = "LK;";
			break;
		case RIG_FUNC_MON:
			cmdstr = "ML0;";
			break;
		case RIG_FUNC_NB:
			cmdstr = "NB0;";
			break;
		case RIG_FUNC_NR:
			cmdstr = "NR0;";
			break;
		case RIG_FUNC_COMP:
			cmdstr = "PR;";
			break;
		case RIG_FUNC_VOX:
			cmdstr = "VX;";
			break;
		default:
			return -RIG_EINVAL;
	}

	err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
	if (err != RIG_OK)
		return err;

	err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
			&cat_term, sizeof(cat_term));
	if (err < 0)
		return err;

	rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
			__func__, err, priv->ret_data);

	ret_data_len = strlen(priv->ret_data);
	if (ret_data_len <= strlen(cmdstr) ||
			priv->ret_data[ret_data_len-1] != ';')
		return -RIG_EPROTO;

	/* skip command */
	retfunc = priv->ret_data + strlen(cmdstr)-1;
	/* chop term */
	priv->ret_data[ret_data_len-1] = '\0';

	switch (func) {
		case RIG_FUNC_MN:
			*status = (retfunc[2] == '0') ? 0 : 1;
			break;
		case RIG_FUNC_ANF:
		case RIG_FUNC_FBKIN:
		case RIG_FUNC_LOCK:
		case RIG_FUNC_MON:
		case RIG_FUNC_NB:
		case RIG_FUNC_NR:
		case RIG_FUNC_COMP:
		case RIG_FUNC_VOX:
			*status = (retfunc[0] == '0') ? 0 : 1;
			break;

		case RIG_FUNC_TONE:
			*status = (retfunc[0] == '2') ? 1 : 0;
			break;
		case RIG_FUNC_TSQL:
			*status = (retfunc[0] == '1') ? 1 : 0;
			break;
		default:
			return -RIG_EINVAL;
	}

	return RIG_OK;
}


int newcat_set_parm(RIG * rig, setting_t parm, value_t val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_parm(RIG * rig, setting_t parm, value_t * val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_ext_level(RIG *rig, vfo_t vfo, token_t token, value_t val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_ext_level(RIG *rig, vfo_t vfo, token_t token, value_t *val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_ext_parm(RIG *rig, token_t token, value_t val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_ext_parm(RIG *rig, token_t token, value_t *val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_conf(RIG * rig, token_t token, const char *val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_conf(RIG * rig, token_t token, char *val)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_send_dtmf(RIG * rig, vfo_t vfo, const char *digits)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_recv_dtmf(RIG * rig, vfo_t vfo, char *digits, int *length)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_send_morse(RIG * rig, vfo_t vfo, const char *msg)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_bank(RIG * rig, vfo_t vfo, int bank)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_mem(RIG * rig, vfo_t vfo, int ch)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    char cmdstr[8];

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;
  
    sprintf(cmdstr, "MC%03d;", ch);

    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
    if (err != RIG_OK)
        return err;

    return RIG_OK;
}


int newcat_get_mem(RIG * rig, vfo_t vfo, int *ch)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
        return -RIG_EINVAL;
  
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    /* Build the command string */
    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "MC;");

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get Memory Channel */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
        return err;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
                      &cat_term, sizeof(cat_term));
    if (err < 0)
        return err;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n",
                  __func__, priv->ret_data);

        return -RIG_EPROTO;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
              __func__, err, priv->ret_data);

    *ch = atoi(priv->ret_data+2);

    return RIG_OK;
}

int newcat_vfo_op(RIG * rig, vfo_t vfo, vfo_op_t op)
{
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char *cmdstr;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!rig)
		return -RIG_EINVAL;


	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	switch (op) {
		case RIG_OP_TUNE:
			cmdstr = "AC002;";
			break;
		case RIG_OP_CPY:
			if (newcat_is_rig(rig, RIG_MODEL_FT450))
				cmdstr = "VV;";
			else
				cmdstr = "AB;"; /* VFO_A to VFO_B */
			break;
		case RIG_OP_XCHG:
			cmdstr = "SV;";
			break;
		case RIG_OP_UP:
			cmdstr = "UP;";
			break;
		case RIG_OP_DOWN:
			cmdstr = "DN;";
			break;
		case RIG_OP_BAND_UP:
			cmdstr = (priv->current_vfo == RIG_VFO_B) ? "BU1;" : "BU0;";
			break;
		case RIG_OP_BAND_DOWN:
			cmdstr = (priv->current_vfo == RIG_VFO_B) ? "BD1;" : "BD0;";
			break;
		case RIG_OP_FROM_VFO:
			/* VFOA ! */
			cmdstr = "AM;";
			break;
		case RIG_OP_TO_VFO:
			/* VFOA ! */
			cmdstr = "MA;";
			break;
		case RIG_OP_TOGGLE:
			cmdstr = "SV;";
			break;
		default:
			return -RIG_EINVAL;
	}

	err = write_block(&state->rigport, cmdstr, strlen(cmdstr));
	if (err != RIG_OK)
		return err;

	return RIG_OK;
}


int newcat_scan(RIG * rig, vfo_t vfo, scan_t scan, int ch)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_trn(RIG * rig, int trn)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_trn(RIG * rig, int *trn)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_decode_event(RIG * rig)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_channel(RIG * rig, const channel_t * chan)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_channel(RIG * rig, channel_t * chan)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


const char *newcat_get_info(RIG * rig)
{
    struct newcat_priv_data *priv;
    struct rig_state *state;
    int err;
    static char idbuf[8];

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
	    return NULL;
  
    priv = (struct newcat_priv_data *)rig->state.priv;
    state = &rig->state;

    /* Build the command string */
    snprintf(priv->cmd_str, sizeof(priv->cmd_str), "ID;");

    rig_debug(RIG_DEBUG_TRACE, "%s: cmd_str = %s\n", __func__, priv->cmd_str);

    /* Get Identification Channel */
    err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
    if (err != RIG_OK)
	    return NULL;

    err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data),
                      &cat_term, sizeof(cat_term));
    if (err < 0)
	    return NULL;

    /* Check that command termination is correct */
    if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
        rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n",
                  __func__, priv->ret_data);

	return NULL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s\n",
              __func__, err, priv->ret_data);

    priv->ret_data[6] = '\0';
    strcpy(idbuf, priv->ret_data);

    return idbuf;
}


#if 0
int newcat_set_chan_all_cb(RIG * rig, chan_cb_t chan_cb, rig_ptr_t)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_chan_all_cb(RIG * rig, chan_cb_t chan_cb, rig_ptr_t)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_set_mem_all_cb(RIG * rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}


int newcat_get_mem_all_cb(RIG * rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t)
{
    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    return -RIG_ENAVAIL;
}
#endif

const char *clone_combo_set;	/*!< String describing key combination to enter load cloning mode */
const char *clone_combo_get;	/*!< String describing key combination to enter save cloning mode */


/*
 * newcat_valid_command
 *
 * Determine whether or not the command is valid for the specified
 * rig.  This function should be called before sending the command
 * to the rig to make it easier to differentiate invalid and illegal
 * commands (for a rig).
 */

ncboolean newcat_valid_command(RIG *rig, char *command) {
    const struct rig_caps *caps;
    ncboolean is_ft450;
    ncboolean is_ft950;
    ncboolean is_ft2000;
    ncboolean is_ft9000;
    int search_high;
    int search_index;
    int search_low;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig) {
        rig_debug(RIG_DEBUG_ERR, "%s: Rig argument is invalid\n", __func__);
        return FALSE;
    }

    caps = rig->caps;
    if (!caps) {
        rig_debug(RIG_DEBUG_ERR, "%s: Rig capabilities not valid\n", __func__);
        return FALSE;
    }

    /*
     * Determine the type of rig from the model number.  Note it is
     * possible for several model variants to exist; i.e., all the
     * FT-9000 variants.
     */

    is_ft450 = strcmp(caps->model_name, "FT-450");
    is_ft950 = strcmp(caps->model_name, "FT-950");
    is_ft2000 = strcmp(caps->model_name, "FT-2000");
    is_ft9000 = strcmp(caps->model_name, "FTDX-9000");
    is_ft9000 = strcmp(caps->model_name, "FTDX-9000 Contest");
    is_ft9000 = strcmp(caps->model_name, "FTDX9000D");
    is_ft9000 = strcmp(caps->model_name, "FTDX9000MP");

    if (!is_ft450 && !is_ft950 && !is_ft2000 && !is_ft9000) {
        rig_debug(RIG_DEBUG_ERR, "%s: '%s' is unknown\n",
                  __func__, caps->model_name);
        return FALSE;
    }

    /*
     * Make sure the command is known, and then check to make sure
     * is it valud for the rig.
     */

    search_low = 0;
    search_high = valid_commands_count;
    while (search_low <= search_high) {
        int search_test;

        search_index = (search_low + search_high) / 2;
        search_test = strcmp (valid_commands[search_index].command, command);
        if (search_test > 0)
            search_high = search_index - 1;
        else if (search_test < 0)
            search_low = search_index + 1;
        else {
            /*
             * The command is valid.  Now make sure it is supported by the rig.
             */
            if (is_ft450 && valid_commands[search_index].ft450)
                return TRUE;
            else if (is_ft950 && valid_commands[search_index].ft950)
                return TRUE;
            else if (is_ft2000 && valid_commands[search_index].ft2000)
                return TRUE;
            else if (is_ft9000 && valid_commands[search_index].ft9000)
                return TRUE;
            else {
                rig_debug(RIG_DEBUG_TRACE, "%s: '%s' command '%s' not supported\n",
                          __func__, caps->model_name, command);
                return FALSE;
                }
        }
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: '%s' command '%s' not valid\n",
              __func__, caps->model_name, command);
    return FALSE;
}

/*
 *  This could change to rig ID from newcat_get_info() 
 *  Need something like model == FT2000
 *  IDa== FT2000 (100W), IDb = FT2000D (200W)
 *  model == FT9000
 *  FT9000DX, FT9000_CONTEST, etc...
 */
ncboolean newcat_is_rig(RIG * rig, rig_model_t model) {
	ncboolean is_rig;

	is_rig = (model == rig->caps->rig_model) ? TRUE : FALSE;

	return is_rig;
}


/*
 * newcat_get_txvfo does not set priv->curr_vfo
 */
int newcat_get_txvfo(RIG * rig, vfo_t * txvfo) {
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char c;
	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", "FT", cat_term);
	/* Get TX VFO */
	err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
	if (err != RIG_OK)
		return err;

	err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
	if (err < 0)
		return err;

	/* Check that command termination is correct */
	if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
		rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

		return -RIG_EPROTO;
	}

	rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, TX_VFO value = %c\n", __func__, err, priv->ret_data, priv->ret_data[2]);

	if (strcmp(priv->ret_data, "?;") == 0) {
		rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting TX_VFO\n");
		return RIG_OK;
	}

	c = priv->ret_data[2];
	switch (c) {
		case '0':
			*txvfo = RIG_VFO_A;
			break;
		case '1' :
			*txvfo = RIG_VFO_B;
			break;
		default:
			return -RIG_EINVAL;
	}

	return RIG_OK;
}


/*
 * newcat_set_txvfo does not set priv->curr_vfo
 */
int newcat_set_txvfo(RIG * rig, vfo_t txvfo) {
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char p1;
	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	err = newcat_set_vfo_from_alias(rig, &txvfo);
	if (err < 0)
		return err;

	switch (txvfo) {
		case RIG_VFO_A:
			p1 = '0';
			break;
		case RIG_VFO_B:
			p1 = '1';
			break;
		default:
			return -RIG_EINVAL;
	}

	if (newcat_is_rig(rig, RIG_MODEL_FT950))
		p1 = p1 + 2;            /* FT950 non-Toggle */

	snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c%c", "FT", p1, cat_term);

	/* Set TX VFO */
	err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
	if (err != RIG_OK)
		return err;

	return RIG_OK;
}

/* 
 * Uses "FR" command
 * Calls newcat_get_vfo() for FT450 rig that does not support "FR" 
 * newcat_get_vfo() Calls newcat_get_rxvfo() for rigs that do not support "VS"
 */
int newcat_get_rxvfo(RIG * rig, vfo_t * rxvfo) {
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char c;
	char command[] = "FR";
	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!newcat_valid_command(rig, command)) {
		if (newcat_is_rig(rig, RIG_MODEL_FT450)) {    /* No "FR" command */
			err = newcat_get_vfo(rig, rxvfo);
			if (err < 0)
				return err;

			return RIG_OK;
		}

		return -RIG_ENAVAIL;
	}

	snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c", command, cat_term);
	/* Get RX VFO */
	err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
	if (err != RIG_OK)
		return err;

	err = read_string(&state->rigport, priv->ret_data, sizeof(priv->ret_data), &cat_term, sizeof(cat_term));
	if (err < 0)
		return err;

	/* Check that command termination is correct */
	if (strchr(&cat_term, priv->ret_data[strlen(priv->ret_data) - 1]) == NULL) {
		rig_debug(RIG_DEBUG_ERR, "%s: Command is not correctly terminated '%s'\n", __func__, priv->ret_data);

		return -RIG_EPROTO;
	}

	rig_debug(RIG_DEBUG_TRACE, "%s: read count = %d, ret_data = %s, RX_VFO value = %c\n", __func__, err, priv->ret_data, priv->ret_data[2]);

	if (strcmp(priv->ret_data, "?;") == 0) {
		rig_debug(RIG_DEBUG_TRACE, "Unrecognized command, getting RX_VFO\n");
		return RIG_OK;
	}

	c = priv->ret_data[2];
	switch (c) {
		case '0':                   /* Main Band VFO_A RX,   Sub Band VFO_B OFF */
		case '1':                   /* Main Band VFO_A Mute, Sub Band VFO_B OFF */
			*rxvfo = RIG_VFO_A;
			break;
		case '2' :                  /* Main Band VFO_A RX,   Sub Band VFO_B RX */
		case '3' :                  /* Main Band VFO_A Mute, Sub Band VFO_B RX */
			*rxvfo = RIG_VFO_A;
			/* FIXME: if (is_rig/is_rigid?(rig, RIG_MODEL_FT9000_CONTEST)) *rxvfo = RIG_VFO_B; */
			break;
		case '4' :                  /* FT950 Main Band VFO_B RX   */
		case '5' :                  /* FT950 Main Band VFO_B Mute */
			*rxvfo = RIG_VFO_B;
			break;
		default:
			return -RIG_EINVAL;
	}

	priv->current_vfo = *rxvfo;     /* Track Main Band RX VFO */

	return RIG_OK;
}

/*
 * Uses "FR" command
 * Calls newcat_set_vfo() for FT450 rig that does not support "FR" 
 * newcat_set_vfo() Calls newcat_set_rxvfo() for rigs that do not support "VS"
 */
int newcat_set_rxvfo(RIG * rig, vfo_t rxvfo) {
	struct newcat_priv_data *priv;
	struct rig_state *state;
	int err;
	char p1;
	char command[] = "FR";
	priv = (struct newcat_priv_data *)rig->state.priv;
	state = &rig->state;

	rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

	if (!newcat_valid_command(rig, command)) {
		if (newcat_is_rig(rig, RIG_MODEL_FT450)) {    /* No "FR" command */
			err = newcat_set_vfo(rig, rxvfo);
			if (err < 0)
				return err;

			return RIG_OK;
		}   

		return -RIG_ENAVAIL;
	}

	err = newcat_set_vfo_from_alias(rig, &rxvfo);
	if (err < 0)
		return err;

	switch (rxvfo) {
		case RIG_VFO_A:
			p1 = '0';
			break;
		case RIG_VFO_B:
			p1 = '2';           /* Main Band VFO_A,   Sub Band VFO_B on certain radios */
			if (newcat_is_rig(rig, RIG_MODEL_FT950))
				p1 = '4';
			break;
		default:
			return -RIG_EINVAL;
	}

	snprintf(priv->cmd_str, sizeof(priv->cmd_str), "%s%c%c", command, p1, cat_term);

	/* Set RX VFO */
	err = write_block(&state->rigport, priv->cmd_str, strlen(priv->cmd_str));
	if (err != RIG_OK)
		return err;

	priv->current_vfo = rxvfo;      /* Track Main Band RX VFO */

	return RIG_OK;
}

int newcat_set_vfo_from_alias(RIG * rig, vfo_t * vfo) {
	struct newcat_priv_data *priv;
	priv = (struct newcat_priv_data *)rig->state.priv;

	rig_debug(RIG_DEBUG_TRACE, "%s: alias vfo = 0x%02x\n", __func__, *vfo);

	switch (*vfo) {
		case RIG_VFO_A:
		case RIG_VFO_B:
		case RIG_VFO_MEM:
			/* passes through */
			break;
		case RIG_VFO_CURR:  /* RIG_VFO_RX == RIG_VFO_CURR */
		case RIG_VFO_VFO:
			*vfo = priv->current_vfo;
			break;
		case RIG_VFO_TX:
			/* set to another vfo for split or uplink */
			*vfo = (priv->current_vfo == RIG_VFO_B) ? RIG_VFO_A : RIG_VFO_B;
			break;
		case RIG_VFO_MAIN:
			*vfo = RIG_VFO_A;
			break;
		case RIG_VFO_SUB:
			*vfo = RIG_VFO_B;
			break;
		default:
			rig_debug(RIG_DEBUG_TRACE, "Unrecognized.  vfo= %d\n", *vfo);
			return -RIG_EINVAL;
	}

	return RIG_OK;
}

/*
 *	Found newcat_set_level() floating point math problem
 *	Using rigctl on FT950 I was trying to set RIG_LEVEL_COMP to 12
 *	I kept setting it to 11.  I wrote some test software and 
 *	found out that 0.12 * 100 = 11 with my setup.
 *  Compilier is gcc 4.2.4, CPU is AMD X2
 *  This works but Find a better way.
 *  The newcat_get_level() seems to work correctly.
 *  Terry KJ4EED
 *
 */
int newcat_scale_float(int scale, float fval) {
	float f;
	float fudge = 0.003;
    
	if ((fval + fudge) > 1.0)
		f = scale * fval;
	else 
		f = scale * (fval + fudge);

	return (int) f;
}
