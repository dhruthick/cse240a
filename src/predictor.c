//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h> // for memset
#include "predictor.h"


//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID = "PID";
const char *email = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// GSHARE INITIALIZATION
// pointer to pattern table
uint8_t *gshare_pattern_table;
uint64_t global_history;
uint32_t pattern_count;

void init_gshare_predictor()
{
  // initialize global history
  global_history = 0;
  // number of patterns = 2^m
  pattern_count = 1 << ghistoryBits;
  // allocate space for pattern table
  gshare_pattern_table = malloc(pattern_count * sizeof(uint8_t));
  // initalize pattern table to "weak not taken" for all patterns
  for (int i = 0; i < pattern_count; i++)
    gshare_pattern_table[i] = WN;
}

// Local history initialization
uint32_t *local_history_table;
uint8_t *local_pattern_table;

uint64_t pcIndexLength;
uint64_t lhistoryLength;

void init_local_predictor()
{
  pcIndexLength = 1 << pcIndexBits;
  lhistoryLength = 1 << lhistoryBits;
  local_history_table = malloc((pcIndexLength) * sizeof(uint32_t));
  memset(local_history_table, 0, (pcIndexLength) * sizeof(uint32_t)); // lhistory assumed to be < 32 bits

  local_pattern_table = malloc((lhistoryLength) * sizeof(uint8_t));
  memset(local_pattern_table, WN, (lhistoryLength) * sizeof(uint8_t));
}

// tournament predictor initialization
uint8_t *choice_table;

void init_tournament_predictor()
{
  init_gshare_predictor();
  init_local_predictor();

  choice_table = malloc((pcIndexLength) * sizeof(uint8_t));
  memset(choice_table, 2, (pcIndexLength) * sizeof(uint8_t)); // weakly prefer global predictor
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare_predictor();
    break;
  case TOURNAMENT:
    init_tournament_predictor();
    break;
  case CUSTOM:
  default:
    break;
  }
}

// GSHARE PREDICTION
uint8_t make_gshare_prediction(uint32_t pc)
{
  // pattern = global history XOR pc
  uint8_t pattern = (global_history ^ pc);
  // consider only the last m bits
  pattern = pattern & (pattern_count - 1);
  // get prediction from pattern table
  uint8_t prediction = gshare_pattern_table[pattern];
  // return prediction
  if (prediction == WN || prediction == SN)
    return NOTTAKEN;
  return TAKEN;
}

uint32_t pcIndex;

// Local (GSELECT) PREDICTION
uint8_t make_local_prediction(uint32_t pc)
{
  pcIndex = pc & ((pcIndexLength)-1); // 111... of pcIndexBits
  uint32_t local_pattern = local_history_table[pcIndex];
  return (local_pattern_table[local_pattern] >= 2) ? TAKEN : NOTTAKEN;
}

// hybrid (tournament) prediction
uint8_t make_tournament_prediction(uint32_t pc)
{
  uint8_t global_prediction = make_gshare_prediction(pc);
  uint8_t local_prediction = make_local_prediction(pc);
  uint8_t choice = choice_table[pcIndex];
  return (choice >= 2) ? global_prediction : local_prediction;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc)
{
  //
  // TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
    break;
  case GSHARE:
    return make_gshare_prediction(pc);
    break;
  case TOURNAMENT:
    return make_tournament_prediction(pc);
    break;
  case CUSTOM:
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

// GSHARE TRAINING (Updating pattern table)
void train_gshare_predictor(uint32_t pc, uint8_t outcome)
{
  // pattern = global history XOR pc
  uint8_t pattern = (global_history ^ pc);
  // consider only the last m bits
  pattern = pattern & (pattern_count - 1);
  // get prediction from pattern table
  uint8_t prediction = gshare_pattern_table[pattern];
  // update pattern table based on outcome
  if (prediction == ST)
    gshare_pattern_table[pattern] = (outcome == TAKEN) ? ST : WT;
  if (prediction == WT)
    gshare_pattern_table[pattern] = (outcome == TAKEN) ? ST : WN;
  if (prediction == WN)
    gshare_pattern_table[pattern] = (outcome == NOTTAKEN) ? SN : WT;
  if (prediction == SN)
    gshare_pattern_table[pattern] = (outcome == NOTTAKEN) ? SN : WN;
  // update global history
  // shift left by 1
  global_history = global_history << 1;
  // consider only the last m bits
  global_history = global_history & (pattern_count - 1);
  // append outcome to global history
  global_history = global_history | outcome;
}

void train_local_predictor(uint32_t pc, uint8_t outcome)
{
  pcIndex = pc & ((pcIndexLength)-1);
  uint32_t history_pattern = local_history_table[pcIndex];

  local_history_table[pcIndex] =
      ((history_pattern << 1) | outcome) & ((lhistoryLength)-1); // 111... of lhistoryBits

  if (outcome == TAKEN)
  {
    if (local_pattern_table[history_pattern] < 3)
    {
      local_pattern_table[history_pattern]++;
    }
  }
  else
  {
    if (local_pattern_table[history_pattern] > 0)
    {
      local_pattern_table[history_pattern]--;
    }
  }
}

// hybrid (tournament) training

void train_tournament_predictor(uint32_t pc, uint8_t outcome)
{
  // uint32_t pcIndex = pc & ((1 << pcIndexBits) - 1);
  // pcIndex will get updated in train_local_predictor
  train_gshare_predictor(pc, outcome);
  train_local_predictor(pc, outcome);

  uint8_t global_prediction = make_gshare_prediction(pc);
  uint8_t local_prediction = make_local_prediction(pc);
  if (global_prediction != local_prediction)
  {
    if (global_prediction == outcome)
    {
      if (choice_table[pcIndex] < 3)
      {
        choice_table[pcIndex]++;
      }
    }
    else
    {
      if (choice_table[pcIndex] > 0)
      {
        choice_table[pcIndex]--;
      }
    }
  }
}

void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  // TODO: Implement Predictor training
  //
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    train_gshare_predictor(pc, outcome);
    break;
  case TOURNAMENT:
    train_tournament_predictor(pc, outcome);
    break;
  case CUSTOM:
  default:
    break;
  }
}
