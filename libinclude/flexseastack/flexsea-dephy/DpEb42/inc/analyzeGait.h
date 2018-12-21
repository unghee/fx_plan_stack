/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-dephy' Exo Controllers
	Copyright (C) 2018 Dephy, Inc. <http://dephy.com/>
*****************************************************************************
	[Lead developer] Luke Mooney, lmooney at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] analyze-Gait: gait parameters
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2018-02-06 | jfduval | Copied from DpEb31
****************************************************************************/

#ifndef INC_ANALYZE_GAIT_H
#define INC_ANALYZE_GAIT_H

//****************************************************************************
// Include(s)
//****************************************************************************

#ifndef BOARD_TYPE_FLEXSEA_PLAN

#include "main.h"
#include "flexsea_global_structs.h"

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void init_ag2(void);
void ag2_fsm(void);
void calc_ppt_prox(void);
void set_gait_type(int8_t);
	
//****************************************************************************
// Definition(s):

#define NUMCONTGS	20 //must be greater than pre + post + 1
#define NUMSTEPSTAT	30 //Number of steps that can be stored in a single instance of stepstat
//****************************************************************************

//****************************************************************************
// Structure(s)

#define NUMGAITSTATFLOAT	13
//update the pound define above when new variables are added
struct gaitstat_s
{
	float t_step; //time of step this variable needs to be first
	float t_swg; //time length of stance
	float t_stn; //time length of swing
	float t_cpf; //time to control PF
	float t_ppf; //time of powered PF

	float a_cpf_max; //control PF max angle
	float a_ppf_max; //max powered PF angle
	float a_df_min;//minimum DF angle during swing
	float a_swg_min; //minimum ankle angle during swing
	float a_swg_init; //initial ankle angle during swing

	float s_min; //minimum shank angle
	float s_swg_max_vel; //maximum velocity during swing
	float s_at_ppf; // shank angle at max ppf
};

struct contstep_s
{
	struct gaitstat_s x[NUMCONTGS];
	struct gaitstat_s last_comp_x;
	int32_t cont;
	int8_t i;
	int8_t pre;
	int8_t post;
};

struct stepstat_s
{
	struct gaitstat_s x[NUMSTEPSTAT];
	struct gaitstat_s mean;
	struct gaitstat_s std;
	struct gaitstat_s s;
	int32_t N;
	int8_t i;
	int8_t full;
};

void init_gaitstat_s(struct gaitstat_s *);
void init_contstep_s(struct contstep_s *,int8_t, int8_t);
void init_stepstat_s(struct stepstat_s *);

void add_contstep_s(struct gaitstat_s *, struct contstep_s *);
void add_stepstat_s(struct contstep_s *, struct stepstat_s *);
void reset_contstep_s(struct contstep_s *);
void update_stepstat_stats(struct stepstat_s *);

float fast_sqrt(const float);
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************
extern int32_t ank_ang_deg;
extern int32_t ag_step_t;
extern struct filt_float_s is_it_ppf;
extern float shk_ang_deg;
extern uint8_t we_are_in_stance;

extern struct gaitstat_s gs0;
extern struct contstep_s cont_steps_0;
extern struct stepstat_s step_stats_0;

#endif //BOARD_TYPE_FLEXSEA_PLAN

#endif //INC_ANALYZE_GAIT_H
