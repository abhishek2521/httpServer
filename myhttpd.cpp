/*
 * myhttpd.cpp
 *
 *  Created on: Oct 29, 2013
 *      Author: abhishek
 */




#define         BUF_LEN        8192
#include        <iostream>
#include        <vector>
#include        <list>
#include        <stdio.h>
#include        <syslog.h>
#include        <unistd.h>
#include        <dirent.h>
#include        <stdlib.h>
#include        <string.h>
#include        <ctype.h>
#include        <termios.h>
#include        <fcntl.h>
#include        <assert.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include        <netdb.h>
#include        <netinet/in.h>
#include        <inttypes.h>
#include        <pthread.h>
#include        <semaphore.h>
#include        <time.h>
#include        <arpa/inet.h>
#include        <sys/stat.h>
#include        <fstream>
#include        <sys/ioctl.h>
#include        <errno.h>
#include        <string>
#include        <sstream>
#include        <syslog.h>
#include        <dirent.h>
#define         errorMessage(msg) { perror(msg) ; exit(1) ; }

using namespace std;
sem_t vectorMutex;
sem_t listMutex;
sem_t vectorSize;
sem_t listSize;
sem_t logMutex;
int firstComeFirstServe;  //Initializing FCFS=1
int shortestJobFirst ;       //Initializing SJF=0
int portNumber;
char *schedulingPolicy="FCFS";
int  queuingTime=60;
char *directory;
char *charDirectory;
string stringDirectory;

int directorySize=400;
struct clientRequest{
    string clientIP;
    int clientPort;
    int socketFileDescriptor;
    string clientRequestType;
    string clientRequestFileName;
    int clientRequestFileSize;
    string fileContentType;
    struct clientRequest *next;
    struct stat fileAttribute;
};
//vector<int> socketFileDescriptorVector;
list<int> socketFileDescriptorList;
list<clientRequest> clientRequestList;
list<clientRequest>::iterator  clientRequestListIterator;

void putVector(int socketFileDescriptor);
void insertClientRequestList(clientRequest *request);
void *serverSetup(void*);
void *serveClientRequest(void*);
void *schedule(void*);
void createThreadPool(int,pthread_t*,pthread_t*,pthread_t*);
void printUsage();
template <typename T>   string numberToString ( T Number );
void fileNotFound(int);
void log(char* logMessage);

void log(char* logMessage){


}

int main(int argc, char **argv)
{
        /*struct sockaddr_in socketAddress;
        The address of the socket
        struct hostent  *hostDetail;
        char hostName[256];
        int sockedId,socketFileDescriptor;*/
        int numberOfRequestHandlerThreads=4;
        string testString="Main thread entered\n ";
        pthread_t listenerThread;
        pthread_t schedulerThread;
        pthread_t *threads;
        char ch;
        int debugMode=0;
        char *logFilePointer;
        portNumber=8080;  //Test
        pid_t pid, sid;

        //Handling directory
        directory=getcwd(charDirectory, directorySize);    
        //Handling Directory Ends

        while ((ch = getopt(argc, argv, "dhl:p::r::t::n::s::")) != -1) {
                switch (ch) {
                case 'd':
                    debugMode = 1;
                    break;
                case 'h':
                    printUsage();
                    exit(1);
                case 'l':
                    logFilePointer = optarg;
                    break;
                case 'p':
                    if (isdigit(*optarg)) {
                        portNumber = atoi(optarg);
                    } else {
                        fprintf(stderr,"[error] The provided port is not an error.\n");
                        exit(1);
                    }
                    if (portNumber < 1024) {
                        fprintf(stderr,"[error] Port number must be greater than or equal to 1024.\n");
                        exit(1);
                    }
                    break;
                case 'r':
                    directory = optarg;
                    break;
                case 't':
                    queuingTime = atoi(optarg);
                    if (queuingTime < 1) {
                        fprintf(stderr, "[error] queueing time must be greater than 0.\n");
                        exit(1);
                    }
                    break;
                case 'n':
                	numberOfRequestHandlerThreads = atoi(optarg);
                    if (numberOfRequestHandlerThreads < 1) {
                        fprintf(stderr,"[error] number of threads must be greater than 0.\n");
                        exit(1);
                    }
                    break;
                case 's':
                    schedulingPolicy = optarg;
                    if (strcasecmp(schedulingPolicy,"FCFS")!=0 &&  strcasecmp(schedulingPolicy,"SJF")!=0 ){
                    exit(1);
}
                    break;
                default:
                    printUsage();
                    exit(1);
                }
            }



        /*Declaring Semaphores*/
        sem_init(&vectorMutex, 0, 1);//Initialize vectorMutex to 1
        sem_init(&listMutex, 0, 1);//Initialize listMutex to 1
        sem_init(&vectorSize, 0, 0);//Initialize vectorSize to 0
        sem_init(&listSize, 0, 0);//Initialize vectorSize to 0
        sem_init(&logMutex, 0, 1);//Initialize vectorMutex to 1
        /*Declaration of semaphore ends*/


        /*test Thread*/
        //write(1, testString.c_str(),strlen(testString.c_str()));

        
         /*Appending / towards the end if it doesnot already  exist */
        /*stringDirectory = string(directory);
        if (directory[strlen(directory)-1]!='/'){
    	   stringDirectory.append(1,'/');
        }*/
        


        //Daemon Process
       if(debugMode ==0){
        pid = fork();
      
        if (pid < 0) {
        exit(1);
         }
        
        if (pid > 0) { 
        exit(1);
         }

         umask(0);
        
        sid = setsid();
        if (sid < 0) {
         exit(1); 
         }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
       }

       //Daemon Process Ends
        if ((chdir(directory)) < 0) {
         exit(1); }



        /*Memory Allocation For RequestHandler Thread Array*/
        threads=(pthread_t*)malloc(numberOfRequestHandlerThreads*sizeof(pthread_t));

        /*Create a Thread Pool*/
        createThreadPool(numberOfRequestHandlerThreads,threads,&listenerThread,&schedulerThread);
        /*Thread Pool Creation ends*/

        /*Thread Waiting For Joining*/
        (void) pthread_join(listenerThread, NULL);
        (void) pthread_join(schedulerThread, NULL);
        for(int i=0;i<=numberOfRequestHandlerThreads;i++){
        (void) pthread_join(threads[i], NULL);
        /*Thread Waiting For Joining*/

        closelog();
        }
}


     /*This function creates a thread pool */
     void createThreadPool(int numberOfRequestHandlerThreads,pthread_t *threads,pthread_t *listenerThreadPointer,pthread_t *schedulerThreadPointer){
    	 string testString = "ThreadPool Has Been created\n";
    	 if(numberOfRequestHandlerThreads>=1){


    		 /*Creation of  Request Handler Threads End*/
    		 /*Create Listener and Scheduler Threads*/
    		 pthread_create(listenerThreadPointer,  NULL,&serverSetup, NULL);
    		 pthread_create(schedulerThreadPointer, NULL,&schedule,    NULL);

    		 /*Create Request Handler Threads*/
    		 for(int i=0; i<numberOfRequestHandlerThreads;i++){
    		    pthread_create(&threads[i], NULL,&serveClientRequest,NULL);
    		 }


    		 //write(1, testString.c_str(),strlen(testString.c_str()));
    	 }
    	 else{
    		 cout<<"Invalid Number Of Threads";//Handle Errors
    	 }

     }




        void *serverSetup(void *x){
        string testString1="Server Setup has been done\n";
        string testString2="A new Client is connected\n";
        struct sockaddr_in socketAddress;        	        /*The address of the socket */
        struct hostent  *hostDetail;
        char hostName[256];
        int sockedId,socketFileDescriptor;
        int iMode=0;
        bzero( &socketAddress, sizeof(socketAddress) );/*Initializing socketAddress to 0 */

        /*Getting HostName Details*/
        gethostname( hostName , 256);
        hostDetail = gethostbyname( hostName );
        /*Getting HostName Details Ends*/

        /*Initialize Socket Address */
        bcopy( hostDetail->h_addr, &socketAddress.sin_addr,hostDetail->h_length);
        socketAddress.sin_family = AF_INET ;
        socketAddress.sin_port = htons(portNumber);
        /*Initialize Socket Address Ends*/

        /*Creating a socket*/
        sockedId = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sockedId == -1 ) {
            errorMessage( "Socket couldnot be created" );
        }
        /*Creating a socket Ends*/

        /*Binding the socket to the given address*/
        if ( bind(sockedId,(const sockaddr*) &socketAddress,sizeof(socketAddress)) != 0 ){
            errorMessage( "Couldnot bind the socket" );
        }
        /*Binding the socket to the given address Ends*/

        /*Listen to a socket*/
        if ( listen(sockedId, 1) != 0 ){
            errorMessage( "Unable to Listen on the provided port" );
        }

        //write(1, testString1.c_str(),strlen(testString1.c_str()));
         /*Listen to a socket ends*/


         while ( 1 ){
         socketFileDescriptor = accept(sockedId, NULL, NULL); /* wait for call */
        //ioctl(socketFileDescriptor, FIONBIO, &iMode);
         //printf("** Server: A new client is connected");
         //write(1, testString2.c_str(),strlen(testString2.c_str()));
         if ( socketFileDescriptor == -1 ){
             errorMessage( "Couldnot get a new connection" );
          }
         else{
             putVector(socketFileDescriptor);//Push the descriptor in the vector
            // schedule();
             /*Serving the client Request*/
   //          serveClientRequest();
              }
          }
}




/*This method will put the descriptors in the vector*/
void putVector(int socketFileDescriptor){
	 string testString="Descriptor has been put into the vector\n";
     sem_wait(&vectorMutex);
     socketFileDescriptorList.push_back(socketFileDescriptor);
     sem_post(&vectorSize);
    // write(1, testString.c_str(),strlen(testString.c_str()));
     sem_post(&vectorMutex);

}


/*This method will pull details from the vector and put it in a list
according to the scheduling algorithm*/
void *schedule(void *x){
	string testString1 = "The request has been taken from the vector\n";
	string testString2 = "The request has been put into the list\n";
	string testString3="";
	DIR* directoryStructure;
	struct dirent* directoryEntry;

    int socketFileDescriptor=0;
    int errorCode=-1;
    char *buffer;
    string requestString="";
    struct clientRequest request;
    FILE *fileDescriptor;
    int fileSize;
    string requestType;
    string requestedFile;
    string requestProtocol;
    int startIndex;
    int endIndex;
    int length;
    size_t size=200;
    char * charDirectory;
    char * directory;
    string stringDirectory;
    string filePath="";
    ifstream readFile;
    stringstream indexStream;
    int indexFlag=0;

    while(1){
    /*Reading file descriptors from the vector*/
    buffer=(char *)malloc(sizeof(char)*BUFSIZ);
    requestString="";
    sem_wait(&vectorSize);
    sem_wait(&vectorMutex);
    socketFileDescriptor=socketFileDescriptorList.front();
    socketFileDescriptorList.pop_front();
    //write(1, testString1.c_str(),strlen(testString1.c_str()));
    sem_post(&vectorMutex);


    request.socketFileDescriptor=socketFileDescriptor;
    /*Reading data from file descriptors and storing in a string */

    //socketFileDescriptorReader = fdopen (socketFileDescriptor, "r" );
   //   FILE *sock_fr = fdopen(socketFileDescriptor,"r"); /* we'll write to the */
      /*while ((bytes = read(s, buffer, BUFSIZ)) > 0)
        write(1, buffer, bytes);*/
      ////char c;
    //  while( (c = getc(sock_fr) ) != EOF )
    //	  requestString+=c;
    //  fclose(sock_fr);
   /* while ((numberOfBytes = read( socketFileDescriptor,buffer, BUFSIZ)) > 0){
        requestString+=buffer;
    }*/

    while((errorCode=read( socketFileDescriptor,buffer, BUFSIZ))<=0);
    if(errorCode==-1){
    	testString3="error";
    }
    else
    	testString3=" reading Successful\n";
    //write(1, testString3.c_str(),strlen(testString3.c_str()));
    //sscanf(buffer,"%s,%s,%s",requestType,requestedFile,requestProtocol);
    requestString=buffer;
    delete(buffer);

    /*Trimming the string for leading and trailing spaces*/
    startIndex=requestString.find_first_not_of(' ');
    endIndex=requestString.find_last_not_of(' ');
    length=endIndex-startIndex + 1;
    requestString=requestString.substr(requestString.find_first_not_of(' '),length );

    /*Getting The Request Type */
    if (strcasecmp(requestString.substr(0,3).c_str(),"GET")==0){
        request.clientRequestType="GET";
        requestedFile=requestString.substr(3);

    }else if (strcasecmp(requestString.substr(0,4).c_str(),"HEAD")==0){
        request.clientRequestType="HEAD";
        requestedFile=requestString.substr(4);
        }
    else{
        //Send the error message
            return (void*)errorCode;//check here
        }




    /*requestString=requestString.substr(requestString.find_first_not_of(' '),requestString.find_last_not_of(' ') + 1);  //put error checks
    Finding the request Type and populating the client request structure
    if (strcasecmp(requestString.substr(0,3).c_str(),"GET")==0){
    request.clientRequestType="GET";
    requestFile=requestString.substr(3,requestString.size());

    }else if (strcasecmp(requestString.substr(0,4).c_str(),"HEAD")==0){
    request.clientRequestType="HEAD";
    requestFile=requestString.substr(4,requestString.size());
    }
    else{
    //Send the error message
        return (void*)errorCode;//check here
    }

*/

    /*if(strcasecmp(requestType,"GET")==0){
    	request.clientRequestType="GET";
    }
    else if (strcasecmp(requestType,"HEAD")==0){
    	request.clientRequestType="HEAD";
    }
    else{
    	return (void*) errorCode;
    }


    request.clientRequestFileName=requestedFile;

*/


    /*Fetching the request File name and storing it in client request structure */
    requestedFile=requestedFile.substr(requestedFile.find_first_not_of(' ')); //Trimming leading spaces
    startIndex=0;
    endIndex=requestedFile.find_first_of(' ');
    length=endIndex-startIndex;
    request.clientRequestFileName=requestedFile.substr(0,length);

    /*Handling directory*/
    //directory=getcwd(charDirectory, size);
    //stringDirectory = string(directory);

    /*Appending / towards the end if it doesnot already exist */
    //if (directory[strlen(directory)-1]!='/'){
    //	stringDirectory.append(1,'/');
    //}

    /*Handling Browser Requests and removing / if it exists at the start of the file name */
    if (request.clientRequestFileName.find('/')==0){
    	request.clientRequestFileName=request.clientRequestFileName.substr(1);
    }

    /*Getting the entire File Path */
    /*filePath.append(stringDirectory);
    filePath.append(request.clientRequestFileName);

    request.clientRequestFileName=filePath;
    filePath="";*/

    /*Fetching the content Type*/
     if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"html")==0){
    request.fileContentType="text/html";
    }
    else if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"txt")==0){
    	request.fileContentType="text/html";
    }
    else if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"gif")==0){
    	request.fileContentType="image/gif";
    }
    else if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"jpg")==0){
    	request.fileContentType="image/jpeg";
    }
    else if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"jpeg")==0){
        	request.fileContentType="image/jpeg";
    }
    else if(strcasecmp(request.clientRequestFileName.substr(request.clientRequestFileName.find_last_of(".") + 1).c_str(),"png")==0){
        	request.fileContentType="image/png";
    }
    else{
    	request.fileContentType="";
    }



     //Populating the Protocol
     requestProtocol=requestedFile.substr(requestedFile.find_first_of(' ')); //Trimming leading spaces
     requestProtocol=requestProtocol.substr(requestProtocol.find_first_not_of(' '),8); //Trimming leading spaces

    /*Verifying the protocol*/
    if(strcasecmp(requestProtocol.c_str(),"HTTP/1.0")!=0 && strcasecmp(requestProtocol.c_str(),"HTTP/1.1")!=0){
    	return (void*)errorCode;//check here
    }

    //Determining The Size of The File Starts*/
    /*if (request.fileContentType=="text/html"){
    fileDescriptor=fopen(request.clientRequestFileName.c_str(), "r");
    fseek(fileDescriptor, 0, SEEK_END);
    request.clientRequestFileSize= ftell(fileDescriptor);
    fclose(fileDescriptor);
    }*/
    /*else{*/

   stat(request.clientRequestFileName.c_str() ,&request.fileAttribute);		// get the attributes of the file


   if (S_ISDIR(request.fileAttribute.st_mode)!=0){
	   indexFlag=2;
           directoryStructure = opendir(request.clientRequestFileName.c_str() );
           if( directoryStructure == NULL ) {
              perror( "cannot open the given directory" );
      }
           else{
           while((directoryEntry = readdir( directoryStructure))!=NULL ){
           if(strcasecmp(directoryEntry->d_name,"index.html")==0){
           indexStream <<request.clientRequestFileName.c_str();
           indexStream << "/index.html";
           request.clientRequestFileName=indexStream.str();
           request.fileContentType="text/html";
           stat(request.clientRequestFileName.c_str() ,&request.fileAttribute);		// get the attributes of the file
           indexStream.str("");
           indexFlag=1;
           break;
      }
      }
           }
           closedir( directoryStructure );
        }


   readFile.open(request.clientRequestFileName.c_str(),ios::in | ios::ate);
    if (readFile.is_open() &&  indexFlag!=2){
    	request.clientRequestFileSize = readFile.tellg();
    	readFile.close();
    }
    else{
    	request.clientRequestFileSize=0;
    }
    indexFlag=0;
    /*}*/
    //Determining The Size of The File Ends */

/*Considering the Scheduling Algorithm to be FCFS*/
    sem_wait(&listMutex);
    insertClientRequestList(&request);
    sem_post(&listSize);
    //write(1, testString2.c_str(),strlen(testString2.c_str()));
    sem_post(&listMutex);

    }
}



void insertClientRequestList(clientRequest *request)
{
    int counter;
    /*When the List is empty just push the item at the start*/
   if (clientRequestList.size()==0 ){
       clientRequestList.push_back(*request);
       return;
   }
   /*When the scheduling algorithm is FCFS we can push the item always towards the end*/
   else if(strcasecmp(schedulingPolicy,"FCFS")==0 ){
       clientRequestList.push_back(*request);
       return;
   }
   /*When the scheduling algorithm is SJF push the item according to the request File size*/
   else{
       clientRequestListIterator= clientRequestList.begin();
       for(counter=0;counter<clientRequestList.size();counter++){
           if(request->clientRequestFileSize  >= clientRequestListIterator->clientRequestFileSize){
               clientRequestListIterator++;
           }
               else{

           clientRequestList.insert(clientRequestListIterator,(const clientRequest&)*request);
                return;
               }
           }
       /*The below condition is to push the item after the last member of the list*/
       clientRequestList.push_back(*request);
       return;
       }

}

void *serveClientRequest(void *abc){

    string testString = "The Request has been Fetched\n";
    clientRequest  currentRequest;
    int socketFileDescriptor;
    string clientRequestFileName;
    ifstream inputFileStream;
    string data;
    int count;
    FILE *writeFile;
    time_t epochTime;
    struct tm * timeinfo;
    string httpFileFoundStatusMessage    ="HTTP/1.0 200 OK\n";
    string dateLabel            ="Date               :";
    string serverLabel          ="Server             :";
    string lastModifiedLabel    ="Last-Modified      :";
    string contentTypeLabel     ="Content-Type       :";
    string ContentLengthLabel   ="Content-Length     :";
    struct stat fileAttribute;
    stringstream indexFile;
    char* imageData;
    stringstream directoryList;
    stringstream pageNotFound;
    DIR* directoryStructure;
    struct dirent* directoryEntry;
    int indexFileFlag=0;
/*  char *dataPointer;
    FILE *readFile;


    char c;*/
    /*FILE *socketFile;*/


    /*Waiting on Queuing Time */
//    sleep(queuingTime);                //Test


    while(1){
    	sleep(queuingTime);
        directoryList.str("");
    sem_wait(&listSize);
    sem_wait(&listMutex);
    currentRequest=clientRequestList.front();      /*Fetch the request to be served*/
    clientRequestList.pop_front();
    //write(1, testString.c_str(),strlen(testString.c_str()));
    sem_post(&listMutex);
    socketFileDescriptor=currentRequest.socketFileDescriptor;
    clientRequestFileName=currentRequest.clientRequestFileName;
    writeFile = fdopen(socketFileDescriptor,"w");
    //Obtaining the request attributes

    //Request attribute stored

  /*  char *clientRequestFileNameArray=new char[clientRequestFileName.size()+1];
    clientRequestFileNameArray[clientRequestFileName.size()]=0;
    memcpy(clientRequestFileNameArray,clientRequestFileName.c_str(),clientRequestFileName.size());*/
    //readFile = fopen( (const char*)&clientRequestFileName , "r");
    //readFile = fopen("abcd.txt" , "r");

    //strcpy (directoryList,"<HTML><BODY> ");
    if (S_ISDIR(currentRequest.fileAttribute.st_mode)!=0){
    	indexFileFlag=2;
        currentRequest.fileContentType="text/html";
        directoryList << "<HTML><BODY><H4> ";
         directoryStructure = opendir(currentRequest.clientRequestFileName.c_str() );
         if( directoryStructure == NULL ) {
            perror( "cannot open the given directory" );
    }
         else{
         while((directoryEntry = readdir( directoryStructure))!=NULL ){
         if(strcasecmp(directoryEntry->d_name,"index.html")==0){
         indexFileFlag=1;
         indexFile <<currentRequest.clientRequestFileName.c_str();
         indexFile << "/index.html";
         currentRequest.clientRequestFileName=indexFile.str();
         stat(currentRequest.clientRequestFileName.c_str() ,&currentRequest.fileAttribute);		// get the attributes of the file
         indexFile.str("");

         break;
    }
         else{
        //	strcat(directoryList,directoryEntry->d_name);
        //	strcat(directoryList,"<br>");
        	 if(directoryEntry->d_name[0]!='.'){
        	 directoryList << directoryEntry->d_name;
        	 directoryList << "<br>";
        	 }
         }
    }
         }
         directoryList << "</BODY></HTML>";
         closedir( directoryStructure );

    //strcat(directoryList,"</BODY></HTML>");




    }


    /*Opening the socket to write and the file to read from */

    if((currentRequest.fileContentType=="text/html" || currentRequest.fileContentType=="" ) && indexFileFlag!=2){
    inputFileStream.open(clientRequestFileName.c_str());
    if(!inputFileStream.is_open()){
    	fileNotFound(currentRequest.socketFileDescriptor);
    	continue;
    }

    }
    else if(currentRequest.fileContentType=="image/jpg" || currentRequest.fileContentType=="image/jpeg" || currentRequest.fileContentType=="image/gif" || currentRequest.fileContentType=="image/png" ){
    inputFileStream.open(clientRequestFileName.c_str(),ios::in|ios::binary);
        if(!inputFileStream.is_open()){
        	fileNotFound(currentRequest.socketFileDescriptor);
        	continue;
        }
	imageData = new char[currentRequest.clientRequestFileSize];
    }



    /*Writing The header Of the File Starts*/

    /*HTTP Status Code*/

    write(socketFileDescriptor,httpFileFoundStatusMessage.c_str(),strlen(httpFileFoundStatusMessage.c_str()));




    /*date */
    write(socketFileDescriptor,dateLabel.c_str(),strlen(dateLabel.c_str()));
    time (&epochTime );
    timeinfo = localtime(&epochTime);
    write(socketFileDescriptor,asctime(timeinfo),strlen(asctime(timeinfo)));

    /*Server Details */
    write(socketFileDescriptor,serverLabel.c_str() ,strlen(serverLabel.c_str()));
    write(socketFileDescriptor,"Apache V0.1\n", strlen("Apache V0.1\n"));


    /*Last Modified */
    write(socketFileDescriptor,lastModifiedLabel.c_str(),strlen(lastModifiedLabel.c_str()));
    //stat(currentRequest.clientRequestFileName.c_str() ,&fileAttribute);		// get the attributes of the file
    if (indexFileFlag!=2){
    write(socketFileDescriptor,asctime(gmtime(&(currentRequest.fileAttribute.st_mtime))),strlen(asctime(gmtime(&(fileAttribute.st_mtime)))));
    }
    else{
    	write(socketFileDescriptor,"\n",1);
    }

    /*Content Type */
    write(socketFileDescriptor,contentTypeLabel.c_str(),strlen(contentTypeLabel.c_str()));
    write(socketFileDescriptor,currentRequest.fileContentType.c_str(),strlen(currentRequest.fileContentType.c_str()));
    write(socketFileDescriptor,"\n",1);

    /*Content Length */
    write(socketFileDescriptor,ContentLengthLabel.c_str(),strlen(ContentLengthLabel.c_str()));
    if (indexFileFlag!=2){
    write(socketFileDescriptor,numberToString(currentRequest.clientRequestFileSize).c_str(),strlen(numberToString(currentRequest.clientRequestFileSize).c_str()));
    }
    else{
    	write(socketFileDescriptor,numberToString(directoryList.str().length()).c_str(),strlen(numberToString(directoryList.str().length()).c_str()));
    }
    write(socketFileDescriptor,"\n",1);
    /*Writing The header Of the File Ends */

	/*Writing the File Contents Only If the Request is of Type GET */
	if (strcasecmp(currentRequest.clientRequestType.c_str(),"GET")==0 && S_ISREG(currentRequest.fileAttribute.st_mode)!=0 && indexFileFlag!=2) {
	if(currentRequest.fileContentType=="text/html"){
    while(!inputFileStream.eof()){ // To get you all the lines.

    write(socketFileDescriptor,"\n",1);
   	getline(inputFileStream,data); // Saves the line in data string
   	write(socketFileDescriptor,data.c_str(),strlen(data.c_str()));
    }
	write(socketFileDescriptor,"\n",1);
       	//cout<<data; // Prints our STRING.
   	/*  char *dataArray=new char[data.size()+1];
   	dataArray[data.size()]=0;
   	memcpy(dataArray,data.c_str(),data.size());
   	cout<<data.c_str();*/
    }
	else if (indexFileFlag!=2){
	write(socketFileDescriptor,"\n",1);
    inputFileStream.read (imageData,currentRequest.clientRequestFileSize);
    write(socketFileDescriptor,imageData,currentRequest.clientRequestFileSize);
    delete[] imageData;
	}
	}

    inputFileStream.close();



    if(indexFileFlag==2){
    	 write(socketFileDescriptor,"\n",1);
       	  write(socketFileDescriptor,directoryList.str().c_str(),strlen(directoryList.str().c_str()));
       	  close(socketFileDescriptor);
       	  continue;
       	  //strcpy(directoryList,"");
         }
         indexFileFlag=0;



    close(socketFileDescriptor);
    //fclose(writeFile);  //Error Handling
    //socketFile = fdopen(socketFileDescriptor,"w");
    /*if(socketFile==NULL){
        printf("Error");
    }
    else {*/
        //fprintf(socketFile,"Hello");


    /*while( (c = getc(readFile) ) != EOF )
    putc(c, writeFile);
    fclose(readFile);*/

    		/*inputFileStream.open ((const char*)&clientRequestFileName);
    	        while(!inputFileStream.eof()) // To get you all the lines.
    	        {
    		        getline(inputFileStream,data); // Saves the line in STRING.
    		        write(socketFileDescriptor,(const char*)&data,sizeof(data));
    		   //     cout<<data; // Prints our STRING.
    	        }
    	        inputFileStream.close();*/
    	/*write(socketFileDescriptor, "Hello", 5);*/
    /*}*/
    }
}



void printUsage(){


	fprintf(stderr,"usage:myhttpd [−d] [−h] [−l file] [−p port] [−r dir] [−t time] [−n threadnum] [−s sched]\n");
	        fprintf(stderr,
	                        "−d : Enter debugging mode. That is, do not daemonize, only accept one connection at a time and enable logging to stdout. Without this option, the web server should run as a daemon process in the background.\n"
	                                        "−h : Print a usage summary with all options and exit.\n"
	                                        "−l file : Log all requests to the given file. See LOGGING for details.\n"
	                                        "−p port : Listen on the given port. If not provided, myhttpd will listen on port 8080.\n"
	                                        "−r dir : Set the root directory for the http server to dir.\n"
	                                        "−t time : Set the queuing time to time seconds. The default should be 60 seconds.\n"
	                                        "−n threadnum: Set number of threads waiting ready in the execution thread pool to threadnum. The default should be 4 execution threads.\n"
	                                        "−s sched : Set the scheduling policy. It can be either FCFS or SJF. The default will be FCFS.\n");

	        exit(1);

}


  template <typename T>
  string numberToString ( T Number )
  {
     ostringstream ss;
     ss << Number;
     return ss.str();
  }



  void fileNotFound(int socketFileDescriptor){
	  time_t epochTime;
	  struct tm * timeinfo;
	  string dateLabel            ="Date               :";
	  string httpFileNotFoundStatusMessage    ="HTTP/1.0 404 Not Found \n";
	  string message="\n\n<HTML><BODY><H3>Page Not Found </H3></BODY></HTML>";
	  write(socketFileDescriptor,httpFileNotFoundStatusMessage.c_str(),strlen(httpFileNotFoundStatusMessage.c_str()));
	  write(socketFileDescriptor,dateLabel.c_str(),strlen(dateLabel.c_str()));
	  time (&epochTime );
	  timeinfo = localtime(&epochTime);
	  write(socketFileDescriptor,asctime(timeinfo),strlen(asctime(timeinfo)));
	  write(socketFileDescriptor,asctime(timeinfo),strlen(asctime(timeinfo)));
	  close(socketFileDescriptor);

  }

