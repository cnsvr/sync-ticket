#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <pthread.h>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

#define NUM_OF_TELLER 3
#define MAX_OF_CLIENT 300
#define ODA_TIYATROSU 60
#define USKUDAR_STUDYO_SAHNE 80
#define KUCUK_SAHNE 200 


// Status of Teller
enum TellerStatus{
  idle,
  busy
};


// Mutexes
std::mutex m1;
std::mutex m2;
std::mutex m3;


// Structure of Client
struct Client{
  int id;
  std::string name;
  int arrivalTime;
  int serviceTime;
  int requestedSeat;
  int status;
} typedef Client;


// Structure OF Teller
struct Teller{
  int id;
  int status;
  int clientNum;
} typedef Teller;


using namespace std;
string inputPath, outputPath;             // path of input and output file
pthread_t tellers[NUM_OF_TELLER];         // array of teller threads
pthread_t clients[MAX_OF_CLIENT];         // array of client threads
Client clientsOfTheatre[MAX_OF_CLIENT];   // array of Client structure in the theater
Teller tellersOfTheatre[NUM_OF_TELLER];   // array of Teller structure in the theater
int theatreSeatStatus[KUCUK_SAHNE] = {0}; // All seats are empty and its size is biggest size(KUCUK_SAHNE)
string theater, client, clientNum;        // name of theater, name and number of client
int clientNumber;                         // number of client coming the theater
int soldTicket = 0;                       // number of sold tickets
int currentTheaterCapacity;               // capacity of theater (60, 80 ,200)
int numberOfDoneClient = 0;               // number of clients whose ticket process are handled so far.


// Splits the string with given delimeter and returns the vector.
vector<string> split(const string str, char delim);


// Runs in the teller thread.
void *tellerRunner(void *num);

// Runs in the client thread.
void *clientRunner(void *num);

// Write the message to output file. 
void writeFile(string message);

// Finds the idle teller 
int findIdleTeller();

// Find the empty seat.
int findSeat(int requestedSeat);

// Sleep thread in millisecond. Source: https://stackoverflow.com/a/1157217
int msleep(long msec){
    struct timespec ts;
    int res;

    if(msec < 0){
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    do {
        res = nanosleep(&ts,&ts);
    } while (res);
    return res;
}

int main(int argc, char *argv[]){
  if (argc < 3) {
    printf("Usage of this executable file: %s %s %s\n", argv[0], "configuration_path", "output_path");
    return 0;
  }

  inputPath = argv[1];  // path of input file
  outputPath = argv[2]; // path of output file
  
  
  // Read the file and create cilent threads
  ifstream config_file (inputPath);
 
  if (config_file.is_open()) {
    getline(config_file,theater); // Reads first line and gets the theater name.
    getline(config_file,clientNum); // Reads seconde line and gets the number of clients
    clientNumber = stoi(clientNum);
    

    for (int i = 0; i < clientNumber; i++)
    {
      getline(config_file,client);  // Reads line by line
      vector<string> clientInfo = split(client, ',');
      clientsOfTheatre[i].id = i + 1;
      clientsOfTheatre[i].name = clientInfo[0];
      clientsOfTheatre[i].arrivalTime = stoi(clientInfo[1]);
      clientsOfTheatre[i].serviceTime = stoi(clientInfo[2]);
      clientsOfTheatre[i].requestedSeat = stoi(clientInfo[3]);
    }


    // Initialize the currentTheaterCapacity.
    if(theater == "OdaTiyatrosu") {
      currentTheaterCapacity = 60;
    } else if (theater == "UskudarStudyoSahne") {
      currentTheaterCapacity = 80;
    } else {
      currentTheaterCapacity = 200;
    }
  }else {
    printf("Unable to open the input file");
    return 0;
  }

  writeFile("Welcome to the Sync-Ticket!\n");
  
  // Creating teller threads
  int tellersID[NUM_OF_TELLER];
  for (int i = 0; i < NUM_OF_TELLER; i++)
  {
    tellersID[i] = i;
    tellersOfTheatre[i].id = i + 1;
    tellersOfTheatre[i].status = idle;
    pthread_create(&tellers[i], NULL, tellerRunner, (void *)(tellersID + i));
    msleep(500);
  }
  
  // Creating client threads
  int clientsID[clientNumber];
  for (int i = 0; i < clientNumber; i++)
  {
    clientsID[i] = i;
    pthread_create(&clients[i], NULL, clientRunner, (int *)(clientsID + i));
  }

  // Joining all thread to be ready for terminating program.
  for (int i = 0; i < clientNumber; i++)
  {
    pthread_join(clients[i], NULL);
  }

  for (int i = 0; i < NUM_OF_TELLER; i++)
  {
    pthread_join(tellers[i],NULL);
  }
  
  writeFile("All clients received service.");
  
  return 0;
  
}



// Runs in the teller thread.
void *tellerRunner(void *num){
  
  int number = *(int *) num;    // Teller number.
  string tellerName = tellersOfTheatre[number].id == 1  ? "A" :
                      tellersOfTheatre[number].id == 2  ? "B" :
                      tellersOfTheatre[number].id == 3  ? "C" :
                      "unknown";

  writeFile("Teller " + tellerName + " has arrived.\n");


  while (1) {

    // Waits if status of teller is idle
    int status = tellersOfTheatre[number].status;
    while(status == idle) {
      status = tellersOfTheatre[number].status;
      
      /* 
        Breaks when sold tickets equals capacity of theater or 
        clients came to the theater equals the number of clients whose ticket process are handled so far.
      */
      if (soldTicket == currentTheaterCapacity || clientNumber == numberOfDoneClient){
        break;
      }
    }
    
    // If theater is busy, it will deal with the client.
    if (status != idle){

      int clientNum = tellersOfTheatre[number].clientNum; // client at service of Teller
      int requestedSeat = clientsOfTheatre[clientNum - 1].requestedSeat; // requested seat number
      
      // Two teller can look at empty seat at the same time, so lock this section. (CRITICAL SECTION)
      m2.lock();
      int findSeatNumber = findSeat(requestedSeat); // finds the empty seat number.
      m2.unlock();

      // If teller can find the empty seat, it will give the seat to the client.
      if (findSeatNumber != -1) {
        // Requested ticket is given the client
        msleep(clientsOfTheatre[clientNum - 1].serviceTime*10);
        writeFile(clientsOfTheatre[clientNum - 1].name + " requests seat " + to_string(requestedSeat) + ", reserves seat " + to_string(findSeatNumber) + ". Signed by Teller " + tellerName +".\n");

      } else {
        // requestedSeat and any empty seat is not found, so teller couldn't give the any seat to the client.
        msleep(clientsOfTheatre[clientNum - 1].serviceTime*10);
        writeFile(clientsOfTheatre[clientNum - 1].name + " requests seat " + to_string(requestedSeat) + ", reserves None. Signed by Teller " + tellerName + ".\n");
      }

      // Change the status of teller as IDLE after process is finished.
      tellersOfTheatre[number].status = idle;
    }

    // If all clients' processes are done, we can terminate the teller thread.
    if (clientNumber == numberOfDoneClient) { 
      // printf("%s","Break enter");
      break;
    }
  }
  // printf("%s is finished\n", tellerName.c_str());
  pthread_exit(NULL);
}


// Runs in the client thread.
void *clientRunner(void *clientNum) {
  int clientNumber = *(int *) clientNum;    // Client number.
  
  // Sleeps until arrival time.
  msleep(clientsOfTheatre[clientNumber].arrivalTime*10);
  
  int idleTeller = 0;
  /* 
    Check which teller is idle, change the status as busy and lock the until the process is finished
    Returns same number for two threads so while looking who's idle, locks this section. (CRITICAL SECTION)
  */
  m1.lock();
  while (idleTeller == 0) {
    
    idleTeller = findIdleTeller();
    if (idleTeller){
      tellersOfTheatre[idleTeller - 1].clientNum = clientsOfTheatre[clientNumber].id; // client and teller at service.
      tellersOfTheatre[idleTeller - 1].status = busy; // make the teller busy
      numberOfDoneClient++;
      break;
    }
  }
  m1.unlock();
  msleep(clientsOfTheatre[clientNumber].serviceTime*10);
  
  pthread_exit(NULL);
}


// Finds the idle teller, otherwise returns 0.
int findIdleTeller() {
  int idleTeller = 0;
  if (tellersOfTheatre[0].status == idle){
    idleTeller = 1;
  }else if(tellersOfTheatre[1].status == idle){
    idleTeller = 2;
  }else if(tellersOfTheatre[2].status == idle){
    idleTeller = 3;
  }
  return idleTeller;
}

// Finds the empty seat number, otherwise returns -1.
int findSeat(int requestedSeat) {
  int emptySeatNumber = -1;
  
  if (theater == "OdaTiyatrosu") { 
    if (theatreSeatStatus[requestedSeat - 1] == 0 && requestedSeat < ODA_TIYATROSU) {  // Checks if requested seat is empty.
      theatreSeatStatus[requestedSeat - 1] = 1;  // it is reserved.
      soldTicket++;
      return requestedSeat;
    }

    for (int i = 0; i < ODA_TIYATROSU; i++)
    {
      if(theatreSeatStatus[i] == 0) { // checks if it is empty;
        theatreSeatStatus[i] = 1; // it is reserved.
        emptySeatNumber = i + 1; // reserved seat number
        soldTicket++;
        
        break;
      }
    }
  } else if(theater == "UskudarStudyoSahne") {
    if (theatreSeatStatus[requestedSeat - 1] == 0 && requestedSeat < USKUDAR_STUDYO_SAHNE) {  // Checks if requested seat is empty.
      theatreSeatStatus[requestedSeat - 1] = 1;  // it is reserved.
      soldTicket++;
      return requestedSeat;
    }

    for (int i = 0; i < USKUDAR_STUDYO_SAHNE; i++)
    {
      if(theatreSeatStatus[i] == 0) { // checks if it is empty;
        theatreSeatStatus[i] = 1; // it is reserved.
        emptySeatNumber = i + 1; // reserved seat number
        soldTicket++;
        break;
      }
    }
  } else {

   if (theatreSeatStatus[requestedSeat - 1] == 0 && requestedSeat < KUCUK_SAHNE) {  // Checks if requested seat is empty.
      theatreSeatStatus[requestedSeat - 1] = 1;  // it is reserved.
      soldTicket++;
      return requestedSeat;
    }

    for (int i = 0; i < KUCUK_SAHNE; i++)
    {
      if(theatreSeatStatus[i] == 0) { // checks if it is empty;
        theatreSeatStatus[i] = 1; // it is reserved.
        emptySeatNumber = i + 1; // reserved seat number
        soldTicket++;
        break;
      }
    }
  }
  return emptySeatNumber;
}

// Splits the string with given delimeter and returns the vector.
vector<string> split(const string str, char delim)
{
  vector<string> result;
  istringstream ss(str);
  string token;
  while (std::getline(ss, token, delim)) {
    if (!token.empty()) {
      result.push_back(token);
    }
  }
  return result;
}

// Write the message to output file. 
void writeFile(string message) {
  
  m3.lock();
  ofstream outputFile;
  outputFile.open(outputPath, outputFile.out | outputFile.app);
  outputFile << message;
  outputFile.close();
  m3.unlock();

}