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

// Common variables and functions for all predictors

uint64_t bhr; // global history register
uint64_t pcIndexLength;
uint64_t lhistoryLength; // pattern count
uint64_t ghistoryLength;

void init_common_predictor()
{
  bhr = 0;
  pcIndexLength = 1 << pcIndexBits;
  lhistoryLength = 1 << lhistoryBits;
  ghistoryLength = 1 << ghistoryBits;
}

void update_global_history(uint8_t outcome)
{
  bhr = (bhr << 1) | outcome;
  bhr = bhr & ((1 << ghistoryBits) - 1);
}

// --------- GSHARE PREDICTOR ---------

// GSHARE INITIALIZATION
// pointer to pattern table
uint8_t *gshare_pattern_table;
uint64_t bhr;
uint32_t pattern_count;

void init_gshare_predictor()
{
  // initialize global history
  // bhr = 0;

  // number of patterns = 2^m
  pattern_count = 1 << ghistoryBits;
  printf("---GSHARE:%d---\n", ghistoryBits);
  // allocate space for pattern table
  gshare_pattern_table = malloc(pattern_count * sizeof(uint8_t));
  // initalize pattern table to "weak not taken" for all patterns
  for (int i = 0; i < pattern_count; i++)
    gshare_pattern_table[i] = WN;
}

uint8_t make_gshare_prediction(uint32_t pc)
{
  // pattern = global history XOR pc
  uint32_t pattern = (bhr ^ pc);
  // consider only the last m bits
  pattern = pattern & (pattern_count - 1);
  // get prediction from pattern table
  uint8_t prediction = gshare_pattern_table[pattern];
  // return prediction
  if (prediction == WN || prediction == SN)
    return NOTTAKEN;
  return TAKEN;
}

// GSHARE TRAINING (Updating pattern table)
void train_gshare_predictor(uint32_t pc, uint8_t outcome)
{
  // pattern = global history XOR pc
  uint32_t pattern = (bhr ^ pc);
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
  /*   bhr = bhr << 1;
    // consider only the last m bits
    bhr = bhr & (pattern_count - 1);
    // append outcome to global history
    bhr = bhr | outcome; */
}

// --------- LOCAL PREDICTOR ---------

// Local history initialization
uint32_t *local_history_table;
uint8_t *local_pattern_table;

void init_local_predictor()
{
  local_history_table = malloc((pcIndexLength) * sizeof(uint32_t));
  memset(local_history_table, 0, (pcIndexLength) * sizeof(uint32_t)); // lhistory assumed to be < 32 bits

  local_pattern_table = malloc((lhistoryLength) * sizeof(uint8_t));
  memset(local_pattern_table, WN, (lhistoryLength) * sizeof(uint8_t));
}

// Local PREDICTION
uint8_t make_local_prediction(uint32_t pc)
{
  uint32_t pcIndex = pc & ((pcIndexLength)-1); // 111... of pcIndexBits
  uint32_t local_pattern = local_history_table[pcIndex];
  return (local_pattern_table[local_pattern] >= 2) ? TAKEN : NOTTAKEN;
}

void train_local_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t pcIndex = pc & ((pcIndexLength)-1);
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

// --------- GLOBAL PREDICTOR ---------

// global history initialization
uint8_t *global_history_table;

void init_global_predictor()
{
  // will have direct map from global history to counter
  global_history_table = malloc((ghistoryLength) * sizeof(uint8_t));
  memset(global_history_table, WN, (ghistoryLength) * sizeof(uint8_t));
}

// global history prediction
uint8_t make_global_prediction(uint32_t pc)
{
  uint8_t global_pattern = global_history_table[bhr];
  return (global_pattern >= 2) ? TAKEN : NOTTAKEN;
}

// global history training
void train_global_predictor(uint32_t pc, uint8_t outcome)
{

  if (outcome == TAKEN)
  {
    if (global_history_table[bhr] < 3)
    {
      global_history_table[bhr]++;
    }
  }
  else
  {
    if (global_history_table[bhr] > 0)
    {
      global_history_table[bhr]--;
    }
  }
}

// --------- TOURNAMENT PREDICTOR ---------

// tournament predictor initialization
uint8_t *choice_table;

void init_tournament_predictor()
{
  init_global_predictor();
  init_local_predictor();
  printf("---Tournament:%d%d%d---\n", ghistoryBits, lhistoryBits, pcIndexBits);

  // SS: "'ghistoryBits' will be used to size the global and choice predictors"
  // is present in the README, but I think it should be PC index bits
  // I think in the original paper PC index choice predictor is used
  choice_table = malloc((ghistoryLength) * sizeof(uint8_t));
  memset(choice_table, 2, (ghistoryLength) * sizeof(uint8_t)); // weakly prefer global predictor
}

// hybrid (tournament) prediction
uint8_t make_tournament_prediction(uint32_t pc)
{
  uint8_t global_prediction = make_global_prediction(pc);
  uint8_t local_prediction = make_local_prediction(pc);
  uint8_t choice = choice_table[bhr];
  return (choice >= 2) ? global_prediction : local_prediction;
}

// hybrid (tournament) training
void train_tournament_predictor(uint32_t pc, uint8_t outcome)
{
  uint8_t global_prediction = make_global_prediction(pc);
  uint8_t local_prediction = make_local_prediction(pc);

  if (global_prediction != local_prediction)
  {
    if (global_prediction == outcome)
    {
      if (choice_table[bhr] < 3)
      {
        choice_table[bhr]++;
      }
    }
    else
    {
      if (choice_table[bhr] > 0)
      {
        choice_table[bhr]--;
      }
    }
  }

  train_global_predictor(pc, outcome);
  train_local_predictor(pc, outcome);
}

// --------- CUSTOM PREDICTOR ---------
// perceptron predictor initialization

int **perceptronWeights; // 2D array to store perceptron weights
// one perceptron for a fixed number of pc bits
// each perceptron has a weight for each bit of the global history

void init_perceptron_predictor()
{
  // Calculate the number of perceptrons based on the BHR length
  // Allocate memory for perceptron weights
  perceptronWeights = (int **)malloc(pcIndexLength * sizeof(int *));
  for (int i = 0; i < pcIndexLength; i++)
  {
    // + 1 for the bias weight
    perceptronWeights[i] = (int *)malloc((ghistoryBits + 1) * sizeof(int));

    // Initialize weights to 0
    memset(perceptronWeights[i], 0, (ghistoryBits + 1) * sizeof(int));
  }
}

// Make a prediction for a branch instruction
uint8_t make_perceptron_prediction(uint32_t pc)
{
  // Get the perceptron index based on the lower bits of the PC
  int pcIndex = pc & (pcIndexLength - 1);

  // Calculate the perceptron output
  int output = 0;
  for (int i = 0; i < ghistoryBits; i++)
  {
    // Get the corresponding bit from the BHR
    int bit = (bhr >> i) & 1;
    // Multiply the bit with the corresponding weight and add to the output
    // output += bit * perceptronWeights[pcIndex][i];
    // replace with if else for better performance
    if (bit == 1)
    {
      output += perceptronWeights[pcIndex][i];
    }
    else
    {
      output -= perceptronWeights[pcIndex][i];
    }
  }
  // Add the bias weight to the output
  output += perceptronWeights[pcIndex][ghistoryBits];

  // Make the prediction based on the output and threshold
  return (output > 0) ? TAKEN : NOTTAKEN; // equivalent of Weakly not taken
}

// Train the perceptron predictor
void train_perceptron_predictor(uint32_t pc, uint8_t outcome)
{
  // Get the perceptron index based on the lower bits of the PC
  int pcIndex = pc & (pcIndexLength - 1);
  int max = 128;
  // Update the perceptron weights
  for (int i = 0; i < ghistoryBits; i++)
  {
    // Get the corresponding bit from the BHR
    int bit = (bhr >> i) & 1;
    // Update the weight based on the outcome with saturation checks

    bit = 2 * bit - 1;

    if (outcome == TAKEN)
    {
      if (perceptronWeights[pcIndex][i] < max)
      {
        perceptronWeights[pcIndex][i] += bit;
      }
    }
    else
    {
      if (perceptronWeights[pcIndex][i] > -max)
      {
        perceptronWeights[pcIndex][i] -= bit;
      }
    }
  }
  // Update the bias weight with saturation checks
  if (outcome == TAKEN)
  {
    if (perceptronWeights[pcIndex][ghistoryBits] < max)
    {
      perceptronWeights[pcIndex][ghistoryBits] += 1;
    }
  }
  else
  {
    if (perceptronWeights[pcIndex][ghistoryBits] > -max)
    {
      perceptronWeights[pcIndex][ghistoryBits] -= 1;
    }
  }
}

// ----------------------------------------------

void init_predictor()
{
  init_common_predictor();

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
    init_perceptron_predictor();
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc)
{

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
    return make_perceptron_prediction(pc);
    break;
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
    train_perceptron_predictor(pc, outcome);
    break;
  default:
    break;
  }

  update_global_history(outcome);

  return;
}
