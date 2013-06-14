#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <map>

using namespace std;

void SYS_ERROR(string str){
  perror(str.c_str());
//   exit(1);
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define SYS_ERROR(msg) SYS_ERROR(string( STR(__LINE__) ": " ) + msg)

#define EXIT_CODE_COUNTER (__COUNTER__ + 1)

#define NYI SYS_ERROR("Not yet implemented.");

void interrupt(int signo){ exit(0); }

const string bialeZnaki = " \t\r";

const int WIELKOSC_BACKLOGU = 100;

int deskryptorSocketuAkceptora = -1;
vector<pair<FILE *, string> > handlerySocketowINazwyZespolowPerKlient;

void clean()
{
        if(-1 != deskryptorSocketuAkceptora)
          close(deskryptorSocketuAkceptora);
        for(int i=0;i<handlerySocketowINazwyZespolowPerKlient.size();++i)
          if(NULL != handlerySocketowINazwyZespolowPerKlient[i].first)
            fclose(handlerySocketowINazwyZespolowPerKlient[i].first);
}

void sendString(FILE * handler, const string & wiadomosc, bool newline = true){
  fprintf(handler, newline ? "%s\n" : "%s", wiadomosc.c_str());
  fflush(handler);
}

string recieveString(FILE * handler){
  string ret;
  do ret += getc(handler); while('\n' != ret[ret.size()-1]);
  return ret;
}

void * watekPerKlient(void* _arg){
  
  FILE * handlerSocketu = ((pair<FILE *, string> *)_arg)->first;
  string nazwaDruzyny = ((pair<FILE *, string> *)_arg)->second;
  
NYI //TODO
  
  return 0;
} // koniec watku per klient

map<string, string> daneDruzyn;

void * watekAkceptora(void* _arg){
  
  for(;;){
    int deskryptorSocketuKlienta =  accept(deskryptorSocketuAkceptora, NULL, NULL);
    if(-1 == deskryptorSocketuKlienta){SYS_ERROR("accept error"); continue;}
    
    FILE * handlerSocketuKlienta = fdopen(deskryptorSocketuKlienta, "r+");
    if(NULL == handlerSocketuKlienta){SYS_ERROR("fdopen error"); continue;}
    
    sendString(handlerSocketuKlienta, "LOGIN");
    string nazwaDruzynyKlienta = recieveString(handlerSocketuKlienta);
    sendString(handlerSocketuKlienta, "PASS");
    string haslo = recieveString(handlerSocketuKlienta);
    
    map<string, string>::iterator it = daneDruzyn.find(nazwaDruzynyKlienta);
    if(daneDruzyn.end() == it){
      sendString(handlerSocketuKlienta, "FAILED 1 bad login or password");
      if(EOF == fclose(handlerSocketuKlienta)){SYS_ERROR("fclose error"); continue;}
    }
    else{
      sendString(handlerSocketuKlienta, "OK");
      
      handlerySocketowINazwyZespolowPerKlient.push_back(make_pair(handlerSocketuKlienta, nazwaDruzynyKlienta));
      
      cout << "Polaczono z klientem druzyny \"" << nazwaDruzynyKlienta << '"' << endl;
      
      pthread_t dummy;
      if(pthread_create(&dummy, NULL, watekPerKlient, (void*)&handlerySocketowINazwyZespolowPerKlient.back())){SYS_ERROR("pthread_create error"); exit(EXIT_CODE_COUNTER);}
    }
  }
  
} // koniec watku akceptora

int main(int argc, char ** argv){
  if(2 != argc){SYS_ERROR("Uzycie: ./serwer numerPortu\n"); return EXIT_CODE_COUNTER;}
  
  char * endptr;
  int numerPortu = strtol(argv[1], &endptr, 0);
  if(*endptr || numerPortu < 0){SYS_ERROR("Numer portu musi byc nieujemna liczba calkowita."); return EXIT_CODE_COUNTER;}
  
  /////
  
  ifstream usersInputFileStream("users");
  if(usersInputFileStream.fail()){SYS_ERROR("Nie udalo sie otworzyc pliku \"users\", zawierajacego dane logowania druzyn."); return EXIT_CODE_COUNTER;}
  
  string login, pass;
  while(usersInputFileStream >> login >> pass)
    daneDruzyn[login] = pass;
  
  usersInputFileStream.clear();
  
  usersInputFileStream.close();
  if(usersInputFileStream.fail()){SYS_ERROR("ifstream::close error"); return EXIT_CODE_COUNTER;}
  
  /////

  if(atexit(clean)){SYS_ERROR("atexit error"); return EXIT_CODE_COUNTER;}
  if(SIG_ERR == signal(SIGINT, interrupt)){SYS_ERROR("signal error"); return EXIT_CODE_COUNTER;}
  
  /////
  
NYI //TODO setup socketu akceptora i stworzenie jego watku
  
  /////
  
  return 0;

}























