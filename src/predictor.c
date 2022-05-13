//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Harsh Gondaliya";
const char *studentID   = "A59001613";
const char *email       = "hgondali@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here. 
int ghistoryBits = 14; // Number of bits used for Global History
int bpType;       // Branch Prediction Type
int verbose;

//tournament configuration
int tournament_local_pattern_bits = 10;
int tournament_global_pattern_bits = 12;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
uint8_t *bht_gshare;
uint64_t ghistory;
//tournament
uint8_t *bht_local_tournament;
uint8_t *bht_global_tournament;
uint8_t *bht_chooser_tournament;
uint32_t *table_local_pattern_tournament;
uint64_t tournament_ghistory;
//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
 int bht_entries = 1 << ghistoryBits; // 2^14
  bht_gshare = (uint8_t*)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_entries; i++){
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

//tournament functions
void init_tournament() {
 int local_pattern_entries = 1 << tournament_local_pattern_bits;
 int local_bht_entries = 1 << tournament_local_pattern_bits;

 int global_pattern_entries = 1 << tournament_global_pattern_bits;
 int global_bht_entries = 1 << tournament_global_pattern_bits;
 int chooser_entries = 1 << tournament_global_pattern_bits;

 table_local_pattern_tournament = (uint32_t*)malloc(local_pattern_entries * sizeof(uint32_t));
 bht_local_tournament = (uint8_t*)malloc(local_bht_entries * sizeof(uint8_t));

 bht_global_tournament = (uint8_t*)malloc(global_bht_entries * sizeof(uint8_t));
 bht_chooser_tournament = (uint8_t*)malloc(chooser_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< local_pattern_entries; i++){
    table_local_pattern_tournament[i] = 0;
  }
  for(i = 0; i< local_bht_entries; i++){
    bht_local_tournament[i] = WN;
  }

  for(i = 0; i< global_pattern_entries; i++){
    bht_global_tournament[i] = WN;
  }
  for(i = 0; i< global_bht_entries; i++){
    bht_chooser_tournament[i] = WN; // not taken is local; taken is global
  }
  tournament_ghistory = 0;
}



uint8_t 
gshare_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch(bht_gshare[index]){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
      return NOTTAKEN;
  }
}

uint8_t 
tournament_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t local_pattern_table_entries = 1 << tournament_local_pattern_bits;
  uint32_t pc_lower_bits = pc & (local_pattern_table_entries-1);

  uint32_t global_pattern_entries = 1 << tournament_global_pattern_bits;
  uint32_t ghistory_lower_bits = tournament_ghistory & (global_pattern_entries -1);
  
  uint32_t local_bht_index = (table_local_pattern_tournament[pc_lower_bits])&(local_pattern_table_entries-1);
  uint8_t local_result;
  // printf("local_bht_index: %d, value: %d\n", local_bht_index, bht_local_tournament[local_bht_index]);
  switch(bht_local_tournament[local_bht_index]){
    case WN:
      local_result = NOTTAKEN;
      break;
    case SN:
      local_result = NOTTAKEN;
      break;
    case WT:
      local_result = TAKEN;
      break;
    case ST:
      local_result = TAKEN;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT --- PREDICTION!\n");
      local_result = NOTTAKEN;
  }

  uint8_t global_result;
  switch(bht_global_tournament[ghistory_lower_bits]){
    case WN:
      global_result = NOTTAKEN;
      break;
    case SN:
      global_result = NOTTAKEN;
      break;
    case WT:
      global_result = TAKEN;
      break;
    case ST:
      global_result = TAKEN;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT --- PREDICTION!\n");
      global_result = NOTTAKEN;
  }

  uint8_t final_result;
  switch(bht_chooser_tournament[ghistory_lower_bits]){
    case WN:
      final_result = local_result;
      break;
    case SN:
      final_result = local_result;
      break;
    case WT:
      final_result = global_result;
      break;
    case ST:
      final_result = global_result;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT --- PREDICTION!\n");
      final_result = NOTTAKEN;
  }
  return final_result;
}

void
train_gshare(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  //Update state of entry in bht based on outcome
  switch(bht_gshare[index]){
    case WN:
      bht_gshare[index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      bht_gshare[index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      bht_gshare[index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      bht_gshare[index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome); 
}

void
train_tournament(uint32_t pc, uint8_t outcome) {
  uint32_t local_pattern_table_entries = 1 << tournament_local_pattern_bits;
  uint32_t pc_lower_bits = pc & (local_pattern_table_entries-1);

  uint32_t global_pattern_entries = 1 << tournament_global_pattern_bits;
  uint32_t ghistory_lower_bits = tournament_ghistory & (global_pattern_entries -1);
  
  uint32_t local_bht_index = (table_local_pattern_tournament[pc_lower_bits])&(local_pattern_table_entries-1);
  // update local pattern history
  table_local_pattern_tournament[pc_lower_bits] = ((table_local_pattern_tournament[pc_lower_bits] << 1) | outcome); 
  uint8_t local_result;
  switch(bht_local_tournament[local_bht_index]){
    case WN:
      local_result = NOTTAKEN;
      bht_local_tournament[local_bht_index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      local_result = NOTTAKEN;
      bht_local_tournament[local_bht_index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      local_result = TAKEN;
      bht_local_tournament[local_bht_index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      local_result = TAKEN;
      bht_local_tournament[local_bht_index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT --- TRAINING!\n");
      local_result = NOTTAKEN;
  }

  uint8_t global_result;
  switch(bht_global_tournament[ghistory_lower_bits]){
    case WN:
      global_result = NOTTAKEN;
      bht_global_tournament[ghistory_lower_bits] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      global_result = NOTTAKEN;
      bht_global_tournament[ghistory_lower_bits] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      global_result = TAKEN;
      bht_global_tournament[ghistory_lower_bits] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      global_result = TAKEN;
      bht_global_tournament[ghistory_lower_bits] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in TOURNAMENT BHT --- TRAINING!\n");
      global_result = NOTTAKEN;
  }
  if(local_result != global_result){
    if(local_result == outcome){
      switch(bht_chooser_tournament[ghistory_lower_bits]){
        case WN:
          bht_chooser_tournament[ghistory_lower_bits]=SN;
          break;
        case SN:
          bht_chooser_tournament[ghistory_lower_bits]=SN;
          break;
        case WT:
          bht_chooser_tournament[ghistory_lower_bits]=WN;
          break;
        case ST:
          bht_chooser_tournament[ghistory_lower_bits]=WT;
          break;
        default:
          printf("Warning: Undefined state of entry in TOURNAMENT BHT --- TRAINING!\n");
      }
    }
    if(global_result == outcome){
      switch(bht_chooser_tournament[ghistory_lower_bits]){
        case WN:
          bht_chooser_tournament[ghistory_lower_bits]=WT;
          break;
        case SN:
          bht_chooser_tournament[ghistory_lower_bits]=WN;
          break;
        case WT:
          bht_chooser_tournament[ghistory_lower_bits]=ST;
          break;
        case ST:
          bht_chooser_tournament[ghistory_lower_bits]=ST;
          break;
        default:
          printf("Warning: Undefined state of entry in TOURNAMENT BHT --- TRAINING!\n");
      }
    }
  }
  //Update history register
  tournament_ghistory = ((tournament_ghistory << 1) | outcome); 
}

void
cleanup_gshare() {
  free(bht_gshare);
}



void
init_predictor()
{
  switch (bpType) {
    case STATIC:
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tournament();
      break;
    case CUSTOM:
      init_tournament();
      break;
    default:
      break;
  }
  
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
      return tournament_predict(pc);
    case CUSTOM:
      return tournament_predict(pc);
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

void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return train_tournament(pc, outcome);
    default:
      break;
  }
  

}
