#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "define_macro_functions.h"
#include "define_macros.h"
#include "structures.h"
#include "variables.h"

#include "define_macros_functions.h"
#include "blockchain_functions.h"
#include "file_functions.h"
#include "network_daemon_functions.h"
#include "network_functions.h"
#include "network_security_functions.h"
#include "server_functions.h"
#include "string_functions.h"
#include "thread_server_functions.h"
#include "convert.h"
#include "vrf.h"
#include "crypto_vrf.h"
#include "VRF_functions.h"
#include "sha512EL.h"

/*
-----------------------------------------------------------------------------------------------------------
Functions
-----------------------------------------------------------------------------------------------------------
*/

/*
-----------------------------------------------------------------------------------------------------------
Name: start_new_round
Description: Gets the current block height and determines if a new round has started
Return: NULL
-----------------------------------------------------------------------------------------------------------
*/

void start_new_round()
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  size_t count;
  int settings;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL)
  {
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // start a new round
  memset(data,0,strlen(data));
  memcpy(data,"A new round is starting for block ",34);
  memcpy(data,current_block_height,strnlen(current_block_height,BUFFER_SIZE));
  color_print(data,"green");

  // reset the variables
  memset(current_round_part,0,strlen(current_round_part));
  memcpy(current_round_part,"1",1);
  memset(current_round_part_backup_node,0,strlen(current_round_part_backup_node));
  memcpy(current_round_part_backup_node,"0",1);

  // check if the current block height - 3 is a X-CASH proof of stake block since this will check to see if these are the first three blocks on the network
  sscanf(current_block_height,"%zu", &count);
  count = count - 3;
  memset(data,0,strnlen(data,BUFFER_SIZE));
  sprintf(data,"%zu",count);
  settings = get_block_settings(data,0);
  if (settings == 0)
  {
    // an error has occured so wait until the next round
    return;
  }
  else if (settings == 1)
  {
    // this is a proof of work block, so this is the start blocks of the network
    start_current_round_start_blocks();    
  }
  else if (settings == 2)
  {
    // this is a X-CASH proof of stake block so this is not the start blocks of the network
    start_current_round();
  }
  pointer_reset(data);
  return;
}



/*
-----------------------------------------------------------------------------------------------------------
Name: get_updated_node_list
Description: Gets the updated node list, so it will know what nodes to accept data from
Parameters:
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int get_updated_node_list()
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  size_t count;
  size_t count2;

   // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(message); \
  message = NULL; \
  free(message2); \
  message2 = NULL; \
  free(message3); \
  message3 = NULL;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || message == NULL || message2 == NULL || message3 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (message != NULL)
    {
      pointer_reset(message);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    if (message3 != NULL)
    {
      pointer_reset(message3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  #define GET_UPDATED_NODE_LIST_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // iniltize the variables
  memset(data,0,strnlen(data,BUFFER_SIZE));

  // read the nodes_updated_time.txt
  read_file(data,NODES_UPDATED_TIME_FILE_NAME);

  // create the message
  const size_t DATA_LENGTH = strnlen(data,BUFFER_SIZE);
  memcpy(message,"{\r\n \"message_settings\": \"NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST\",\r\n \"nodes_updated_time\": \"",98);
  memcpy(message+98,data,DATA_LENGTH);
  memcpy(message+98+DATA_LENGTH,"\",\r\n}",5);
  memset(data,0,strnlen(data,BUFFER_SIZE));

  // sign_data
  if (sign_data(message,0) == 0)
  { 
    GET_UPDATED_NODE_LIST_ERROR("Could not sign_data\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
  }
 
  // send the message to the consensus node
  if (send_and_receive_data_socket(data,current_consensus_nodes_IP_address,SEND_DATA_PORT,message,TOTAL_CONNECTION_TIME_SETTINGS,"getting last block verifiers update time",0) == 0)
  {
    GET_UPDATED_NODE_LIST_ERROR("Could not send data to the consensus node\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
  }
  
  // verify the data
  if (verify_data(data,0,0,0) == 0)
  {   
    GET_UPDATED_NODE_LIST_ERROR("Could not verify data from the consensus node\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
  }

  // parse the data
  memset(message,0,strnlen(message,BUFFER_SIZE));
  if (parse_json_data(data,"nodes_name_list",message) == 0 || parse_json_data(data,"nodes_public_address_list",message2) == 0 || parse_json_data(data,"nodes_IP_address_list",message3) == 0)
  {
    GET_UPDATED_NODE_LIST_ERROR("Could not parse data\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
  }

  // convert the strings to json
  string_replace(message,"\\\"","\"");
  string_replace(message2,"\\\"","\"");
  string_replace(message3,"\\\"","\"");

  // check if we need to update the node list
  if (strncmp(message,"UPDATED_NODE_LIST",BUFFER_SIZE) != 0 && strncmp(message2,"UPDATED_NODE_LIST",BUFFER_SIZE) != 0 && strncmp(message3,"UPDATED_NODE_LIST",BUFFER_SIZE) != 0)
  {
    memset(data,0,strnlen(data,BUFFER_SIZE));
    sprintf(data,"%ld",time(NULL)); 

    // update the node list and load the nodes list into the global variables  
    write_file(message,NODES_NAME_LIST_FILE_NAME);  
    write_file(message2,NODES_PUBLIC_ADDRESS_LIST_FILE_NAME);      
    write_file(message3,NODES_IP_ADDRESS_LIST_FILE_NAME);
    write_file(data,NODES_IP_ADDRESS_LIST_FILE_NAME);  
    color_print("The node list has been updated successfully","green");
  }
  else
  {
    // the node list has already been updated    
    read_file(message,NODES_NAME_LIST_FILE_NAME);
    read_file(message2,NODES_PUBLIC_ADDRESS_LIST_FILE_NAME);
    read_file(message3,NODES_IP_ADDRESS_LIST_FILE_NAME);
    color_print("The node list is already up to date","green");
  }

  // clear any data that was already in the block_verifiers_list struct
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    memset(block_verifiers_list.block_verifiers_name[count],0,strnlen(block_verifiers_list.block_verifiers_name[count],BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    memset(block_verifiers_list.block_verifiers_public_address[count],0,strnlen(block_verifiers_list.block_verifiers_public_address[count],XCASH_WALLET_LENGTH));
    memset(block_verifiers_list.block_verifiers_IP_address[count],0,strnlen(block_verifiers_list.block_verifiers_IP_address[count],BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
  }

  // load all of the data into the block_verifiers_list struct
  for (count = 0, count2 = 1; count < BLOCK_VERIFIERS_AMOUNT; count++, count2++)
  {
    memset(data,0,strnlen(data,BUFFER_SIZE));
    memcpy(data,"node",4);
    sprintf(data+4,"%zu",count2);
    if (parse_json_data(message,data,data2) == 0)
    {
      GET_UPDATED_NODE_LIST_ERROR("Could not parse data\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
    }
    memcpy(block_verifiers_list.block_verifiers_name[count],data2,strnlen(data2,BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    memset(data2,0,strnlen(data2,BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    if (parse_json_data(message2,data,data2) == 0)
    {
      GET_UPDATED_NODE_LIST_ERROR("Could not parse data\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
    }
    memcpy(block_verifiers_list.block_verifiers_public_address[count],data2,strnlen(data2,XCASH_WALLET_LENGTH));
    memset(data2,0,strnlen(data2,XCASH_WALLET_LENGTH));
    if (parse_json_data(message3,data,data2) == 0)
    {
      GET_UPDATED_NODE_LIST_ERROR("Could not parse data\nFunction: get_updated_node_list\nReceived Message: CONSENSUS_NODE_TO_NODE_RECEIVE_UPDATED_NODE_LIST\nSend Message: NODE_TO_CONSENSUS_NODE_SEND_UPDATED_NODE_LIST");
    }
    memcpy(block_verifiers_list.block_verifiers_IP_address[count],data2,strnlen(data2,BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
    memset(data2,0,strnlen(data2,BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
  }

  return 1;

  #undef pointer_reset_all
  #undef GET_UPDATED_NODE_LIST_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_received_data_xcash_proof_of_stake_test_data
Description: Runs the code when the server receives the xcash_proof_of_stake_test_data message
Parameters:
  CLIENT_SOCKET - The socket to send data to
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_received_data_xcash_proof_of_stake_test_data(const int CLIENT_SOCKET, char* MESSAGE)
{
  // verify the message
  if (verify_data(MESSAGE,0,1,1) == 0)
  {   
    return 0;
  }
  else
  {
    if (send_data(CLIENT_SOCKET,MESSAGE,1) == 1)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }  
}


/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_consensus_node_to_node
Description: Runs the code when the server receives the CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS message
Parameters:
  parameters - A mainnode_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
    int data_received - 1 if the node has received data from the main node, otherwise 0
    char* main_node - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* current_round_part - The current round part (1-4).
    char* current_round_part_backup_node - The current main node in the current round part (0-5)
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_consensus_node_to_node(struct mainnode_timeout_thread_parameters* parameters, char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL)
  {
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // threads
  pthread_t thread_id;

  // define macros
  #define SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset(data); \
  return 0;

  // verify the data
  if (verify_data(message,0,0,0) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_ERROR("Could not verify data\nFunction: server_receive_data_socket_consensus_node_to_node\nReceived Message: CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS");
  }

  // parse the message
  memset(main_nodes_public_address,0,strnlen(main_nodes_public_address,BUFFER_SIZE));
  memset(current_round_part,0,strnlen(current_round_part,BUFFER_SIZE));
  memset(current_round_part_backup_node,0,strnlen(current_round_part_backup_node,BUFFER_SIZE));
  if (parse_json_data(message,"main_nodes_public_address",main_nodes_public_address) == 0 || parse_json_data(message,"current_round_part",current_round_part) == 0 || parse_json_data(message,"current_round_part_backup_node",current_round_part_backup_node) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_ERROR("Could not parse main_nodes_public_address\nFunction: server_receive_data_socket_consensus_node_to_node\nReceived Message: CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS");
  }
  memset(data,0,strnlen(data,BUFFER_SIZE));

  // create a timeout from the time the consensus node lets us know who the main node is for this part of the round, to the time the main node sends us data.
  if (strncmp(current_round_part,"1",BUFFER_SIZE) == 0 || strncmp(current_round_part,"3",BUFFER_SIZE) == 0)
  {
    memcpy(data,"VRF_PUBLIC_AND_SECRET_KEY",25);
  }
  else if (strncmp(current_round_part,"2",BUFFER_SIZE) == 0)
  {
    memcpy(data,"VRF_RANDOM_DATA",15);
  }
  else if (strncmp(current_round_part,"4",BUFFER_SIZE) == 0)
  {
    memcpy(data,"BLOCK_PRODUCER",14);
  }

  // create a mainnode_timeout_thread_parameters struct since this function will use the mainnode_timeout_thread
  parameters->data_received = 0;
  parameters->main_node = data;
  parameters->current_round_part = current_round_part;
  parameters->current_round_part_backup_node = current_round_part_backup_node;
 
  // create a timeout for this connection, since we need to limit the amount of time a client has to send data from once it connected
  if (pthread_create(&thread_id, NULL, &mainnode_timeout_thread, (void *)parameters) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_ERROR("Could not create the timeout thread\nFunction: server_receive_data_socket_consensus_node_to_node\nReceived Message: CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS");
  }
  // set the thread to dettach once completed, since we do not need to use anything it will return
  if (pthread_detach(thread_id) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_ERROR("Could not start the timeout thread in detach mode\nFunction: server_receive_data_socket_consensus_node_to_node\nReceived Message: CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS");
  }

  // set the next server message
  if (strncmp(current_round_part,"1",BUFFER_SIZE) == 0)
  {
    memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
    memcpy(server_message,"MAIN_NODES_TO_NODES_PART_1_OF_ROUND",35);
  }
  else if (strncmp(current_round_part,"2",BUFFER_SIZE) == 0)
  {
    memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
    memcpy(server_message,"MAIN_NODES_TO_NODES_PART_2_OF_ROUND",35);
  }
  else if (strncmp(current_round_part,"3",BUFFER_SIZE) == 0)
  {
    memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
    memcpy(server_message,"MAIN_NODES_TO_NODES_PART_3_OF_ROUND",35);
  }
  else if (strncmp(current_round_part,"4",BUFFER_SIZE) == 0)
  {
    memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
    memcpy(server_message,"MAIN_NODES_TO_NODES_PART_4_OF_ROUND",35);
  }

  pointer_reset(data);
  return 1;
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_main_node_to_node_message_part_1
Description: Runs the code when the server receives the MAIN_NODES_TO_NODES_PART_1_OF_ROUND message
Parameters:
  mainnode_timeout_thread_parameters - A mainnode_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
    int data_received - 1 if the node has received data from the main node, otherwise 0
    char* main_node - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* current_round_part - The current round part (1-4).
    char* current_round_part_backup_node - The current main node in the current round part (0-5)
  node_to_node_timeout_thread_parameters - A node_to_node_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
  current_round_part_consensus_node_data - A current_round_part_consensus_node_data struct
    char* vrf_public_key - Holds the forked process ID that the client is connected to
    char* vrf_alpha_string - 1 if the node has received data from the main node, otherwise 0
    char* vrf_proof - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* vrf_beta_string - The current round part (1-4).
    char* block_blob - The current main node in the current round part (0-5)
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_main_node_to_node_message_part_1(struct mainnode_timeout_thread_parameters* mainnode_timeout_thread_parameters, struct node_to_node_timeout_thread_parameters* node_to_node_timeout_thread_parameters, char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  int count = 0;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL || message == NULL || message2 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    if (message3 != NULL)
    {
      pointer_reset(message3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // threads
  pthread_t thread_id;

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(message2); \
  message2 = NULL; \
  free(message3); \
  message3 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // since the block verifier has received data from the main node, we need to stop the mainnode_timeout_thread
  mainnode_timeout_thread_parameters->data_received = 1;

  // set the next server message since the block verifiers will send the data to each other
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"NODES_TO_NODES_VOTE_RESULTS",27); 

  // verify the data
  if (verify_data(message,0,1,1) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR("Could not verify data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // parse the message
  memset(vrf_public_key_part_1,0,strnlen(vrf_public_key_part_1,BUFFER_SIZE));
  if (parse_json_data(message,"vrf_public_key",vrf_public_key_part_1) == 0 || parse_json_data(message,"vrf_alpha_string",data) == 0 || parse_json_data(message,"vrf_proof",data2) == 0 || parse_json_data(message,"vrf_beta_string",data3) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR("Could not parse the data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // create the message
  memcpy(message2,"{\r\n \"message_settings\": \"NODES_TO_NODES_VOTE_RESULTS\",\r\n \"vote_settings\": \"",75);

  // verify the VRF data
  if (crypto_vrf_verify((unsigned char*)data3,(const unsigned char*)vrf_public_key_part_1,(const unsigned char*)data2,(const unsigned char*)data,(unsigned long long)strnlen(data,BUFFER_SIZE)) == 0)
  {
    memcpy(message2+75,"valid",5);
  }
  else
  {
    memcpy(message2+75,"invalid",7);
  }
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n \"vote_data\": \"",19);

  // SHA2-512 hash all of the VRF data
  memcpy(message3,vrf_public_key_part_1,strnlen(vrf_public_key_part_1,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data,strnlen(data,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data2,strnlen(data2,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data3,strnlen(data3,BUFFER_SIZE));
  crypto_hash_sha512((unsigned char*)current_round_part_vote_data.current_vote_results,(const unsigned char*)message3,(unsigned long long)strnlen(message3,BUFFER_SIZE));
  memcpy(message2+strnlen(message,BUFFER_SIZE),current_round_part_vote_data.current_vote_results,DATA_HASH_LENGTH);
  
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n}",5); 

  // save all of the VRF data to the current_round_part_consensus_node_data struct
  memcpy(current_round_part_consensus_node_data.vrf_public_key,vrf_public_key_part_1,strnlen(vrf_public_key_part_1,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_alpha_string,data,strnlen(data,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_proof,data2,strnlen(data2,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_beta_string,data3,strnlen(data3,BUFFER_SIZE));

  // sign_data
  if (sign_data(message2,0) == 0)
  { 
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR("Could not sign_data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // send the message to all block verifiers
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    if (memcmp(block_verifiers_list.block_verifiers_public_address[count],xcash_wallet_public_address,XCASH_WALLET_LENGTH) != 0)
    {
      send_data_socket(block_verifiers_list.block_verifiers_IP_address[count],SEND_DATA_PORT,message2,"sending NODES_TO_NODES_VOTE_RESULTS to the block verifiers",0);
    }
  }

  // start the node_to_node_message_timeout
  if (pthread_create(&thread_id, NULL, &node_to_node_message_timeout_thread, (void *)node_to_node_timeout_thread_parameters) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR("Could not create the timeout thread\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }
  // set the thread to dettach once completed, since we do not need to use anything it will return
  if (pthread_detach(thread_id) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR("Could not start the timeout thread in detach mode\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_1_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_main_node_to_node_message_part_2
Description: Runs the code when the server receives the MAIN_NODES_TO_NODES_PART_2_OF_ROUND message
Parameters:
  mainnode_timeout_thread_parameters - A mainnode_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
    int data_received - 1 if the node has received data from the main node, otherwise 0
    char* main_node - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* current_round_part - The current round part (1-4).
    char* current_round_part_backup_node - The current main node in the current round part (0-5)
  node_to_node_timeout_thread_parameters - A node_to_node_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
  current_round_part_consensus_node_data - A current_round_part_consensus_node_data struct
    char* vrf_public_key - Holds the forked process ID that the client is connected to
    char* vrf_alpha_string - 1 if the node has received data from the main node, otherwise 0
    char* vrf_proof - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* vrf_beta_string - The current round part (1-4).
    char* block_blob - The current main node in the current round part (0-5)
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_main_node_to_node_message_part_2(struct mainnode_timeout_thread_parameters* mainnode_timeout_thread_parameters, struct node_to_node_timeout_thread_parameters* node_to_node_timeout_thread_parameters, char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  int count = 0;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL || message == NULL || message2 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    if (message3 != NULL)
    {
      pointer_reset(message3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // threads
  pthread_t thread_id;

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(message2); \
  message2 = NULL; \
  free(message3); \
  message3 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // since the block verifier has received data from the main node, we need to stop the mainnode_timeout_thread
  mainnode_timeout_thread_parameters->data_received = 1;

  // set the next server message since the block verifiers will send the data to each other
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"NODES_TO_NODES_VOTE_RESULTS",27); 

  // verify the data
  if (verify_data(message,0,1,1) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR("Could not verify data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_2_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // parse the message
  memset(vrf_alpha_string_part_2,0,strnlen(vrf_alpha_string_part_2,BUFFER_SIZE));
  if (parse_json_data(message,"vrf_public_key",data) == 0 || parse_json_data(message,"vrf_alpha_string",vrf_alpha_string_part_2) == 0 || parse_json_data(message,"vrf_proof",data2) == 0 || parse_json_data(message,"vrf_beta_string",data3) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR("Could not parse the data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_2_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // create the message
  memcpy(message2,"{\r\n \"message_settings\": \"NODES_TO_NODES_VOTE_RESULTS\",\r\n \"vote_settings\": \"",75);

  // verify the VRF data
  if (crypto_vrf_verify((unsigned char*)data3,(const unsigned char*)data,(const unsigned char*)data2,(const unsigned char*)vrf_alpha_string_part_2,(unsigned long long)strnlen(vrf_alpha_string_part_2,BUFFER_SIZE)) == 0)
  {
    memcpy(message2+75,"valid",5);
  }
  else
  {
    memcpy(message2+75,"invalid",7);
  }
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n \"vote_data\": \"",19);

  // SHA2-512 hash all of the VRF data
  memcpy(message3,data,strnlen(data,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),vrf_alpha_string_part_2,strnlen(vrf_alpha_string_part_2,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data2,strnlen(data2,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data3,strnlen(data3,BUFFER_SIZE));
  crypto_hash_sha512((unsigned char*)current_round_part_vote_data.current_vote_results,(const unsigned char*)message3,(unsigned long long)strnlen(message3,BUFFER_SIZE));
  memcpy(message2+strnlen(message,BUFFER_SIZE),current_round_part_vote_data.current_vote_results,DATA_HASH_LENGTH);
  
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n}",5); 

  // save all of the VRF data to the current_round_part_consensus_node_data struct
  memcpy(current_round_part_consensus_node_data.vrf_public_key,data,strnlen(data,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_alpha_string,vrf_alpha_string_part_2,strnlen(vrf_alpha_string_part_2,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_proof,data2,strnlen(data2,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_beta_string,data3,strnlen(data3,BUFFER_SIZE));

  // sign_data
  if (sign_data(message2,0) == 0)
  { 
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR("Could not sign_data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_2_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // send the message to all block verifiers
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    if (memcmp(block_verifiers_list.block_verifiers_public_address[count],xcash_wallet_public_address,XCASH_WALLET_LENGTH) != 0)
    {
      send_data_socket(block_verifiers_list.block_verifiers_IP_address[count],SEND_DATA_PORT,message2,"sending NODES_TO_NODES_VOTE_RESULTS to the block verifiers",0);
    }
  }

  // start the node_to_node_message_timeout
  if (pthread_create(&thread_id, NULL, &node_to_node_message_timeout_thread, (void *)node_to_node_timeout_thread_parameters) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR("Could not create the timeout thread\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_2_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }
  // set the thread to dettach once completed, since we do not need to use anything it will return
  if (pthread_detach(thread_id) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR("Could not start the timeout thread in detach mode\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_2_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_2_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_main_node_to_node_message_part_3
Description: Runs the code when the server receives the MAIN_NODES_TO_NODES_PART_3_OF_ROUND message
Parameters:
  mainnode_timeout_thread_parameters - A mainnode_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
    int data_received - 1 if the node has received data from the main node, otherwise 0
    char* main_node - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* current_round_part - The current round part (1-4).
    char* current_round_part_backup_node - The current main node in the current round part (0-5)
  node_to_node_timeout_thread_parameters - A node_to_node_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
  current_round_part_consensus_node_data - A current_round_part_consensus_node_data struct
    char* vrf_public_key - Holds the forked process ID that the client is connected to
    char* vrf_alpha_string - 1 if the node has received data from the main node, otherwise 0
    char* vrf_proof - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* vrf_beta_string - The current round part (1-4).
    char* block_blob - The current main node in the current round part (0-5)
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_main_node_to_node_message_part_3(struct mainnode_timeout_thread_parameters* mainnode_timeout_thread_parameters, struct node_to_node_timeout_thread_parameters* node_to_node_timeout_thread_parameters, char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data4 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  int count = 0;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL || message == NULL || message2 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (data4 != NULL)
    {
      pointer_reset(data4);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    if (message3 != NULL)
    {
      pointer_reset(message3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // threads
  pthread_t thread_id;

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(data4); \
  data4 = NULL; \
  free(message2); \
  message2 = NULL; \
  free(message3); \
  message3 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // since the block verifier has received data from the main node, we need to stop the mainnode_timeout_thread
  mainnode_timeout_thread_parameters->data_received = 1;

  // set the next server message since the block verifiers will send the data to each other
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"NODES_TO_NODES_VOTE_RESULTS",27); 

  // verify the data
  if (verify_data(message,0,1,1) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR("Could not verify data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_3_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // parse the message
  if (parse_json_data(message,"vrf_public_key",data) == 0 || parse_json_data(message,"vrf_alpha_string",data2) == 0 || parse_json_data(message,"vrf_proof",data3) == 0 || parse_json_data(message,"vrf_beta_string",data4) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR("Could not parse the data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_3_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // create the message
  memcpy(message2,"{\r\n \"message_settings\": \"NODES_TO_NODES_VOTE_RESULTS\",\r\n \"vote_settings\": \"",75);

  // verify the VRF data and check that the vrf_public_key_part_1 and vrf_alpha_string_part_2 match the current vrf_public_key and vrf_alpha_string
  if (crypto_vrf_verify((unsigned char*)data4,(const unsigned char*)data,(const unsigned char*)data3,(const unsigned char*)data2,(unsigned long long)strnlen(data2,BUFFER_SIZE)) == 0 || memcmp(current_round_part_consensus_node_data.vrf_public_key,data,strnlen(data,BUFFER_SIZE)) != 0 || memcmp(current_round_part_consensus_node_data.vrf_alpha_string,data2,strnlen(data2,BUFFER_SIZE)) != 0)
  {
    memcpy(message2+75,"valid",5);
  }
  else
  {
    memcpy(message2+75,"invalid",7);
  }
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n \"vote_data\": \"",19);

  // SHA2-512 hash all of the VRF data
  memcpy(message3,data,strnlen(data,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data2,strnlen(data2,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data3,strnlen(data3,BUFFER_SIZE));
  memcpy(message3+strnlen(message3,BUFFER_SIZE),data4,strnlen(data4,BUFFER_SIZE));
  crypto_hash_sha512((unsigned char*)current_round_part_vote_data.current_vote_results,(const unsigned char*)message3,(unsigned long long)strnlen(message3,BUFFER_SIZE));
  memcpy(message2+strnlen(message,BUFFER_SIZE),current_round_part_vote_data.current_vote_results,DATA_HASH_LENGTH);
  
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n}",5); 

  // save all of the VRF data to the current_round_part_consensus_node_data struct
  memcpy(current_round_part_consensus_node_data.vrf_public_key,data,strnlen(data,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_alpha_string,vrf_alpha_string_part_2,strnlen(vrf_alpha_string_part_2,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_proof,data2,strnlen(data2,BUFFER_SIZE));
  memcpy(current_round_part_consensus_node_data.vrf_beta_string,data3,strnlen(data3,BUFFER_SIZE));

  // sign_data
  if (sign_data(message2,0) == 0)
  { 
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR("Could not sign_data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_3_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // send the message to all block verifiers
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    if (memcmp(block_verifiers_list.block_verifiers_public_address[count],xcash_wallet_public_address,XCASH_WALLET_LENGTH) != 0)
    {
      send_data_socket(block_verifiers_list.block_verifiers_IP_address[count],SEND_DATA_PORT,message2,"sending NODES_TO_NODES_VOTE_RESULTS to the block verifiers",0);
    }
  }

  // start the node_to_node_message_timeout
  if (pthread_create(&thread_id, NULL, &node_to_node_message_timeout_thread, (void *)node_to_node_timeout_thread_parameters) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR("Could not create the timeout thread\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_3_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }
  // set the thread to dettach once completed, since we do not need to use anything it will return
  if (pthread_detach(thread_id) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR("Could not start the timeout thread in detach mode\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_3_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_3_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_main_node_to_node_message_part_4
Description: Runs the code when the server receives the MAIN_NODES_TO_NODES_PART_4_OF_ROUND message
Parameters:
  mainnode_timeout_thread_parameters - A mainnode_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
    int data_received - 1 if the node has received data from the main node, otherwise 0
    char* main_node - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* current_round_part - The current round part (1-4).
    char* current_round_part_backup_node - The current main node in the current round part (0-5)
  node_to_node_timeout_thread_parameters - A node_to_node_timeout_thread_parameters struct
    pid_t process_id - Holds the forked process ID that the client is connected to
  current_round_part_consensus_node_data - A current_round_part_consensus_node_data struct
    char* vrf_public_key - Holds the forked process ID that the client is connected to
    char* vrf_alpha_string - 1 if the node has received data from the main node, otherwise 0
    char* vrf_proof - The main node (VRF_PUBLIC_AND_SECRET_KEY, VRF_RANDOM_DATA, BLOCK_PRODUCER)
    char* vrf_beta_string - The current round part (1-4).
    char* block_blob - The current main node in the current round part (0-5)
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_main_node_to_node_message_part_4(struct mainnode_timeout_thread_parameters* mainnode_timeout_thread_parameters, struct node_to_node_timeout_thread_parameters* node_to_node_timeout_thread_parameters, char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  int count = 0;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // threads
  pthread_t thread_id;

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(message2); \
  message2 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // since the block verifier has received data from the main node, we need to stop the mainnode_timeout_thread
  mainnode_timeout_thread_parameters->data_received = 1;

  // set the next server message since the block verifiers will send the data to each other
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"NODES_TO_NODES_VOTE_RESULTS",27); 

  // verify the data
  if (verify_data(message,0,1,1) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not verify data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // parse the message
  if (parse_json_data(message,"block_blob",data) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not parse the data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // create the message
  memcpy(message2,"{\r\n \"message_settings\": \"NODES_TO_NODES_VOTE_RESULTS\",\r\n \"vote_settings\": \"",75);

  // convert the network_block_string to blockchain_data
  if (network_block_string_to_blockchain_data(data,"0") == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not convert the network_block_string to blockchain_data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // verify the network block
  if (verify_network_block_data(0,1,1,"0") == 1)
  {
    memcpy(message2+75,"valid",5);
  }
  else
  {
    memcpy(message2+75,"invalid",7);
  }
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n \"vote_data\": \"",19);

  // SHA2-512 hash the network block
  crypto_hash_sha512((unsigned char*)current_round_part_vote_data.current_vote_results,(const unsigned char*)data,(unsigned long long)strnlen(data,BUFFER_SIZE));
  memcpy(message2+strnlen(message,BUFFER_SIZE),current_round_part_vote_data.current_vote_results,DATA_HASH_LENGTH);
  
  memcpy(message2+strnlen(message2,BUFFER_SIZE),"\",\r\n}",5); 

  // save the netowrk block to the current_round_part_consensus_node_data struct
  memcpy(current_round_part_consensus_node_data.block_blob,data,strnlen(data,BUFFER_SIZE));

  // sign_data
  if (sign_data(message2,0) == 0)
  { 
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not sign_data\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // send the message to all block verifiers
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    if (memcmp(block_verifiers_list.block_verifiers_public_address[count],xcash_wallet_public_address,XCASH_WALLET_LENGTH) != 0)
    {
      send_data_socket(block_verifiers_list.block_verifiers_IP_address[count],SEND_DATA_PORT,message2,"sending NODES_TO_NODES_VOTE_RESULTS to the block verifiers",0);
    }
  }

  // start the node_to_node_message_timeout
  if (pthread_create(&thread_id, NULL, &node_to_node_message_timeout_thread, (void *)node_to_node_timeout_thread_parameters) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not create the timeout thread\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }
  // set the thread to dettach once completed, since we do not need to use anything it will return
  if (pthread_detach(thread_id) != 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR("Could not start the timeout thread in detach mode\nFunction: mainnode_to_node_message_part_1\nReceived Message: MAIN_NODES_TO_NODES_PART_4_OF_ROUND\nSend Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_MAIN_NODE_TO_NODE_MESSAGE_PART_4_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_node_to_node
Description: Runs the code when the server receives the NODES_TO_NODES_VOTE_RESULTS message
Parameters:
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_node_to_node(char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_NODE_TO_NODE_ERROR(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // verify the data
  if (verify_data(message,0,1,1) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_NODE_TO_NODE_ERROR("Could not verify data\nFunction: server_receive_data_socket_node_to_node\nReceived Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // parse the message
  if (parse_json_data(message,"vote_settings",data) == 0 || parse_json_data(message,"vote_data",data2) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_NODE_TO_NODE_ERROR("Could not parse the data\nFunction: server_receive_data_socket_node_to_node\nReceived Message: NODES_TO_NODES_VOTE_RESULTS");
  }

  // process the vote data
  if (memcmp(data,"valid",5) == 0 && memcmp(data2,current_round_part_vote_data.current_vote_results,DATA_HASH_LENGTH) == 0)
  {
    current_round_part_vote_data.vote_results_valid++;
  }
  else
  {
    current_round_part_vote_data.vote_results_invalid++;
  }

  pointer_reset_all;
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_NODE_TO_NODE_ERROR
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round
Description: Runs the code when the server receives the CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND message
Parameters:
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round(char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* vrf_public_key = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* vrf_secret_key = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* vrf_alpha_string = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* vrf_proof = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* vrf_beta_string = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message_copy1;
  int count = 0;
  int counter = 0;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL || vrf_public_key == NULL || vrf_secret_key == NULL || vrf_alpha_string == NULL || vrf_proof == NULL || vrf_beta_string == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (vrf_public_key != NULL)
    {
      pointer_reset(vrf_public_key);
    }
    if (vrf_secret_key != NULL)
    {
      pointer_reset(vrf_secret_key);
    }
    if (vrf_alpha_string != NULL)
    {
      pointer_reset(vrf_alpha_string);
    }
    if (vrf_proof != NULL)
    {
      pointer_reset(vrf_proof);
    }
    if (vrf_beta_string != NULL)
    {
      pointer_reset(vrf_beta_string);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // define macros
  #define MINIMUM 1
  #define MAXIMUM 255
  
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(vrf_public_key); \
  vrf_public_key = NULL; \
  free(vrf_secret_key); \
  vrf_secret_key = NULL; \
  free(vrf_alpha_string); \
  vrf_alpha_string = NULL; \
  free(vrf_proof); \
  vrf_proof = NULL; \
  free(vrf_beta_string); \
  vrf_beta_string = NULL; \

  #define SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // verify the data
  if (verify_data(message,0,0,0) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not verify data\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
  }

  // parse the message
  memset(current_round_part,0,strnlen(current_round_part,BUFFER_SIZE));
  memset(current_round_part_backup_node,0,strnlen(current_round_part_backup_node,BUFFER_SIZE));
  if (parse_json_data(message,"message",current_round_part) == 0 || parse_json_data(message,"VRF_block_blob",data2) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not parse main_nodes_public_address\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
  }

  // create the VRF data for this part of the round
  if (memcmp(current_round_part,"1",1) == 0)
  {
    // The VRF private and secret key will create the public and secret key
    if (create_random_VRF_keys((unsigned char*)vrf_public_key,(unsigned char*)vrf_secret_key) == 0 || crypto_vrf_is_valid_key((const unsigned char*)vrf_public_key) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF public and secret key\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the alpha string
    memcpy(vrf_alpha_string,vrf_public_key,strnlen(vrf_public_key,BUFFER_SIZE));

    // create the proof
    if (crypto_vrf_prove((unsigned char*)vrf_proof,(const unsigned char*)vrf_secret_key,(const unsigned char*)vrf_alpha_string,(unsigned long long)strlen(vrf_alpha_string)) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the beta string
    if (crypto_vrf_proof_to_hash((unsigned char*)vrf_beta_string,(const unsigned char*)vrf_proof) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF beta string, or verify the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // save the VRF public key and secret key
    memset(vrf_public_key_part_1,0,strlen(vrf_public_key_part_1));
    memset(vrf_secret_key_part_1,0,strlen(vrf_secret_key_part_1));
    memcpy(vrf_public_key_part_1,vrf_public_key,strnlen(vrf_public_key,BUFFER_SIZE));
    memcpy(vrf_secret_key_part_1,vrf_secret_key,strnlen(vrf_secret_key,BUFFER_SIZE));

    // create the message
    memcpy(data3,"{\r\n \"message_settings\": \"MAIN_NODES_TO_NODES_PART_1_OF_ROUND\",\r\n \"vrf_public_key\": \"",84);
    memcpy(data3,vrf_public_key,strnlen(vrf_public_key,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_alpha_string\": \"",26);
    memcpy(data3,vrf_alpha_string,strnlen(vrf_alpha_string,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_proof\": \"",26);
    memcpy(data3,vrf_proof,strnlen(vrf_proof,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_beta_string\": \"",26);
    memcpy(data3,vrf_beta_string,strnlen(vrf_beta_string,BUFFER_SIZE));
    memcpy(data3,"\",\r\n}",5);
  }
  else if (memcmp(current_round_part,"2",1) == 0)
  {
    // The VRF private and secret key will create the public and secret key
    if (create_random_VRF_keys((unsigned char*)vrf_public_key,(unsigned char*)vrf_secret_key) == 0 || crypto_vrf_is_valid_key((const unsigned char*)vrf_public_key) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF public and secret key\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the alpha string
    memset(data,0,strnlen(data,BUFFER_SIZE));
    if (get_previous_block_hash(data,0) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not get the previous block hash\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }
    memcpy(vrf_alpha_string,data,strnlen(data,BUFFER_SIZE));
    // create the 100 character random string
    for (count = 0, counter = 0; count < 50; count++, counter +=2)
    {
      sprintf(vrf_alpha_string+counter,"%02x",((rand() % (MAXIMUM - MINIMUM + 1)) + MINIMUM) & 0xFF);
    }
    memset(vrf_alpha_string_part_2,0,strlen(vrf_alpha_string_part_2));
    memcpy(vrf_alpha_string_part_2,vrf_alpha_string,strnlen(vrf_alpha_string,BUFFER_SIZE));

    // create the proof
    if (crypto_vrf_prove((unsigned char*)vrf_proof,(const unsigned char*)vrf_secret_key,(const unsigned char*)vrf_alpha_string,(unsigned long long)strlen(vrf_alpha_string)) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the beta string
    if (crypto_vrf_proof_to_hash((unsigned char*)vrf_beta_string,(const unsigned char*)vrf_proof) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF beta string, or verify the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the message
    memcpy(data3,"{\r\n \"message_settings\": \"MAIN_NODES_TO_NODES_PART_2_OF_ROUND\",\r\n \"vrf_public_key\": \"",84);
    memcpy(data3,vrf_public_key,strnlen(vrf_public_key,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_alpha_string\": \"",26);
    memcpy(data3,vrf_alpha_string,strnlen(vrf_alpha_string,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_proof\": \"",26);
    memcpy(data3,vrf_proof,strnlen(vrf_proof,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_beta_string\": \"",26);
    memcpy(data3,vrf_beta_string,strnlen(vrf_beta_string,BUFFER_SIZE));
    memcpy(data3,"\",\r\n}",5);
  }
  else if (memcmp(current_round_part,"3",1) == 0)
  {
    // create the proof
    if (crypto_vrf_prove((unsigned char*)vrf_proof,(const unsigned char*)vrf_secret_key_part_1,(const unsigned char*)vrf_alpha_string_part_2,(unsigned long long)strlen(vrf_alpha_string_part_2)) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the beta string
    if (crypto_vrf_proof_to_hash((unsigned char*)vrf_beta_string,(const unsigned char*)vrf_proof) == 1)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not create the VRF beta string, or verify the VRF proof\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // create the message
    memcpy(data3,"{\r\n \"message_settings\": \"MAIN_NODES_TO_NODES_PART_3_OF_ROUND\",\r\n \"vrf_public_key\": \"",84);
    memcpy(data3,vrf_public_key_part_1,strnlen(vrf_public_key_part_1,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_alpha_string\": \"",26);
    memcpy(data3,vrf_alpha_string_part_2,strnlen(vrf_alpha_string_part_2,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_proof\": \"",26);
    memcpy(data3,vrf_proof,strnlen(vrf_proof,BUFFER_SIZE));
    memcpy(data3,"\",\r\n \"vrf_beta_string\": \"",26);
    memcpy(data3,vrf_beta_string,strnlen(vrf_beta_string,BUFFER_SIZE));
    memcpy(data3,"\",\r\n}",5);
  }
  else if (memcmp(current_round_part,"4",1) == 0)
  {
    // get the block template
    memset(data,0,strnlen(data,BUFFER_SIZE));
    if (get_block_template(data,MAXIMUM_RESERVE_BYTE_TEXT_LENGTH,0) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not get the block template\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
    }

    // replace the reserve bytes with the received reserve bytes
    message_copy1 = strstr(data,RESERVE_BYTE_START_STRING);
    count = strnlen(data,BUFFER_SIZE) - strnlen(message_copy1,BUFFER_SIZE);
    memcpy(data+count,data2,strnlen(data2,BUFFER_SIZE));

    // create the message
    memcpy(data3,"{\r\n \"message_settings\": \"MAIN_NODES_TO_NODES_PART_4_OF_ROUND\",\r\n \"block_blob\": \"",80);
    memcpy(data3,data,strnlen(data,BUFFER_SIZE));
    memcpy(data3,"\",\r\n}",5);
  }

  // sign_data
  if (sign_data(data3,0) == 0)
  { 
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND("Could not sign_data\nFunction: server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round\nReceived Message: CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\nSend Message: MAIN_NODES_TO_NODES_PART_1_OF_ROUND|MAIN_NODES_TO_NODES_PART_2_OF_ROUND|MAIN_NODES_TO_NODES_PART_3_OF_ROUND|MAIN_NODES_TO_NODES_PART_4_OF_ROUND");
  }

  // send the message to all block verifiers
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    if (memcmp(block_verifiers_list.block_verifiers_public_address[count],xcash_wallet_public_address,XCASH_WALLET_LENGTH) != 0)
    {
      send_data_socket(block_verifiers_list.block_verifiers_IP_address[count],SEND_DATA_PORT,data,"",0);
    }
  }

  // set the next server message since the block verifiers will send the data to each other
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS|CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND",96);

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_MAIN_NODE_MESSAGE_START_PART_OF_ROUND
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes
Description: Runs the code when the server receives the CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES message
Parameters:
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes(char* message)
{
  // Variables
  char* data = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* data3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message2 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  char* message3 = (char*)calloc(BUFFER_SIZE,sizeof(char));
  size_t count;
  size_t count2;

  // check if the memory needed was allocated on the heap successfully
  if (data == NULL || data2 == NULL || data3 == NULL || message2 == NULL || message3 == NULL)
  {
    if (data != NULL)
    {
      pointer_reset(data);
    }
    if (data2 != NULL)
    {
      pointer_reset(data2);
    }
    if (data3 != NULL)
    {
      pointer_reset(data3);
    }
    if (message2 != NULL)
    {
      pointer_reset(message2);
    }
    if (message3 != NULL)
    {
      pointer_reset(message3);
    }
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }

  // define macros
  #define pointer_reset_all \
  free(data); \
  data = NULL; \
  free(data2); \
  data2 = NULL; \
  free(data3); \
  data3 = NULL; \
  free(message2); \
  message2 = NULL; \
  free(message3); \
  message3 = NULL;

  #define SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES(settings) \
  color_print(settings,"red"); \
  pointer_reset_all; \
  return 0;

  // verify the data
  if (verify_data(message,0,0,0) == 0)
  {   
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES("Could not verify data\nFunction: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes\nReceived Message: CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES");
  }

  // parse the data
  memset(message,0,strnlen(message,BUFFER_SIZE));
  if (parse_json_data(message,"nodes_name_list",data) == 0 || parse_json_data(message,"nodes_public_address_list",data2) == 0 || parse_json_data(message,"nodes_IP_address_list",data3) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES("Could not parse data\nFunction: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes\nReceived Message: CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES");
  }

  // update the node list and load the nodes list into the global variables  
  write_file(data,NODES_NAME_LIST_FILE_NAME); 
  memset(data,0,strnlen(data,BUFFER_SIZE));
  sprintf(data,"%ld",time(NULL));  
  write_file(data2,NODES_PUBLIC_ADDRESS_LIST_FILE_NAME);      
  write_file(data3,NODES_IP_ADDRESS_LIST_FILE_NAME);
  write_file(data,NODES_IP_ADDRESS_LIST_FILE_NAME);  

  // clear any data that was already in the block_verifiers_list struct
  for (count = 0; count < BLOCK_VERIFIERS_AMOUNT; count++)
  {
    memset(block_verifiers_list.block_verifiers_name[count],0,strnlen(block_verifiers_list.block_verifiers_name[count],BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    memset(block_verifiers_list.block_verifiers_public_address[count],0,strnlen(block_verifiers_list.block_verifiers_public_address[count],XCASH_WALLET_LENGTH));
    memset(block_verifiers_list.block_verifiers_IP_address[count],0,strnlen(block_verifiers_list.block_verifiers_IP_address[count],BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
  }

  // load all of the data into the block_verifiers_list struct
  for (count = 0, count2 = 1; count < BLOCK_VERIFIERS_AMOUNT; count++, count2++)
  {
    memset(message2,0,strnlen(message2,BUFFER_SIZE));
    memcpy(message2,"node",4);
    sprintf(message2+4,"%zu",count2);
    if (parse_json_data(data,message2,message3) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES("Could not parse data\nFunction: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes\nReceived Message: CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES");
    }
    memcpy(block_verifiers_list.block_verifiers_name[count],message3,strnlen(message3,BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    memset(message3,0,strnlen(message3,BLOCK_VERIFIERS_NAME_TOTAL_LENGTH));
    if (parse_json_data(data2,message2,message3) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES("Could not parse data\nFunction: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes\nReceived Message: CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES");
    }
    memcpy(block_verifiers_list.block_verifiers_public_address[count],message3,strnlen(message3,XCASH_WALLET_LENGTH));
    memset(message3,0,strnlen(message3,XCASH_WALLET_LENGTH));
    if (parse_json_data(data3,message2,message3) == 0)
    {
      SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES("Could not parse data\nFunction: server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes\nReceived Message: CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES");
    }
    memcpy(block_verifiers_list.block_verifiers_IP_address[count],message3,strnlen(message3,BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
    memset(message3,0,strnlen(message3,BLOCK_VERIFIERS_IP_ADDRESS_TOTAL_LENGTH));
  }

  pointer_reset(data);
  return 1;

  #undef pointer_reset_all
  #undef SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_MESSAGE_LIST_OF_ENABLED_NODES
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_consensus_node_to_node_and_main_node_restart
Description: Runs the code when the server receives the CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_NEW_PART_OF_ROUND, CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_NEXT_ROUND, CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_ROUND_CHANGE, CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_CONSENSUS_NODE_CREATE_NEW_BLOCK_MESSAGE or CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_RECALCULATING_VOTES message
Parameters:
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_consensus_node_to_node_and_main_node_restart(char* message)
{
  // define macros
  #define SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_RESTART(settings) \
  color_print(settings,"red"); \
  return 0;

  // verify the data
  if (verify_data(message,0,0,0) == 0)
  {   
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_RESTART("Could not verify data\nFunction: server_receive_data_socket_consensus_node_to_node_and_main_node_restart");
  }

  // set the current_round_part, current_round_part_backup_node and server message, this way the node will start at the begining of a round
  memset(current_round_part,0,strnlen(current_round_part,BUFFER_SIZE));
  memset(current_round_part_backup_node,0,strnlen(current_round_part_backup_node,BUFFER_SIZE));
  memcpy(current_round_part,"1",1);
  memcpy(current_round_part_backup_node,"0",1);
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS|CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND",96);
  
  return 1;

  #undef SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_RESTART
}



/*
-----------------------------------------------------------------------------------------------------------
Name: server_receive_data_socket_consensus_node_to_node_and_main_node_message_consensus_node_change
Description: Runs the code when the server receives the CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_CONSENSUS_NODE_CHANGE message
Parameters:
  message - The message
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int server_receive_data_socket_consensus_node_to_node_and_main_node_message_consensus_node_change(char* message)
{
  // define macros
  #define SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_CONSENSUS_NODE_CHANGE(settings) \
  color_print(settings,"red"); \
  return 0;

  // verify the data
  if (verify_data(message,0,0,0) == 0)
  {   
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_CONSENSUS_NODE_CHANGE("Could not verify data\nFunction: server_receive_data_socket_consensus_node_to_node_and_main_node_message_consensus_node_change\nReceived Message: CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_CONSENSUS_NODE_CHANGE");
  }

  // parse the data
  memset(current_consensus_nodes_IP_address,0,strnlen(current_consensus_nodes_IP_address,BUFFER_SIZE));
  if (parse_json_data(message,"consensus_node_public_address",current_consensus_nodes_IP_address) == 0)
  {
    SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_CONSENSUS_NODE_CHANGE("Could not parse data\nFunction: server_receive_data_socket_consensus_node_to_node_and_main_node_restart");
  }

  // set the current_round_part, current_round_part_backup_node and server message, this way the node will start at the begining of a round
  memset(current_round_part,0,strnlen(current_round_part,BUFFER_SIZE));
  memset(current_round_part_backup_node,0,strnlen(current_round_part_backup_node,BUFFER_SIZE));
  memcpy(current_round_part,"1",1);
  memcpy(current_round_part_backup_node,"0",1);
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE));
  memcpy(server_message,"CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS|CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND",96);

  return 1;

  #undef SERVER_RECEIVE_DATA_SOCKET_CONSENSUS_NODE_TO_NODE_AND_MAIN_NODE_MESSAGE_CONSENSUS_NODE_CHANGE
}



/*
-----------------------------------------------------------------------------------------------------------
Name: create_server
Description: Creates the server
Parameters:
   MESSAGE_SETTINGS - 1 to print the messages, otherwise 0. This is used for the testing flag to not print any success or error messages
Return: 0 if an error has occured, 1 if successfull
-----------------------------------------------------------------------------------------------------------
*/

int create_server(const int MESSAGE_SETTINGS)
{
  // Constants
  const int SOCKET_OPTION = 1; 

  // Variables
  char buffer[BUFFER_SIZE];
  char buffer2[BUFFER_SIZE];
  char client_address[BUFFER_SIZE];  
  char* string = (char*)calloc(BUFFER_SIZE,sizeof(char)); 
  int len;
  int receive_data_result; 
  struct sockaddr_in addr, cl_addr; 
  struct mainnode_timeout_thread_parameters mainnode_timeout_thread_parameters;
  struct node_to_node_timeout_thread_parameters node_to_node_timeout_thread_parameters;

  // define macros
  #define SOCKET_FILE_DESCRIPTORS_LENGTH 1

  #define pointer_reset_all \
  free(string); \
  string = NULL;

  /* Reset the node so it is ready for the next round.
  close the client socket
  reset the variables for the forked process
  reset the current_round_part to 1 and current_round_part_backup_node to 0
  reset the server_message to CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND|CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS
  exit the forked process
  this way the node will sit out the current round, and start the next round.
  */
  #define SERVER_ERROR(settings) \
  close(SOCKET); \
  pointer_reset_all; \
  memset(current_round_part,0,strnlen(current_round_part,BUFFER_SIZE)); \
  memset(current_round_part_backup_node,0,strnlen(current_round_part_backup_node,BUFFER_SIZE)); \
  memcpy(current_round_part,"1",1); \
  memcpy(current_round_part_backup_node,"0",1); \
  memset(server_message,0,strnlen(server_message,BUFFER_SIZE)); \
  memcpy(server_message,"CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND|CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS",96); \
  if (settings == 0) \
  { \
    return 0; \
  } \
  else \
  { \
    _exit(0); \
  }  
  
  // threads
  pthread_t thread_id;

  // set the main process to ignore if forked processes return a value or not, since the timeout for the total connection time is run on a different thread
  signal(SIGCHLD, SIG_IGN);  

  // check if the memory needed was allocated on the heap successfully
  if (string == NULL)
  {
    color_print("Could not allocate the memory needed on the heap","red");
    exit(0);
  }
    
  /* Create the socket  
  AF_INET = IPV4 support
  SOCK_STREAM = TCP protocol
  */
  const int SOCKET = socket(AF_INET, SOCK_STREAM, 0);
  if (SOCKET == -1)
  {
    if (MESSAGE_SETTINGS == 1)
    {
      color_print("Error creating socket","red");
    }    
    pointer_reset_all;
    return 0;
  }

  /* Set the socket options
  SOL_SOCKET = socket level
  SO_REUSEADDR = allows for reuse of the same address, so one can shutdown and restart the program without errros
  SO_REUSEPORT = allows for reuse of the same port, so one can shutdown and restart the program without errros
  */
  if (setsockopt(SOCKET, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &SOCKET_OPTION,sizeof(int)) != 0)
  {
    if (MESSAGE_SETTINGS == 1)
    {
      color_print("Error setting socket options","red"); 
    }
    SERVER_ERROR(0);
  } 
  if (MESSAGE_SETTINGS == 1)
  {
    color_print("Socket created","green");
  }
 
  // convert the port to a string
  sprintf(buffer2,"%d",SEND_DATA_PORT);  
 
  memset(&addr, 0, sizeof(addr));
  /* setup the connection
  AF_INET = IPV4
  INADDR_ANY = connect to 0.0.0.0
  use htons to convert the port from host byte order to network byte order short
  */
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(SEND_DATA_PORT);
 
  // connect to 0.0.0.0
  if (bind(SOCKET, (struct sockaddr *) &addr, sizeof(addr)) != 0)
  {
   if (MESSAGE_SETTINGS == 1)
   {
     memset(string,0,strnlen(string,BUFFER_SIZE));
     memcpy(string,"Error connecting to port ",25);
     memcpy(string+25,buffer2,strnlen(buffer2,BUFFER_SIZE));
     color_print(string,"red"); 
   }
   SERVER_ERROR(0);
  } 
  if (MESSAGE_SETTINGS == 1)
  {
    memset(string,0,strnlen(string,BUFFER_SIZE));
    memcpy(string,"Connected to port ",18);
    memcpy(string+18,buffer2,strnlen(buffer2,BUFFER_SIZE));
    color_print(string,"green");

    printf("Waiting for a connection...\n");
  }

  // set the maximum simultaneous connections
  if (listen(SOCKET, MAXIMUM_CONNECTIONS) != 0)
  {
    if (MESSAGE_SETTINGS == 1)
    {
      color_print("Error creating the server","red"); 
    }
    SERVER_ERROR(0);
  }

  for (;;)
  {
    len = sizeof(cl_addr);
    const int CLIENT_SOCKET = accept(SOCKET, (struct sockaddr *) &cl_addr, (socklen_t*)&len);
    inet_ntop(AF_INET, &(cl_addr.sin_addr), client_address, BUFFER_SIZE);
    if (client_address == NULL)
    {
      if (MESSAGE_SETTINGS == 1)
      {
        color_print("Error accepting connection","red"); 
      }
      continue;
    }
    const size_t CLIENT_ADDRESS_LENGTH = strnlen(client_address,BUFFER_SIZE);
    const size_t BUFFER2_LENGTH = strnlen(buffer2,BUFFER_SIZE);
  
    if (CLIENT_SOCKET == -1)
    {
      if (MESSAGE_SETTINGS == 1)
      {
        memset(string,0,strnlen(string,BUFFER_SIZE));
        memcpy(string,"Error accepting connection from ",32);
        memcpy(string+32,inet_ntoa(cl_addr.sin_addr),CLIENT_ADDRESS_LENGTH);
        memcpy(string+32+CLIENT_ADDRESS_LENGTH," on ",4);
        memcpy(string+36+CLIENT_ADDRESS_LENGTH,buffer2,strnlen(buffer2,BUFFER_SIZE));
        color_print(string,"red"); 
      }
      continue;
    }
    if (MESSAGE_SETTINGS == 1)
    {
      memset(string,0,strnlen(string,BUFFER_SIZE));
      memcpy(string,"Connection accepted from ",25);
      memcpy(string+25,inet_ntoa(cl_addr.sin_addr),CLIENT_ADDRESS_LENGTH);
      memcpy(string+25+CLIENT_ADDRESS_LENGTH," on ",4);
      memcpy(string+29+CLIENT_ADDRESS_LENGTH,buffer2,strnlen(buffer2,BUFFER_SIZE));
      color_print(string,"green"); 
    }

   

    if (fork() == 0)
    {     
      // create a struct for the parameters
      struct total_connection_time_thread_parameters parameters = {
        getpid(),
        client_address,
        buffer2,
        0,
        (int)MESSAGE_SETTINGS
      };
          // create a timeout for this connection, since we need to limit the amount of time a client has to send data from once it connected
         if (pthread_create(&thread_id, NULL, &total_connection_time_thread, (void *)&parameters) != 0)
         {
           // close the forked process
           SERVER_ERROR(1);
         }
         // set the thread to dettach once completed, since we do not need to use anything it will return.
         if (pthread_detach(thread_id) != 0)
         {
           // close the forked process
           SERVER_ERROR(1);
         }
      

       // close the main socket, since the socket is now copied to the forked process
       close(SOCKET); 

       for (;;)
       {         
         // receive the data
         memset(buffer, 0, BUFFER_SIZE); 
         receive_data_result = receive_data(CLIENT_SOCKET,buffer,SOCKET_END_STRING,0,TOTAL_CONNECTION_TIME_SETTINGS);
         if (receive_data_result < 2)
         {
           if (MESSAGE_SETTINGS == 1)
           {
             memset(string,0,strnlen(string,BUFFER_SIZE));
             memcpy(string,"Error receiving data from ",26);
             memcpy(string+26,client_address,CLIENT_ADDRESS_LENGTH);
             memcpy(string+26+CLIENT_ADDRESS_LENGTH," on port ",9);
             memcpy(string+35+CLIENT_ADDRESS_LENGTH,buffer2,BUFFER2_LENGTH);
             if (receive_data_result == 1)
             {
               memcpy(string+35+CLIENT_ADDRESS_LENGTH+BUFFER2_LENGTH,", because of a timeout issue",28);
             }
             else if (receive_data_result == 0)
             {
               memcpy(string+35+CLIENT_ADDRESS_LENGTH+BUFFER2_LENGTH,", because of a potential buffer overflow issue",46);
             }
             color_print(string,"red"); 
           }
           // close the forked process, since the client had an error sending data       
           SERVER_ERROR(1);
         }
         else if (receive_data_result == 2)
         {
          // update the parameters, since we have received data from the client
          parameters.data_received = 1;
         }    



         // check if a certain type of message has been received         
         if (strstr(buffer,"\"message_settings\": \"XCASH_PROOF_OF_STAKE_TEST_DATA\"") != NULL && strncmp(server_message,"XCASH_PROOF_OF_STAKE_TEST_DATA",BUFFER_SIZE) == 0)
         {
           // close the forked process when done
           server_received_data_xcash_proof_of_stake_test_data(CLIENT_SOCKET,buffer);
           SERVER_ERROR(1);
         }
         else if (strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS\"") != NULL && strstr(server_message,"CONSENSUS_NODE_TO_NODES_MAIN_NODE_PUBLIC_ADDRESS") != NULL)
         {
           // only close the forked process on the timeout in the mainnode_timeout_thread
           // create a mainnode_timeout_thread_parameters struct since this function will use the mainnode_timeout_thread
           mainnode_timeout_thread_parameters.process_id = getpid();
           mainnode_timeout_thread_parameters.data_received = 0;
           mainnode_timeout_thread_parameters.main_node = "";
           mainnode_timeout_thread_parameters.current_round_part = "";
           mainnode_timeout_thread_parameters.current_round_part_backup_node = "";
           if (server_receive_data_socket_consensus_node_to_node(&mainnode_timeout_thread_parameters,buffer) == 0)
           {
             SERVER_ERROR(1);
           }           
         } 
         else if (strstr(buffer,"\"message_settings\": \"MAIN_NODES_TO_NODES_PART_1_OF_ROUND\"") != NULL && strstr(server_message,"MAIN_NODES_TO_NODES_PART_1_OF_ROUND") != NULL)
         {
           // only close the forked process on the timeout in the node_to_node_timeout_thread
           // create a node_to_node_timeout_thread_parameters struct since this function will use the node_to_node_timeout_thread
           node_to_node_timeout_thread_parameters.process_id = getpid();
           if (server_receive_data_socket_main_node_to_node_message_part_1(&mainnode_timeout_thread_parameters,&node_to_node_timeout_thread_parameters,buffer) == 0)
           {
             SERVER_ERROR(1);
           }
         } 
         else if (strstr(buffer,"\"message_settings\": \"MAIN_NODES_TO_NODES_PART_2_OF_ROUND\"") != NULL && strstr(server_message,"MAIN_NODES_TO_NODES_PART_2_OF_ROUND") != NULL)
         {
           // only close the forked process on the timeout in the node_to_node_timeout_thread
           // create a node_to_node_timeout_thread_parameters struct since this function will use the node_to_node_timeout_thread
           node_to_node_timeout_thread_parameters.process_id = getpid();
           if (server_receive_data_socket_main_node_to_node_message_part_2(&mainnode_timeout_thread_parameters,&node_to_node_timeout_thread_parameters,buffer) == 0)
           {
             SERVER_ERROR(1);
           }
         } 
         else if (strstr(buffer,"\"message_settings\": \"MAIN_NODES_TO_NODES_PART_3_OF_ROUND\"") != NULL && strstr(server_message,"MAIN_NODES_TO_NODES_PART_3_OF_ROUND") != NULL)
         {
           // only close the forked process on the timeout in the node_to_node_timeout_thread
           // create a node_to_node_timeout_thread_parameters struct since this function will use the node_to_node_timeout_thread
           node_to_node_timeout_thread_parameters.process_id = getpid();
           if (server_receive_data_socket_main_node_to_node_message_part_3(&mainnode_timeout_thread_parameters,&node_to_node_timeout_thread_parameters,buffer) == 0)
           {
             SERVER_ERROR(1);
           }
         } 
         else if (strstr(buffer,"\"message_settings\": \"MAIN_NODES_TO_NODES_PART_4_OF_ROUND\"") != NULL && strstr(server_message,"MAIN_NODES_TO_NODES_PART_4_OF_ROUND") != NULL)
         {
           // only close the forked process on the timeout in the node_to_node_timeout_thread
           // create a node_to_node_timeout_thread_parameters struct since this function will use the node_to_node_timeout_thread
           node_to_node_timeout_thread_parameters.process_id = getpid();
           if (server_receive_data_socket_main_node_to_node_message_part_4(&mainnode_timeout_thread_parameters,&node_to_node_timeout_thread_parameters,buffer) == 0)
           {
             SERVER_ERROR(1);
           }
         } 
         if (strstr(buffer,"\"message_settings\": \"NODES_TO_NODES_VOTE_RESULTS\"") != NULL && strstr(server_message,"NODES_TO_NODES_VOTE_RESULTS") != NULL)
         {
           // close the forked process when done
           server_receive_data_socket_node_to_node(buffer);
           SERVER_ERROR(1);
         }
         else if (strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND\"") != NULL && strstr(server_message,"CONSENSUS_NODE_TO_MAIN_NODE_START_PART_OF_ROUND") != NULL)
         {
           // close the forked process when done
           server_receive_data_socket_consensus_node_to_main_node_message_start_part_of_round(buffer);
           SERVER_ERROR(1);
         } 
         else if (strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_LIST_OF_ENABLED_NODES\"") != NULL)
         {
           // close the forked process when done
           server_receive_data_socket_consensus_node_to_node_message_list_of_enabled_nodes(buffer);
           SERVER_ERROR(1);
         } 
         else if (strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_NEW_PART_OF_ROUND\"") != NULL || strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_NEXT_ROUND\"") != NULL || strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_ROUND_CHANGE\"") != NULL || strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_CONSENSUS_NODE_CREATE_NEW_BLOCK_MESSAGE\"") != NULL || strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_RECALCULATING_VOTES\"") != NULL)
         {
           if (server_receive_data_socket_consensus_node_to_node_and_main_node_restart(buffer) == 0)
           {
             SERVER_ERROR(1);
           }
           // close the server
           break;           
         } 
         else if (strstr(buffer,"\"message_settings\": \"CONSENSUS_NODE_TO_NODES_AND_MAIN_NODES_CONSENSUS_NODE_CHANGE\"") != NULL)
         {
           if (server_receive_data_socket_consensus_node_to_node_and_main_node_message_consensus_node_change(buffer) == 0)
           {
             SERVER_ERROR(1);
           }
           // close the server
           break;           
         } 
         else
         {
           printf("Received %s from %s on port %s\r\n",buffer,client_address,buffer2);
           // send the message 
           if (send_data(CLIENT_SOCKET,buffer,1) == 1)
           {
             printf("Sent %s to %s on port %s\r\n",buffer,client_address,buffer2);
           } 
           else
           {
             memset(string,0,strnlen(string,BUFFER_SIZE));
             memcpy(string,"Error sending data to ",22);
             memcpy(string+22,client_address,CLIENT_ADDRESS_LENGTH);
             memcpy(string+22+CLIENT_ADDRESS_LENGTH," on port ",9);
             memcpy(string+31+CLIENT_ADDRESS_LENGTH,buffer2,BUFFER2_LENGTH);
             color_print(string,"red"); 
             continue;
           } 
         }

         
       
       }
     }   
     else
     {
       // if the process did not fork, close the client socket
       close(CLIENT_SOCKET);
     } 
   }
   return 1;

   #undef pointer_reset_all
   #undef SERVER_ERROR
}

