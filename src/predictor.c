//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
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


void init_predictor()
{
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      init_gshare_predictor();
      break;
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//


// GSHARE PREDICTION
uint8_t make_gshare_prediction(uint32_t pc){
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


uint8_t make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
      break;
    case GSHARE:
      return make_gshare_prediction(pc);
    case TOURNAMENT:
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

//GSHARE TRAINING (Updating pattern table)

void train_gshare_predictor(uint32_t pc, uint8_t outcome){
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

void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      train_gshare_predictor(pc, outcome);
      break;
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}
