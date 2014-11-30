#ifndef _PBFT_R_Message_tags_h
#define _PBFT_R_Message_tags_h 1

//
// Each message type is identified by one of the tags in the set below.
//

const short Free_message_tag=0; // Used to mark free message reps.
                                // A valid message may never use this tag.
const short PBFT_R_Request_tag=1;
const short PBFT_R_Reply_tag=2;
const short PBFT_R_Pre_prepare_tag=3;
const short PBFT_R_Prepare_tag=4;
const short PBFT_R_Commit_tag=5;
const short PBFT_R_Checkpoint_tag=6;
const short PBFT_R_Status_tag=7;
const short PBFT_R_View_change_tag=8;
const short PBFT_R_New_view_tag=9;
const short PBFT_R_View_change_ack_tag=10;
const short PBFT_R_New_key_tag=11;
const short PBFT_R_Meta_data_tag=12;
const short PBFT_R_Meta_data_d_tag=13;
const short PBFT_R_Data_tag=14;
const short PBFT_R_Fetch_tag=15;
const short PBFT_R_Query_stable_tag=16;
const short PBFT_R_Reply_stable_tag=17;

#endif // _PBFT_R_Message_tags_h
