/*-------------------------------------------------------------------*
 *                         WB_VAD.C									 *
 *-------------------------------------------------------------------*
 * Voice Activity Detection.										 *
 *-------------------------------------------------------------------*/

/******************************************************************************
*                         INCLUDE FILES
******************************************************************************/
#include <stdlib.h>

#include "typedef.h"
#include "wb_vad_c.h"
#include "wb_vad.h"

#include "basic_op.h"
#include "basicop2.c"


/******************************************************************************
*
*     Function     : filter5
*     Purpose      : Fifth-order half-band lowpass/highpass filter pair with
*                    decimation.
*
*/
static void filter5(
     Word16 * in0,                         /* i/o : input values; output low-pass part  */
     Word16 * in1,                         /* i/o : input values; output high-pass part */
     Word16 data[]                         /* i/o : filter memory                       */
)
{
    Word16 temp0, temp1, temp2;

    temp0 = sub(*in0, mult(COEFF5_1, data[0]));
    temp1 = add(data[0], mult(COEFF5_1, temp0));
    data[0] = temp0;

    temp0 = sub(*in1, mult(COEFF5_2, data[1]));
    temp2 = add(data[1], mult(COEFF5_2, temp0));
    data[1] = temp0;

    *in0 = extract_h(L_shl(L_add(temp1, temp2), 15));
    *in1 = extract_h(L_shl(L_sub(temp1, temp2), 15));
}

/******************************************************************************
*
*     Function     : filter3
*     Purpose      : Third-order half-band lowpass/highpass filter pair with
*                    decimation.
*
*/
static void filter3(
     Word16 * in0,                         /* i/o : input values; output low-pass part  */
     Word16 * in1,                         /* i/o : input values; output high-pass part */
     Word16 * data                         /* i/o : filter memory                       */
)
{
    Word16 temp1, temp2;

    temp1 = sub(*in1, mult(COEFF3, *data));
    temp2 = add(*data, mult(COEFF3, temp1));
    *data = temp1;

    *in1 = extract_h(L_shl(L_sub(*in0, temp2), 15));
    *in0 = extract_h(L_shl(L_add(*in0, temp2), 15));
}

/******************************************************************************
*
*     Function   : level_calculation
*     Purpose    : Calculate signal level in a sub-band. Level is calculated
*                  by summing absolute values of the input data.
*
*                  Signal level calculated from of the end of the frame
*                  (data[count1 - count2]) is stored to (*sub_level)
*                  and added to the level of the next frame.
*
*/
static Word16 level_calculation(           /* return: signal level */
     Word16 data[],                        /* i   : signal buffer                                    */
     Word16 * sub_level,                   /* i   : level calculated at the end of the previous frame*/
										   /* o   : level of signal calculated from the last         */
										   /*       (count2 - count1) samples                        */
     Word16 count1,                        /* i   : number of samples to be counted                  */
     Word16 count2,                        /* i   : number of samples to be counted                  */
     Word16 ind_m,                         /* i   : step size for the index of the data buffer       */
     Word16 ind_a,                         /* i   : starting index of the data buffer                */
     Word16 scale                          /* i   : scaling for the level calculation                */
)
{
    Word32 l_temp1, l_temp2;
    Word16 level, i;

    l_temp1 = 0L;
    for (i = count1; i < count2; i++)
    {
        l_temp1 = L_mac(l_temp1, 1, abs_s(data[ind_m * i + ind_a]));
    }

    l_temp2 = L_add(l_temp1, L_shl(*sub_level, sub(16, scale)));
    *sub_level = extract_h(L_shl(l_temp1, scale));

    for (i = 0; i < count1; i++)
    {
        l_temp2 = L_mac(l_temp2, 1, abs_s(data[ind_m * i + ind_a]));
    }
    level = extract_h(L_shl(l_temp2, scale));

    return level;
}

/******************************************************************************
*
*     Function     : filter_bank
*     Purpose      : Divide input signal into bands and calculate level of
*                    the signal in each band
*
*/
static void filter_bank(
     VadVars * st,                         /* i/o : State struct               */
     Word16 in[],                          /* i   : input frame                */
     Word16 level[]                        /* 0   : signal levels at each band */
)
{
    Word16 i;
    Word16 tmp_buf[FRAME_LEN];

    /* shift input 1 bit down for safe scaling */
    for (i = 0; i < FRAME_LEN; i++)
    {
        tmp_buf[i] = shr(in[i], 1);
    }

    /* run the filter bank */
    for (i = 0; i < FRAME_LEN / 2; i++)
    {
        filter5(&tmp_buf[2 * i], &tmp_buf[2 * i + 1], st->a_data5[0]);
    }
    for (i = 0; i < FRAME_LEN / 4; i++)
    {
        filter5(&tmp_buf[4 * i], &tmp_buf[4 * i + 2], st->a_data5[1]);
        filter5(&tmp_buf[4 * i + 1], &tmp_buf[4 * i + 3], st->a_data5[2]);
    }
    for (i = 0; i < FRAME_LEN / 8; i++)
    {
        filter5(&tmp_buf[8 * i], &tmp_buf[8 * i + 4], st->a_data5[3]);
        filter5(&tmp_buf[8 * i + 2], &tmp_buf[8 * i + 6], st->a_data5[4]);
        filter3(&tmp_buf[8 * i + 3], &tmp_buf[8 * i + 7], &st->a_data3[0]);
    }
    for (i = 0; i < FRAME_LEN / 16; i++)
    {
        filter3(&tmp_buf[16 * i + 0], &tmp_buf[16 * i + 8], &st->a_data3[1]);
        filter3(&tmp_buf[16 * i + 4], &tmp_buf[16 * i + 12], &st->a_data3[2]);
        filter3(&tmp_buf[16 * i + 6], &tmp_buf[16 * i + 14], &st->a_data3[3]);
    }

    for (i = 0; i < FRAME_LEN / 32; i++)
    {
        filter3(&tmp_buf[32 * i + 0], &tmp_buf[32 * i + 16], &st->a_data3[4]);
        filter3(&tmp_buf[32 * i + 8], &tmp_buf[32 * i + 24], &st->a_data3[5]);
    }

    /* calculate levels in each frequency band */

    /* 4800 - 6400 Hz */
    level[11] = level_calculation(tmp_buf, &st->sub_level[11],
        FRAME_LEN / 4 - 48, FRAME_LEN / 4, 4, 1, 14);
    /* 4000 - 4800 Hz */
    level[10] = level_calculation(tmp_buf, &st->sub_level[10],
        FRAME_LEN / 8 - 24, FRAME_LEN / 8, 8, 7, 15);
    /* 3200 - 4000 Hz */
    level[9] = level_calculation(tmp_buf, &st->sub_level[9],
        FRAME_LEN / 8 - 24, FRAME_LEN / 8, 8, 3, 15);
    /* 2400 - 3200 Hz */
    level[8] = level_calculation(tmp_buf, &st->sub_level[8],
        FRAME_LEN / 8 - 24, FRAME_LEN / 8, 8, 2, 15);
    /* 2000 - 2400 Hz */
    level[7] = level_calculation(tmp_buf, &st->sub_level[7],
        FRAME_LEN / 16 - 12, FRAME_LEN / 16, 16, 14, 16);
    /* 1600 - 2000 Hz */
    level[6] = level_calculation(tmp_buf, &st->sub_level[6],
        FRAME_LEN / 16 - 12, FRAME_LEN / 16, 16, 6, 16);
    /* 1200 - 1600 Hz */
    level[5] = level_calculation(tmp_buf, &st->sub_level[5],
        FRAME_LEN / 16 - 12, FRAME_LEN / 16, 16, 4, 16);
    /* 800 - 1200 Hz */
    level[4] = level_calculation(tmp_buf, &st->sub_level[4],
        FRAME_LEN / 16 - 12, FRAME_LEN / 16, 16, 12, 16);
    /* 600 - 800 Hz */
    level[3] = level_calculation(tmp_buf, &st->sub_level[3],
        FRAME_LEN / 32 - 6, FRAME_LEN / 32, 32, 8, 17);
    /* 400 - 600 Hz */
    level[2] = level_calculation(tmp_buf, &st->sub_level[2],
        FRAME_LEN / 32 - 6, FRAME_LEN / 32, 32, 24, 17);
    /* 200 - 400 Hz */
    level[1] = level_calculation(tmp_buf, &st->sub_level[1],
        FRAME_LEN / 32 - 6, FRAME_LEN / 32, 32, 16, 17);
    /* 0 - 200 Hz */
    level[0] = level_calculation(tmp_buf, &st->sub_level[0],
        FRAME_LEN / 32 - 6, FRAME_LEN / 32, 32, 0, 17);
}

/******************************************************************************
*
*     Function   : update_cntrl
*     Purpose    : Control update of the background noise estimate.
*
*/
static void update_cntrl(
     VadVars * st,                         /* i/o : State structure                    */
     Word16 level[]                        /* i   : sub-band levels of the input frame */
)
{
    Word16 i, temp, stat_rat, exp;
    Word16 num, denom;
    Word16 alpha;

    /* if a tone has been detected for a while, initialize stat_count */

    if (sub((Word16) (st->tone_flag & 0x7c00), 0x7c00) == 0)
    {
        st->stat_count = STAT_COUNT;
    } else
    {
        /* if 8 last vad-decisions have been "0", reinitialize stat_count */

        if ((st->vadreg & 0x7f80) == 0)
        {
            st->stat_count = STAT_COUNT;
        } else
        {
            stat_rat = 0;
            for (i = 0; i < COMPLEN; i++)
            {

                if (sub(level[i], st->ave_level[i]) > 0)
                {
                    num = level[i];
                    denom = st->ave_level[i];
                } else
                {
                    num = st->ave_level[i];
                    denom = level[i];
                }
                /* Limit nimimum value of num and denom to STAT_THR_LEVEL */

                if (sub(num, STAT_THR_LEVEL) < 0)
                {
                    num = STAT_THR_LEVEL;
                }

                if (sub(denom, STAT_THR_LEVEL) < 0)
                {
                    denom = STAT_THR_LEVEL;
                }
                exp = norm_s(denom);
                denom = shl(denom, exp);

                /* stat_rat = num/denom * 64 */
                temp = div_s(shr(num, 1), denom);
                stat_rat = add(stat_rat, shr(temp, sub(8, exp)));
            }

            /* compare stat_rat with a threshold and update stat_count */

            if (sub(stat_rat, STAT_THR) > 0)
            {
                st->stat_count = STAT_COUNT;
            } else
            {

                if ((st->vadreg & 0x4000) != 0)
                {

                    if (st->stat_count != 0)
                    {
                        st->stat_count = sub(st->stat_count, 1);
                    }
                }
            }
        }
    }

    /* Update average amplitude estimate for stationarity estimation */
    alpha = ALPHA4;

    if (sub(st->stat_count, STAT_COUNT) == 0)
    {
        alpha = 32767;
    } else if ((st->vadreg & 0x4000) == 0)
    {

        alpha = ALPHA5;
    }
    for (i = 0; i < COMPLEN; i++)
    {
        st->ave_level[i] = add(st->ave_level[i],
            mult_r(alpha, sub(level[i], st->ave_level[i])));
    }
}

/******************************************************************************
*
*     Function   : noise_estimate_update
*     Purpose    : Update of background noise estimate
*
*/

static void noise_estimate_update(
     VadVars * st,                         /* i/o : State structure                       */
     Word16 level[]                        /* i   : sub-band levels of the input frame */
)
{
    Word16 i, alpha_up, alpha_down, bckr_add;

    /* Control update of bckr_est[] */
    update_cntrl(st, level);

    /* Reason for using bckr_add is to avoid problems caused by fixed-point dynamics when noise level and
     * required change is very small. */
    bckr_add = 2;
    /* Choose update speed */

    if ((0x7800 & st->vadreg) == 0)
    {
        alpha_up = ALPHA_UP1;
        alpha_down = ALPHA_DOWN1;
    } else
    {

        if ((st->stat_count == 0))
        {
            alpha_up = ALPHA_UP2;
            alpha_down = ALPHA_DOWN2;
        } else
        {
            alpha_up = 0;
            alpha_down = ALPHA3;
            bckr_add = 0;
        }
    }

    /* Update noise estimate (bckr_est) */
    for (i = 0; i < COMPLEN; i++)
    {
        Word16 temp;

        temp = sub(st->old_level[i], st->bckr_est[i]);


        if (temp < 0)
        {                                  /* update downwards */
            st->bckr_est[i] = add(-2, add(st->bckr_est[i],
                    mult_r(alpha_down, temp)));

            /* limit minimum value of the noise estimate to NOISE_MIN */

            if (sub(st->bckr_est[i], NOISE_MIN) < 0)
            {
                st->bckr_est[i] = NOISE_MIN;
            }
        } else
        {                                  /* update upwards */
            st->bckr_est[i] = add(bckr_add, add(st->bckr_est[i],
                    mult_r(alpha_up, temp)));

            /* limit maximum value of the noise estimate to NOISE_MAX */

            if (sub(st->bckr_est[i], NOISE_MAX) > 0)
            {
                st->bckr_est[i] = NOISE_MAX;
            }
        }
    }

    /* Update signal levels of the previous frame (old_level) */
    for (i = 0; i < COMPLEN; i++)
    {
        st->old_level[i] = level[i];
    }
}

/******************************************************************************
*
*     Function     : vad_decision
*     Purpose      : Calculates VAD_flag
*
*/

static Word16 vad_decision(                /* return value : VAD_flag */
     VadVars * st,                         /* i/o : State structure                       */
     Word16 level[COMPLEN],                /* i   : sub-band levels of the input frame */
     Word32 pow_sum                        /* i   : power of the input frame           */
)
{
    Word16 i;
    Word32 L_snr_sum;
    Word32 L_temp;
    Word16 vad_thr, temp, noise_level;
    Word16 low_power_flag;
    Word16 hang_len, burst_len;
    Word16 ilog2_speech_level, ilog2_noise_level;
    Word16 temp2;

    /* Calculate squared sum of the input levels (level) divided by the background noise components
     * (bckr_est). */
    L_snr_sum = 0;
    for (i = 0; i < COMPLEN; i++)
    {
        Word16 exp;

        exp = norm_s(st->bckr_est[i]);
        temp = shl(st->bckr_est[i], exp);
        temp = div_s(shr(level[i], 1), temp);
        temp = shl(temp, sub(exp, UNIRSHFT - 1));
        L_snr_sum = L_mac(L_snr_sum, temp, temp);
    }

    /* Calculate average level of estimated background noise */
    L_temp = 0;
    for (i = 1; i < COMPLEN; i++)          /* ignore lowest band */
    {
        L_temp = L_add(L_temp, st->bckr_est[i]);
    }

    noise_level = extract_h(L_shl(L_temp, 12));
    /* if SNR is lower than a threshold (MIN_SPEECH_SNR), and increase speech_level */
    temp = shl(mult(noise_level, MIN_SPEECH_SNR), 3);


    if (sub(st->speech_level, temp) < 0)
    {
        st->speech_level = temp;
    }
    ilog2_noise_level = ilog2(noise_level);

    /* If SNR is very poor, speech_level is probably corrupted by noise level. This is correctred by
     * subtracting MIN_SPEECH_SNR*noise_level from speech level */
    ilog2_speech_level = ilog2(sub(st->speech_level, temp));

    temp = add(mult(NO_SLOPE, sub(ilog2_noise_level, NO_P1)), THR_HIGH);

    temp2 = add(SP_CH_MIN, mult(SP_SLOPE, sub(ilog2_speech_level, SP_P1)));

    if (sub(temp2, SP_CH_MIN) < 0)
    {
        temp2 = SP_CH_MIN;
    }

    if (sub(temp2, SP_CH_MAX) > 0)
    {
        temp2 = SP_CH_MAX;
    }
    vad_thr = add(temp, temp2);

    if (sub(vad_thr, THR_MIN) < 0)
    {
        vad_thr = THR_MIN;
    }
    /* Shift VAD decision register */
    st->vadreg = shr(st->vadreg, 1);

    /* Make intermediate VAD decision */

    if (L_sub(L_snr_sum, L_mult(vad_thr, 512 * COMPLEN)) > 0)
    {
        st->vadreg = (Word16) (st->vadreg | 0x4000);
    }

    /* check if the input power (pow_sum) is lower than a threshold" */

    if (L_sub(pow_sum, VAD_POW_LOW) < 0)
    {
        low_power_flag = 1;
    } else
    {
        low_power_flag = 0;
    }
    /* Update background noise estimates */
    noise_estimate_update(st, level);

    /* Calculate values for hang_len and burst_len based on vad_thr */
    hang_len = add(mult(HANG_SLOPE, sub(vad_thr, HANG_P1)), HANG_HIGH);

    if (sub(hang_len, HANG_LOW) < 0)
    {
        hang_len = HANG_LOW;
    };

    burst_len = add(mult(BURST_SLOPE, sub(vad_thr, BURST_P1)), BURST_HIGH);

    //return (hangover_addition(st, low_power_flag, hang_len, burst_len));
    return (st->vadreg);
}



/******************************************************************************
*
*  Function:   wb_vad_reset
*  Purpose:    Initializes state memory
*
*/

Word16 wb_vad_reset(                       /* return: non-zero with error, zero for ok. */
     VadVars * state                       /* i/o : State structure    */
)
{
    Word16 i, j;

    if (state == (VadVars *) NULL)
    {
        //fprintf(stderr, "vad_reset: invalid parameter\n");
        return -1;
    }
    state->tone_flag = 0;
    state->vadreg = 0;
    state->hang_count = 0;
    state->burst_count = 0;
    state->hang_count = 0;

    /* initialize memory used by the filter bank */
    for (i = 0; i < F_5TH_CNT; i++)
    {
        for (j = 0; j < 2; j++)
        {
            state->a_data5[i][j] = 0;
        }
    }

    for (i = 0; i < F_3TH_CNT; i++)
    {
        state->a_data3[i] = 0;
    }

    /* initialize the rest of the memory */
    for (i = 0; i < COMPLEN; i++)
    {
        state->bckr_est[i] = NOISE_INIT;
        state->old_level[i] = NOISE_INIT;
        state->ave_level[i] = NOISE_INIT;
        state->sub_level[i] = 0;
    }

    state->sp_est_cnt = 0;
    state->sp_max = 0;
    state->sp_max_cnt = 0;
    state->speech_level = SPEECH_LEVEL_INIT;
    state->prev_pow_sum = 0;
    return 0;
}
/******************************************************************************
*
*     Estimate_Speech()
*     Purpose      : Estimate speech level
*
* Maximum signal level is searched and stored to the variable sp_max.
* The speech frames must locate within SP_EST_COUNT number of frames.
* Thus, noisy frames having occasional VAD = "1" decisions will not
* affect to the estimated speech_level.
*
*/
static void Estimate_Speech(
     VadVars * st,                         /* i/o : State structure    */
     Word16 in_level                       /* level of the input frame */
)
{
    Word16 alpha;

    /* if the required activity count cannot be achieved, reset counters */

    /* if (SP_ACTIVITY_COUNT  > SP_EST_COUNT - st->sp_est_cnt + st->sp_max_cnt) */
    if (sub(sub(st->sp_est_cnt, st->sp_max_cnt), SP_EST_COUNT - SP_ACTIVITY_COUNT) > 0)
    {
        st->sp_est_cnt = 0;
        st->sp_max = 0;
        st->sp_max_cnt = 0;
    }
    st->sp_est_cnt = add(st->sp_est_cnt, 1);


    if (((st->vadreg & 0x4000) || (sub(in_level, st->speech_level) > 0))
        && (sub(in_level, MIN_SPEECH_LEVEL1) > 0))
    {
        /* update sp_max */

        if (sub(in_level, st->sp_max) > 0)
        {
            st->sp_max = in_level;
        }
        st->sp_max_cnt = add(st->sp_max_cnt, 1);

        if (sub(st->sp_max_cnt, SP_ACTIVITY_COUNT) >= 0)
        {
            Word16 tmp;

            /* update speech estimate */
            tmp = shr(st->sp_max, 1);      /* scale to get "average" speech level */

            /* select update speed */

            if (sub(tmp, st->speech_level) > 0)
            {
                alpha = ALPHA_SP_UP;
            } else
            {
                alpha = ALPHA_SP_DOWN;
            }

            if (sub(tmp, MIN_SPEECH_LEVEL2) > 0)
            {
                st->speech_level = add(st->speech_level,
                    mult_r(alpha, sub(tmp, st->speech_level)));
            }
            /* clear all counters used for speech estimation */
            st->sp_max = 0;
            st->sp_max_cnt = 0;
            st->sp_est_cnt = 0;
        }
    }
}

/******************************************************************************
*
*     Function     : wb_vad
*     Purpose      : Main program for Voice Activity Detection (VAD) for AMR
*
*/

Word16 wb_vad(                             /* Return value : VAD Decision, 1 = speech, 0 = noise */
     VadVars * st,                         /* i/o : State structure                 */
     Word16 in_buf[]                       /* i   : samples of the input frame   */
)
{
    Word16 level[COMPLEN];
    Word16 i;
    Word16 VAD_flag, temp;
    Word32 L_temp, pow_sum;

    L_temp = 0L;
    for (i = 0; i < FRAME_LEN; i++)
    {
        L_temp = L_mac(L_temp, in_buf[i], in_buf[i]);
    }

    pow_sum = L_add(L_temp, st->prev_pow_sum);

    st->prev_pow_sum = L_temp;


    if (L_sub(pow_sum, POW_TONE_THR) < 0)
    {
        st->tone_flag = (Word16) (st->tone_flag & 0x1fff);
    }
    filter_bank(st, in_buf, level);





    VAD_flag = vad_decision(st, level, pow_sum);


    L_temp = 0;
    for (i = 1; i < COMPLEN; i++)
    {
        L_temp = L_add(L_temp, level[i]);
    }

    temp = extract_h(L_shl(L_temp, 12));

    Estimate_Speech(st, temp);
    return (VAD_flag);
}
