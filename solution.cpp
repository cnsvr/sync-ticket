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


enum TellerStatus{
  idle,
  busy
};


// Getting the mutex
std::mutex m1;
std::mutex m2;
std::mutex m3;

struct Client{
  int id;
  std::string name;
  int arrivalTime;
  int serviceTime;
  int requestedSeat;
  int status;
} typedef Client;

struct Teller{
  int id;
  int status;
  int clientNum;
} typedef Teller;


using namespace std;
string inputPath, outputPath;
pthread_t tellers[NUM_OF_TELLER]; // array of teller threads
pthread_t clients[MAX_OF_CLIENT]; // array of client threads
Client clientsOfTheatre[MAX_OF_CLIENT];
Teller tellersOfTheatre[NUM_OF_TELLER];
int theatreSeatStatus[KUCUK_SAHNE] = {0}; // All seats are empty.
string theater, client, clientNum;
int clientNumber;
int soldTicket = 0;
int currentTheaterCapacity;
int numberOfDoneClient = 0;

vector<string> split(const string str, char delim);

void *tellerRunner(void *num);
void *clientRunner(void *num);
void writeFile(string message);
int findIdleTeller();
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

  // writeFile("Welcome to the Sync-Ticket!");

  /*
  pthread_attr_init(&attr);

  ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  if (ret != 0){
    printf("Unable to set policy: %d\n", ret);
  }
  */

  
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
  
  // writeFile("All clients received service.");
  return 0;
  
}




void *tellerRunner(void *num){
  int number = *(int *) num;
  string tellerName = tellersOfTheatre[number].id == 1  ? "A" :
                      tellersOfTheatre[number].id == 2  ? "B" :
                      tellersOfTheatre[number].id == 3  ? "C" :
                      "unknown";

  // writeFile("Teller " + tellerName + " has arrived.\n");
  // writeFile(("Teller %s has arrived.\n", tellerName.c_str()));
  printf("Teller %s has arrived.\n", tellerName.c_str());
  // TODO: Waits until the client is in the queue

  
  while (1) {

    // Waits if status is idle
    int status = tellersOfTheatre[number].status;
    while(status == idle) {
      status = tellersOfTheatre[number].status;
      if (soldTicket == currentTheaterCapacity){
        break;
      }
    }
    if (status != idle){

      int clientNum = tellersOfTheatre[number].clientNum; // client at service of Teller
      // printf("clientNum %d and Teller %s \n", clientNum,tellerName.c_str());
      int requestedSeat = clientsOfTheatre[clientNum - 1].requestedSeat; // requested seat number
      // printf("Teller %s is busy now for %s\n", tellerName.c_str(), clientsOfTheatre[clientNum - 1].name.c_str());
      // TODO: Check if request seat is empty or find possible empty seat
      
      // Two teller can look at empty seat at the same time, so lock this section.
      m2.lock();
      int findSeatNumber = findSeat(requestedSeat);
      m2.unlock();
      if (findSeatNumber != -1) {
        // Requested ticket is given the client
        msleep(clientsOfTheatre[clientNum - 1].serviceTime*10);
        // printf("ClientNum %d\n",clientNum);
        // writeFile(clientsOfTheatre[clientNum - 1].name + " requests seat " + to_string(requestedSeat) + ", reserves seat " + to_string(findSeatNumber) + ". Signed by Teller " + tellerName +".\n");
        printf("%s requests seat %d, reserves seat %d. Signed by Teller %s.\n", clientsOfTheatre[clientNum - 1].name.c_str(), requestedSeat, findSeatNumber, tellerName.c_str());
        // tellersOfTheatre[number].status = idle;
      } else {
        msleep(clientsOfTheatre[clientNum - 1].serviceTime*10);
        // writeFile(clientsOfTheatre[clientNum - 1].name + " requests seat" + to_string(requestedSeat) + ", reserves None. Signed by Teller" + tellerName + ".\n");
        printf("%s requests seat %d, reserves None. Signed by Teller %s.\n", clientsOfTheatre[clientNum - 1].name.c_str(), requestedSeat, tellerName.c_str());
        // tellersOfTheatre[number].status = idle;
      }
      tellersOfTheatre[number].status = idle;
    }
    // printf("sold ticket %d\n", soldTicket );

    // printf("%d %d %d\n",findSeatNumber, soldTicket, clientNumber);
    if (clientNumber == numberOfDoneClient) { // No more ticket left.
      // printf("%s","Break enter");
      break;
    }
  }
  // printf("%s is finished\n", tellerName.c_str());
  pthread_exit(NULL);
}

void *clientRunner(void *clientNum) {
  int clientNumber = *(int *) clientNum;
  // printf("ClientNumber %d in clientRunner", clientNumber);
  
  // Sleeps until arrival time.
  msleep(clientsOfTheatre[clientNumber].arrivalTime*10);
  // printf("Client%d is arrived\n", clientsOfTheatre[clientNumber].id);
  
  int idleTeller = 0;
  m1.lock();
  while (idleTeller == 0) {
    // Check which teller is idle, change the status as busy and lock the until the process is finished
    // Returns same number for two threads so while looking who's idle, locks this section.
    idleTeller = findIdleTeller();
    // printf("Client%d, idleTeller %d\n", clientsOfTheatre[clientNumber].id,idleTeller);
    if (idleTeller){
      // printf("Client%d and teller %d\n",clientsOfTheatre[clientNumber].id, idleTeller);
      tellersOfTheatre[idleTeller - 1].clientNum = clientsOfTheatre[clientNumber].id; // client and teller at service.
      tellersOfTheatre[idleTeller - 1].status = busy; // make the teller busy
      numberOfDoneClient++;
      // tellersOfTheatre[idleTeller - 1].status = idle;
      break;
    }
  }
  m1.unlock();
  msleep(clientsOfTheatre[clientNumber].serviceTime*10);
  
  // printf("%s is finished\n",clientsOfTheatre[clientNumber].name.c_str());
  pthread_exit(NULL);
}

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

int findSeat(int requestedSeat) {
  int emptySeatNumber = -1;
  
  if (theater == "OdaTiyatrosu") { // OdaTiyatrosu, UskudarStudyoSahne, and KucukSahne
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

void writeFile(string message) {
  
  m3.lock();
  ofstream outputFile;
  outputFile.open(outputPath, outputFile.out | outputFile.app);
  outputFile << message;
  outputFile.close();
  m3.unlock();

}